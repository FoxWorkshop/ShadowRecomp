#include <kernel/function.h>
#include <user/config.h>
#include <gpu/video.h>

const uint32_t g_deltaTime = 0x541FA8;
const uint32_t g_refRate = 0x54DD80;
const uint32_t g_frameRateDivisor = 0x54DD40;

static bool g_isEventPlaying = false;
void SetFramerateMidAsmHook()
{
	auto base = g_memory.base;
	if (g_isEventPlaying == false)
	{
		X86_STORE_U32(g_refRate, (uint32_t)GetTargetFPS());
	}
}

// TPjsEventActionManager::Exec
X86_FUNC_IMPL(__imp__sub_228740);
X86_FUNC(sub_228740)
{
	g_isEventPlaying = true;
	__imp__sub_228740(ctx, base);

}

// TPjsEventActionManager Destructor
X86_FUNC_IMPL(__imp__sub_2293E0);
X86_FUNC(sub_2293E0)
{
	g_isEventPlaying = false;
	__imp__sub_2293E0(ctx, base);

}

// Get the requested in-game framerate
// TPjsEventActionManager sets divisor to 2 for events
// 0 represents no internal framerate request (safe to do any limit)
float GetInternalFrameRate()
{
	auto base = g_memory.base;
	uint32_t refRate = X86_LOAD_U32(g_refRate);
	int32_t divisor = (int32_t)X86_LOAD_U32(g_frameRateDivisor);
	if (refRate == 0 || divisor <= 0)
		return 0.0f;

	float fps = (float)refRate / (float)divisor;
	return (fps >= 1.0f && fps <= 500.0f) ? fps : 0.0f;
}

// Patch the internal busy-wait (sub_56FC0) to not burn a core
void FrameLimiterWaitMidAsmHook(X86Register& ebx, X86Register& esi)
{
	auto base = g_memory.base;

	uint32_t refRate = X86_LOAD_U32(g_refRate);
	int32_t divisor = (int32_t)X86_LOAD_U32(g_frameRateDivisor);
	if (refRate == 0 || divisor <= 0)
		return;

	int64_t period = (int64_t)(1000000.0 / (double)refRate);
	int64_t target = period * divisor;
	int64_t elapsed = (int64_t)(((uint64_t)esi.u32 << 32) | ebx.u32);
	int64_t remaining = target - elapsed;

	if (remaining > 2500)
		std::this_thread::sleep_for(std::chrono::microseconds(remaining - 2000));
}

std::unordered_map<uint32_t, float> g_sfxTimers;
// SFX duration are hardcoded without using any kind of delta time
// We need to patch them so they don't cut too early
void SFXTimingMidAsmHook(X86Register &esi)
{
	if (GetFPS() < 60)
		return;

	auto base = g_memory.base; // use base for the load and store macro.

	uint32_t timerAddr = esi.u32 + 0xc;
	uint16_t timer = X86_LOAD_U16(timerAddr) + 1; // restore og value that the game removed using hardcoded 60 fps.

	g_sfxTimers[esi.u32] += X86_LOAD_F32(g_deltaTime) * 60.f;
	int steps = (int)(g_sfxTimers[esi.u32] + 0.5f); // use extra 0.5 for rounding, avoid having 0.999960 that prevent step to pass.
	if (steps > 0)
	{
		g_sfxTimers[esi.u32] -= steps;
		int result = (int)timer - steps;
		timer = (uint16_t)std::max(result, 0);
	}


	X86_STORE_U16(timerAddr, timer);
}

void FreeSFXMapMidAsmHook(X86Register& esi)
{
	g_sfxTimers.erase(esi.u32);
}

void FpsConfigValueChangedCallback(IConfigDef* config)
{
	if (config == &Config::FPS)
	{
		float rounded = std::roundf(Config::FPS.Value * 100.0f) / 100.0f;

		Config::FPS.Value = rounded;

		if (Config::FPS.Value < 0.01f)
			Config::FPS.Value = 0.0f;

		if (Config::FPS.Value > FPS_MAX)
			Config::FPS.Value = FPS_MAX;
	}
}

// Set while the jump board patch is replaying a launch collision pass with 60 FPS geometry.
// See the jump board patch further down
static int g_launchCollisionPass = 0;

// Collision patch globally, but sticking in here to check for the high FPS launch fix
// Fix ray to ground 'stick'
// Mainly for higher framerates, since even small slopes broke
// Unlike the original 61FPS+ patch, I scope to non-grounded (no more camera, snap, or slide Y push issue!)
// There are a few spots that originally break on 60 too (Space Gadget CC Orb area, Death Ruins spawn)
bool CollisionGroundStickMidAsmHook(X86Register& esi, X86Register& esp)
{
	auto base = g_memory.base;

	// Don't interfere with the Launch fix
	if (g_launchCollisionPass > 0)
	{
		return false;
	}

	const uint32_t player = X86_LOAD_U32(esi.u32);
	if (player == NULL)
	{
		return false;
	}

	// Behavior is already Grounded
	if ((X86_LOAD_U32(player + 0xa8) & 0x4) != 0)
	{
		return false;
	}

	// ray did not hit
	if (X86_LOAD_U8(esp.u32 + 0x3f) == 0)
	{
		return false;
	}

	// skip to second sweep
	return true;
}

// Common platforms that are used in many different stages
// the mode linear type movement don't use delta time so the platforms move way too fast above 60 fps
// we can patch this to cap the speed properly
void FootingMovableLinearMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t this_ = esi.u32;
	const float speed = X86_LOAD_F32(this_ + 0x54); // degree
	const float phase = X86_LOAD_F32(this_ + 0x7C); // also degree
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	X86_STORE_F32(this_ + 0x7C, phase + speed * (frames - 1.0f));
}

// reel (pulley) goes with fixed speed instead of delta time

static const uint32_t g_upDownReelStep = 0x468A18; // 0.18f originally

static void UpDownReelPreBias(uint32_t this_, float sign)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.f;
	const float bias = sign * X86_LOAD_F32(g_upDownReelStep) * (frames - 1.0f);
	X86_STORE_F32(this_ + 0x58, X86_LOAD_F32(this_ + 0x58) + bias);
}

void UpDownReelRaiseMidAsmHook(X86Register& edi)
{
	UpDownReelPreBias(edi.u32, 1.0f);
}

void UpDownReelLowerMidAsmHook(X86Register& edi)
{
	UpDownReelPreBias(edi.u32, -1.0f);
}

static const uint32_t g_upDownReelAngleRate = 0x54CD64;

void UpDownReelAngleMidAsmHook(X86Register& edi)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	const float angle = X86_LOAD_F32(edi.u32 + 0x7C);
	const float extra = X86_LOAD_F32(g_upDownReelAngleRate) * (frames - 1.0f);

	if (extra >= std::fabs(angle))
	{
		X86_STORE_F32(edi.u32 + 0x7C, 0.0f);
	}
	else
	{
		X86_STORE_F32(edi.u32 + 0x7C, angle - (angle >= 0.0f ? extra : -extra));
	}
}

void UpDownReelHangTurnMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const float rate = std::bit_cast<float>(eax.u32) * frames;
	eax.u32 = std::bit_cast<uint32_t>(rate);
}

/**	--SetStretchGrass Patch START-- */
// aka death ruin object that launch you 
// for some reason only the mode 5 of this object use delta time
// anything else doesn't so we have to patch the math

static const uint32_t g_stretchGrassChargeStep = 0x46A558; // 0.005f
static const uint32_t g_stretchGrassRecoverStep = 0x46A560; //0.42f
static const uint32_t g_stretchGrassDecayFactor = 0x46A608; // -0.93f

// for mode 2 of the object
// the charge builds with frame count and then launch gets stronger the higher the framerate goes
void StretchGrassChargeMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t this_ = esi.u32;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	const float step = X86_LOAD_F32(g_stretchGrassChargeStep);
	const float acc = X86_LOAD_F32(this_ + 0x4C) + step * frames;

	X86_STORE_F32(this_ + 0x4C, acc - step);
	X86_STORE_F32(this_ + 0x48, X86_LOAD_F32(this_ + 0x48) + acc * (frames - 1.0f));
}

// for mode 3 of the object
void StretchGrassRecoverMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t self = esi.u32;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	const float step = X86_LOAD_F32(g_stretchGrassRecoverStep);
	X86_STORE_F32(self + 0x48, X86_LOAD_F32(self + 0x48) - step * (frames - 1.0f));
}

// for mode 4 of the object
std::unordered_map<uint32_t, float> g_stretchGrassDecay;

void StretchGrassDecayMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t this_ = esi.u32;
	const float factor = X86_LOAD_F32(g_stretchGrassDecayFactor);

	float &carry = g_stretchGrassDecay[this_];
	carry += X86_LOAD_F32(g_deltaTime) * 60.0f;

	// just to be safe
	int steps = std::min((int)carry, 128);
	carry -= steps;

	float value = X86_LOAD_F32(this_ + 0x48);

	if (steps == 0)
	{
		value /= factor; // the game's own multiply puts it straight back
	}
	else
	{
		for (int i = 1; i < steps; i++)
		{
			value *= factor;
		}
	}

	X86_STORE_F32(this_ + 0x48, value);


	if (std::fabs(value * factor) < 0.001f)
		g_stretchGrassDecay.erase(this_);
}


/**	--SetStretchGrass Patch END-- */

// rocket goes way too fast above 60 fps
// some part of the velocity logic uses raw value which increase above 60
// we can patch it with proper delta time
X86_FUNC_IMPL(__imp__sub_1AD1A0);
X86_FUNC(sub_1AD1A0)
{
	const uint32_t rocketPtr = ctx.ecx.u32; // thiscall
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	const float velX = X86_LOAD_F32(rocketPtr + 0x4C);
	const float velY = X86_LOAD_F32(rocketPtr + 0x50);
	const float velZ = X86_LOAD_F32(rocketPtr + 0x54);

	X86_STORE_F32(rocketPtr + 0x4C, velX * frames);
	X86_STORE_F32(rocketPtr + 0x50, velY * frames);
	X86_STORE_F32(rocketPtr + 0x54, velZ * frames);

	__imp__sub_1AD1A0(ctx, base);

	X86_STORE_F32(rocketPtr + 0x4C, velX);
	X86_STORE_F32(rocketPtr + 0x50, velY);
	X86_STORE_F32(rocketPtr + 0x54, velZ);
}

// Light in digital circuit that yeet Shadow
// the speed when getting launch is way too high above 60
// we can patch the scale value stored in the vector just before the launch to use delta time.
void ElecSearchLightPullMidAsmHook(X86Register &esp)
{
	auto base = g_memory.base;
	const uint32_t scaleAddr = esp.u32;
	float step = X86_LOAD_F32(scaleAddr) * (X86_LOAD_F32(g_deltaTime) * 60.0f);
	X86_STORE_F32(scaleAddr, std::min(step, 45.0f));
}

/** --ScatteringRing Patch START-- */

// Rings that drop on the floor when you get hit
// they go too fast above 60 fps so we adjust the physics to use delta time.

static const uint32_t g_scatteringRingDamp = 0x46806C; // 0.98437297f
static const uint32_t g_scatteringRingDrag = 0x46B380; // 0.1f
static const uint32_t g_scatteringRingGravity = 0x468030; // 0.12138f

void ScatteringRingPhysicsMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t this_ = esi.u32;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	const float damp = X86_LOAD_F32(g_scatteringRingDamp);
	const float drag = 1.0f - X86_LOAD_F32(g_scatteringRingDrag);
	const float gravity = X86_LOAD_F32(g_scatteringRingGravity);

	const float velX = X86_LOAD_F32(this_ + 0x54);
	const float velY = X86_LOAD_F32(this_ + 0x58);
	const float velZ = X86_LOAD_F32(this_ + 0x5C);

	const float dampN = std::pow(damp, frames);
	const float dragN = std::pow(drag, frames);
	const float terminal = -gravity / (1.0f - drag);
	const float endVelX = velX * dampN;
	const float endVelY = terminal + (velY - terminal) * dragN;
	const float endVelZ = velZ * dampN;

	// distance stuff
	const float sumDamp = damp * (1.0f - dampN) / (1.0f - damp);
	const float sumDrag = drag * (1.0f - dragN) / (1.0f - drag);
	const float travelX = velX * sumDamp;
	const float travelY = terminal * frames + (velY - terminal) * sumDrag;
	const float travelZ = velZ * sumDamp;

	X86_STORE_F32(this_ + 0x54, endVelX / damp);
	X86_STORE_F32(this_ + 0x58, (endVelY + gravity) / drag);
	X86_STORE_F32(this_ + 0x5C, endVelZ / damp);

	X86_STORE_F32(this_ + 0x48, X86_LOAD_F32(this_ + 0x48) + (travelX - endVelX));
	X86_STORE_F32(this_ + 0x4C, X86_LOAD_F32(this_ + 0x4C) + (travelY - endVelY));
	X86_STORE_F32(this_ + 0x50, X86_LOAD_F32(this_ + 0x50) + (travelZ - endVelZ));
}

// Simple visual fix so the rings rotate at normal speed at high fps.
static const uint32_t g_scatteringRingSpinPi = 0x47E0A4; // pi
static const uint32_t g_scatteringRingSpinDeg = 0x42A880; // 1/180
static const uint32_t g_scatteringRingSpinRate = 0x468054; // 1.5f

void ScatteringRingSpinMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.f;
	const float spinPi = X86_LOAD_F32(g_scatteringRingSpinPi);
	const float spinDeg = X86_LOAD_F32(g_scatteringRingSpinDeg);
	const float spinRate = X86_LOAD_F32(g_scatteringRingSpinRate);
	const float stepRes = spinPi * spinDeg * spinRate;

	X86_STORE_F32(esi.u32 + 0x64, X86_LOAD_F32(esi.u32 + 0x64) + stepRes * (frames - 1.0f));
}

/** --ScatteringRing Patch END-- */

// Regular rings spin too fast just like their drop variation
static const uint32_t g_regularRingSpinStep = 0x467D00; // 0.01f
static const uint32_t g_regularRingSpinAmplitude = 0x467D04; // 0.05f

void RegularRingSpinMidAsmHook(X86Register& eax, X86Register& ebp)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	// eax is the current ring array while ebp is the current offset
	const uint32_t entry = eax.u32 + ebp.u32;
	const float step = X86_LOAD_F32(g_regularRingSpinStep);
	const float amplitude = X86_LOAD_F32(g_regularRingSpinAmplitude);
	const float phase = X86_LOAD_F32(entry + 0x24);
	const float extra = frames - 1.0f;

	const float sway = amplitude * (std::sin(extra * step * 0.5f) / std::sin(step * 0.5f)) * std::sin(phase + (extra + 1.0f) * step * 0.5f);
	X86_STORE_F32(entry + 0x24, phase + step * extra);
	X86_STORE_F32(entry + 0x1C, X86_LOAD_F32(entry + 0x1C) + sway);
}

/**	--Weight Patch START-- */

// The big black block that drops on the player.
// State 5 and 6 don't use delta time for some reason so we patch this as usual

static const uint32_t g_weightRiseSpeed = 0x46B438; // 1.0f
static const uint32_t g_weightFallSpeed = 0x468C74; // 2.5f

// eax is the transform the block is positioned through
void WeightRiseMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	const float step = X86_LOAD_F32(g_weightRiseSpeed);
	X86_STORE_F32(eax.u32 + 0x8, X86_LOAD_F32(eax.u32 + 0x8) + step * (frames - 1.0f));
}

void WeightFallMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	const float step = X86_LOAD_F32(g_weightFallSpeed);
	X86_STORE_F32(eax.u32 + 0x8, X86_LOAD_F32(eax.u32 + 0x8) - step * (frames - 1.0f));
}

/**	--Weight Patch END-- */

// 'ContainerBehavior' patch for fall speed
// Behavior is used by Containers with type Normal, Hard, Unbreak, Crate, Weapon, Secret (aka Shadow Box)
// ContainerBehavior
// 0x20 = Y
// 0x34 = Fall speed
void ContainerBehaviorFallMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames >= 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
		return;

	const uint32_t containerBehavior = esi.u32; // ContainerBehavior
	const float fallSpeed = X86_LOAD_F32(containerBehavior + 0x34); // applies per frame, usually static per container type

	X86_STORE_F32(containerBehavior + 0x20, X86_LOAD_F32(containerBehavior + 0x20) + fallSpeed * (frames - 1.0f));
}


/**	--Damage blink patch START-- */

static bool DamageBlinkPhaseHidden(uint32_t player, uint32_t motion)
{
	auto base = g_memory.base;
	const float remaining = player != 0 ? X86_LOAD_F32(player + 0x27C) : 0.0f;

	// anything that the game would not treat as a valid remaining time should use the original logic
	if (!(remaining > 0.0f) || remaining > 1.0e6f)
		return (X86_LOAD_U32(motion + 0x3EC) & 1) != 0;

	return (static_cast<int>(remaining * 60.0f + 0.25f) & 1) != 0;
}

// ShadowMotion::Exec, the counter above still run and can be used here to patch the timing being wrong at > 60 fps
// False = run original code
bool ShadowMotionBlinkPhaseMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	return DamageBlinkPhaseHidden(X86_LOAD_U32(esi.u32 + 0x58), esi.u32);
}

// Partners have similar blink logic than Shadow
// False = run original code
bool NpcMotionBlinkPhaseMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	return DamageBlinkPhaseHidden(X86_LOAD_U32(esi.u32 + 0x458), esi.u32);
}

/* --Damage blink patch END-- */

// Helper to match exponential smoothing for vehicles
static float ScaleFirstOrderLag(float before, float after, float retention, float frames)
{
	const float span = 1.0f - retention;
	if (!(span > 0.0f))
	{
		return after;
	}
	return before - (before - after) * ((1.0f - std::pow(retention, frames)) / span);
}


/**	--Vehicle::AirSaucer Patch START-- */
// Air Saucer (and affects Walker/WalkerCannon)
// Mostly deltatime safe by default but has a bunch of per-frame ops
//
// Idle velocity damp @ sub_24C190
// Grip lag & rider anchor chase @ sub_24BEC0
// Heading turn @ sub_24C600
//
// Mover measurement scaling per frame suppression @ sub_24B5F0
// Collision advance @ sub_327840 (scoped for sub_24B6F0)
// Sweep slide gate / resting pad @ sub_24F900 (same idea as the fix for CarType)
// Mover frame tick @ sub_24D590

// Idle damp
X86_FUNC_IMPL(__imp__sub_24C190);
X86_FUNC(sub_24C190)
{
	const uint32_t self = ctx.ecx.u32;
	const float velX = X86_LOAD_F32(self + 0x1C);
	const float velY = X86_LOAD_F32(self + 0x20);
	const float velZ = X86_LOAD_F32(self + 0x24);

	__imp__sub_24C190(ctx, base);

	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}
	if (X86_LOAD_F32(self + 0x1C) == velX && X86_LOAD_F32(self + 0x20) == velY && X86_LOAD_F32(self + 0x24) == velZ)
	{
		return;
	}

	// Idle damp per frame while grounded
	const float dampIdle = std::pow(0.7f, frames - 1.0f);
	X86_STORE_F32(self + 0x1C, X86_LOAD_F32(self + 0x1C) * dampIdle);
	X86_STORE_F32(self + 0x20, X86_LOAD_F32(self + 0x20) * dampIdle);
	X86_STORE_F32(self + 0x24, X86_LOAD_F32(self + 0x24) * dampIdle);
}

// Per-frame
// Lag 0x148
// Grip factor 0x154
// Carry the grip lag over the frame, then redo the anchor chase as if 60fps
X86_FUNC_IMPL(__imp__sub_24BEC0);
X86_FUNC(sub_24BEC0)
{
	constexpr uint32_t gripStepAddr = 0x46B380; // 0.1 (per frame)
	constexpr uint32_t gripMaxAddr = 0x471B4C; // 0.96875
	const uint32_t self = ctx.ecx.u32;
	const float gripBefore = X86_LOAD_F32(self + 0x154);

	__imp__sub_24BEC0(ctx, base);

	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const float gripAfter = X86_LOAD_F32(self + 0x154);
	const float grip = ScaleFirstOrderLag(gripBefore, gripAfter, 1.0f - X86_LOAD_F32(gripStepAddr), frames);
	const float gripMax = X86_LOAD_F32(gripMaxAddr);
	const float gripClamped = std::min(std::max(grip, 0.0f), gripMax);
	X86_STORE_F32(self + 0x154, gripClamped);

	// get pre-chase then apply as if 60
	const float appliedFrac = std::min(frames * (1.0f - gripAfter), 1.0f);
	const float anchorAfter[3] =
	{
		X86_LOAD_F32(self + 0x148),
		X86_LOAD_F32(self + 0x14C),
		X86_LOAD_F32(self + 0x150)
	};

	if (appliedFrac < 0.9999f)
	{
		const float target[3] =
		{
			X86_LOAD_F32(self + 4) + X86_LOAD_F32(self + 0x1C) * (1.0f / 128.0f),
			X86_LOAD_F32(self + 8) + 5.0f + X86_LOAD_F32(self + 0x20) * (1.0f / 128.0f),
			X86_LOAD_F32(self + 0xC) + X86_LOAD_F32(self + 0x24) * (1.0f / 128.0f)
		};

		const float remainFrac = 1.0f - appliedFrac;
		if (remainFrac > 0.0001f) // avoid a snap
		{
			const float idealFrac = 1.0f / (1.0f + gripClamped / (frames * (1.0f - gripClamped)));

			for (uint32_t i = 0; i < 3; i++)
			{
				const float anchorBefore = (anchorAfter[i] - appliedFrac * target[i]) / remainFrac;
				X86_STORE_F32(self + 0x148 + i * 4, anchorBefore + (target[i] - anchorBefore) * idealFrac);
			}

			// shift by 2 in either direction
			const float gapY = X86_LOAD_F32(self + 8) + 5.0f + X86_LOAD_F32(self + 0x20) * (1.0f / 128.0f) - X86_LOAD_F32(self + 0x14C);
			X86_STORE_F32(self + 0x14C, X86_LOAD_F32(self + 0x14C) + gapY - std::min(2.0f, std::max(-2.0f, gapY)));
		}
	}
}

static uint32_t g_moverTick = 0;

// Turn tilt snap fix
X86_FUNC_IMPL(__imp__sub_24C600);
X86_FUNC(sub_24C600)
{
	const uint32_t self = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	const float yawBefore = X86_LOAD_F32(self + 0x11C);
	const float grip = X86_LOAD_F32(self + 0x154);
	const float rotSpeed = X86_LOAD_F32(0x471B10);
	const float linearMax = (1.0f - grip) * dt * (rotSpeed * 0.017453294f);

	__imp__sub_24C600(ctx, base);

	if (frames >= 0.99f)
	{
		return;
	}

	const float yawAfter = X86_LOAD_F32(self + 0x11C);
	float step = yawAfter - yawBefore;
	if (step > 3.14159265f)
	{
		step -= 6.28318531f;
	}
	else if (step < -3.14159265f)
	{
		step += 6.28318531f;
	}
	if (step == 0.0f)
	{
		return;
	}
	if (std::fabs(step) >= linearMax * 0.999f) // already dt aligned
	{
		return;
	}
	X86_STORE_F32(self + 0x11C, yawBefore + step * frames);
}

// Fix flat grounded inconsistencies at high fps
X86_FUNC_IMPL(__imp__sub_24B5F0);
X86_FUNC(sub_24B5F0)
{
	static uint32_t s_moverLastFrame = 0xFFFFFFFF;
	static uint32_t s_moverLastSelf = 0;
	static float s_moverKeptVel[3] = { 0, 0, 0 };
	const uint32_t self = ctx.ecx.u32;
	const float velBefore[3] =
	{
		X86_LOAD_F32(self + 0x1C),
		X86_LOAD_F32(self + 0x20),
		X86_LOAD_F32(self + 0x24)
	};

	const float velDeltaSq = (velBefore[0] - s_moverKeptVel[0]) * (velBefore[0] - s_moverKeptVel[0])
		+ (velBefore[1] - s_moverKeptVel[1]) * (velBefore[1] - s_moverKeptVel[1])
		+ (velBefore[2] - s_moverKeptVel[2]) * (velBefore[2] - s_moverKeptVel[2]);
	const bool keptVelStale = !(self == s_moverLastSelf && s_moverLastFrame == g_moverTick) || velDeltaSq > 100.0f; // determine when to accept new

	__imp__sub_24B5F0(ctx, base);

	if (keptVelStale)
	{
		// scale to 60 FPS
		s_moverLastSelf = self;
		s_moverLastFrame = g_moverTick;
		const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

		if (frames < 0.99f)
		{
			// determine if we can skip friction (already works properly with delta)
			const float hSpeedBefore = std::sqrt(velBefore[0] * velBefore[0] + velBefore[2] * velBefore[2]);
			const float velLen = std::sqrt(velBefore[0] * velBefore[0] + velBefore[1] * velBefore[1] + velBefore[2] * velBefore[2]);
			const float velDelta[3] =
			{
				X86_LOAD_F32(self + 0x1C) - velBefore[0],
				X86_LOAD_F32(self + 0x20) - velBefore[1],
				X86_LOAD_F32(self + 0x24) - velBefore[2]
			};
			const bool frictionLike = velLen > 20.0f
			&& std::sqrt(velDelta[0] * velDelta[0] + velDelta[1] * velDelta[1] + velDelta[2] * velDelta[2]) < 0.15f * velLen
			&& (velDelta[0] * velBefore[0] + velDelta[1] * velBefore[1] + velDelta[2] * velBefore[2]) < 0.0f;

			for (int i = 0; i < 3; i++)
			{
				const uint32_t velAddr = self + 0x1C + i * 4;
				const float delta = X86_LOAD_F32(velAddr) - velBefore[i];
				float scale = frames;

				if (frictionLike)
				{
					scale = 1.0f; // wall hit
				}
				else if (i != 1 && hSpeedBefore > 1.0f && std::fabs(delta) > 0.5f * hSpeedBefore)
				{
					scale = 1.0f; // horizontal bounce
				}
				X86_STORE_F32(velAddr, velBefore[i] + delta * scale);
			}
		}
		s_moverKeptVel[0] = X86_LOAD_F32(self + 0x1C);
		s_moverKeptVel[1] = X86_LOAD_F32(self + 0x20);
		s_moverKeptVel[2] = X86_LOAD_F32(self + 0x24);
	}
	else
	{
		// discard and use prior data
		X86_STORE_F32(self + 0x1C, s_moverKeptVel[0]);
		X86_STORE_F32(self + 0x20, s_moverKeptVel[1]);
		X86_STORE_F32(self + 0x24, s_moverKeptVel[2]);
	}
}

// Main collision (generic integrator)
// Function is v' = v*e^(-k*dt) + a/k*(1-e^(-k*dt)
// TODO: Probably will need to extend this for other collision patches
// 0x4 posPtr
// 0x8 velPtr
// 0xC accPtr
// 0x10 k
// 0x14 dt
static int g_inSaucerCollision = 0;
X86_FUNC_IMPL(__imp__sub_327840);
X86_FUNC(sub_327840)
{
	const uint32_t args = ctx.esp.u32 + 4;
	const uint32_t posPtr = X86_LOAD_U32(args);
	const uint32_t velPtr = X86_LOAD_U32(args + 4);
	const float k = X86_LOAD_F32(args + 0x10);
	const float dt = X86_LOAD_F32(args + 0x14);
	const float velBefore[3] =
	{
		X86_LOAD_F32(velPtr),
		X86_LOAD_F32(velPtr + 4),
		X86_LOAD_F32(velPtr + 8)
	};
	const float posBefore[3] =
	{
		X86_LOAD_F32(posPtr),
		X86_LOAD_F32(posPtr + 4),
		X86_LOAD_F32(posPtr + 8)
	};
	const float frames = dt * 60.0f;
	const bool airSaucerHighFPSDetected = g_inSaucerCollision > 0 && k > 0.0f && dt > 0.0f && frames < 0.99f && frames > 0.01f;

	__imp__sub_327840(ctx, base);

	// fix Air Saucer slow crawling
	if (airSaucerHighFPSDetected)
	{
		for (uint32_t i = 0; i < 3; i++)
		{
			const float velAfter = X86_LOAD_F32(velPtr + i * 4);
			const float posAfter = X86_LOAD_F32(posPtr + i * 4);
			const float velStep = velBefore[i] * dt;
			float shrink = 1.0f;
			if (std::fabs(velStep) > 1e-9f)
			{
				shrink = (posAfter - posBefore[i]) / velStep;
				if (!(shrink > 0.0f) || shrink > 1.0f)
				{
					shrink = 1.0f;
				}
			}
			const float shrinkFinal = shrink >= 0.95f ? std::pow(shrink, frames) : shrink;
			X86_STORE_F32(posPtr + i * 4, posBefore[i] + velAfter * dt * shrinkFinal);
		}
	}
}

// Shared collide & slide, used by AirSaucer, Walker, WalkerCannon (aka GUN Jump Mechs)
// Modify before call to sub_24E780
X86_FUNC_IMPL(__imp__sub_24F900);
X86_FUNC(sub_24F900)
{
	const float dt = X86_LOAD_F32(g_deltaTime);
	float frames = dt * 60.0f;
	if (!(frames > 0.0f))
	{
		frames = 1.0f;
	}
	frames = std::min(frames, 1.0f);
	frames = std::max(frames, 0.05f);

	constexpr uint32_t slideGateAddr = 0x42A18C;
	constexpr float slideGateAt60 = 0.001f;
	constexpr uint32_t saucerPadAddr = 0x51D3F4;
	constexpr float saucerPadAt60 = 0.02f;
	const uint32_t self = ctx.ecx.u32;
	const uint32_t posAddr = X86_LOAD_U32(ctx.esp.u32 + 4);
	const uint32_t dispAddr = X86_LOAD_U32(ctx.esp.u32 + 8);

	// struct to track and re-attach to ground (higher fps = more precise collision = drops the pin = GG, fall through)
	// this does cause a tiny bit of drift but that is a result of the collision step
	static struct SaucerChase
	{
		uint32_t self;
		uint32_t pinned;
		float sink;
		float kick;
		float ride;
		float slack;
		float airTime;
		uint32_t lastTick;

		void reset()
		{
			pinned = 0;
			sink = 0.0f;
			kick = 0.0f;
			ride = 0.0f;
			slack = 0.0f;
		}
	} s_chase[16];
	static int s_chaseCount = 0;

	int slot = -1;
	if (frames < 0.9999f)
	{
		for (int i = 0; i < s_chaseCount; i++)
		{
			if (s_chase[i].self == self)
			{
				slot = i;
				break;
			}
		}
		if (slot < 0 && s_chaseCount < 16)
		{
			slot = s_chaseCount++;
			s_chase[slot].self = self;
			s_chase[slot].reset();
			s_chase[slot].airTime = 0.0f;
			s_chase[slot].lastTick = 0;
		}
	}

	if (slot >= 0)
	{
		const bool grounded = (X86_LOAD_U32(self + 0x1C) & 1u) != 0;
		if (grounded)
		{
			s_chase[slot].airTime = 0.0f;
		}
		else if (s_chase[slot].lastTick != g_moverTick)
		{
			s_chase[slot].airTime += dt;
		}
		s_chase[slot].lastTick = g_moverTick;

		// Double jump fix
		if (!grounded && s_chase[slot].airTime <= dt * 1.5f)
		{
			const float reqX = X86_LOAD_F32(dispAddr), reqZ = X86_LOAD_F32(dispAddr + 8);
			const uint32_t saucer = posAddr - 4;
			const float velX = X86_LOAD_F32(saucer + 0x1C);
			const float velZ = X86_LOAD_F32(saucer + 0x24);

			if (std::fabs(reqX) + std::fabs(reqZ) < 0.005f && std::fabs(velX) + std::fabs(velZ) > 15.0f)
			{
				X86_STORE_F32(dispAddr, velX * dt);
				X86_STORE_F32(dispAddr + 8, velZ * dt);
			}
		}
	}

	const float reqY = X86_LOAD_F32(dispAddr + 4);
	const float posYBefore = X86_LOAD_F32(posAddr + 4);
	bool chased = false;
	float chaseExtY = 0.0f;
	if (slot >= 0 && s_chase[slot].pinned != 0 && reqY < 0.0f)
	{
		// extended cast down to handle more precise coli
		float extY;

		if (s_chase[slot].kick > 0.001f)
		{
			extY = -(s_chase[slot].kick + 0.045f + 2.0f * s_chase[slot].sink);
		}
		else
		{
			extY = -frames * std::min(0.5f * 1301.0f / 3600.0f, 0.02f + 2.0f * s_chase[slot].sink + 0.025f);
		}

		// slow speed
		const float dispXZ = std::sqrt(X86_LOAD_F32(dispAddr) * X86_LOAD_F32(dispAddr) + X86_LOAD_F32(dispAddr + 8) * X86_LOAD_F32(dispAddr + 8));
		if (dt > 0.0f && dispXZ / dt < 250.0f)
		{
			const float pinStep = frames * 0.5f * 1301.0f / 3600.0f;
			extY = std::min(extY, -pinStep);
		}

		if (reqY > extY)
		{
			chased = true;
			chaseExtY = extY;
			X86_STORE_F32(dispAddr + 4, extY);
		}
	}

	const float restoreGate = X86_LOAD_F32(slideGateAddr);
	const float restorePad = X86_LOAD_F32(saucerPadAddr);

	X86_STORE_F32(slideGateAddr, slideGateAt60 * frames * frames * frames * frames);
	X86_STORE_F32(saucerPadAddr, saucerPadAt60 * frames * frames);

	__imp__sub_24F900(ctx, base);

	X86_STORE_F32(slideGateAddr, restoreGate);
	X86_STORE_F32(saucerPadAddr, restorePad);

	const float appliedY = X86_LOAD_F32(posAddr + 4) - posYBefore;
	if (chased && appliedY <= chaseExtY + 0.003f)
	{
		// handle no floor bug (collision miss, and jump being consumed bug)
		const float naturalDrop = -reqY;
		const float riding = s_chase[slot].ride;
		const float drop = riding > naturalDrop ? riding : naturalDrop;

		if (riding > 0.008f && s_chase[slot].slack > 0.0f)
		{
			s_chase[slot].slack -= drop;

			X86_STORE_F32(posAddr + 4, posYBefore - drop);
			X86_STORE_F32(dispAddr + 4, -drop);

			// force grounded state
			X86_STORE_U32(self + 0x1C, X86_LOAD_U32(self + 0x1C) | 1u);
		}
		else
		{
			X86_STORE_F32(posAddr + 4, posYBefore + reqY);
			X86_STORE_F32(dispAddr + 4, reqY);
		}
	}

	if (slot >= 0)
	{
		if (reqY > 0.000001f)
		{
			s_chase[slot].reset();
		}
		else
		{
			if ((X86_LOAD_U32(self + 0x1C) & 1) != 0)
			{
				s_chase[slot].sink = 0.0f;
			}

			if (reqY < -0.000001f)
			{
				// ride the floor at the rate it recedes with decay
				s_chase[slot].ride = std::max(-reqY, s_chase[slot].ride * 0.85f);

				if (appliedY > (chased ? chaseExtY : reqY) + 0.000001f)
				{
					// prevent sticking
					s_chase[slot].pinned = 1;
					s_chase[slot].sink = 0.0f;
					s_chase[slot].kick = appliedY > 0.01f ? appliedY : 0.0f;
					s_chase[slot].slack = 0.5f;
				}
				else
				{
					if (chased && s_chase[slot].slack > 0.0f)
					{
						s_chase[slot].sink = 0.0f;
					}
					else
					{
						s_chase[slot].sink += -reqY;
					}

					// ignore pin if sink is way off
					if (s_chase[slot].sink > 0.05f + s_chase[slot].kick)
					{
						s_chase[slot].reset();
					}
				}
			}
		}
	}
}


X86_FUNC_IMPL(__imp__sub_24B6F0);
X86_FUNC(sub_24B6F0)
{
	++g_inSaucerCollision;
	__imp__sub_24B6F0(ctx, base);
	--g_inSaucerCollision;
}

X86_FUNC_IMPL(__imp__sub_24D590);
X86_FUNC(sub_24D590)
{
	++g_moverTick;
	__imp__sub_24D590(ctx, base);
}
/**	--Vehicle::AirSaucer Patch END-- */

/**	--Vehicle::TVehicleCar Patch START-- */
// TVehicleCar / CarType vehicles.
// Affects 'Norael' (yes that is not a typo lol) aka Normal Car [Jeep], Open Car [Sports Car], Armed Car [Armored GUN Car], Bike [GUN Motorcycle]
// This obj has a lot of weird "almost using deltatime" going on, so this patch is a bit of a mess.
// Issues are mostly exponential smoothing related
// It already uses gravity and steps properly, but there's a bunch of per-frame adjustments happening.

// Input low-pass @ sub_243BB0
// Axle velocity blend @ sub_246120 (tail)
// In-air velocity projection @ sub_245FB0 (tail)
// Sweep slide gate / resting pad @ sub_244CD0, fixed in sub_245FB0
// coast dead-stop that falls out of the gate @ sub_245FB0

// The byte flags record which held/released K the game used, so the lag can be carried.
struct VehicleInputChannel
{
	uint32_t value;
	uint32_t flag;
	uint32_t heldK;
	uint32_t releasedK;
};

static const VehicleInputChannel g_vehicleInputChannels[] =
{
	{ 0x58, 0x00, 0x51CC1C, 0x51CC1C }, // steering
	{ 0x64, 0x5E, 0x51CC20, 0x51CC24 }, // accelerate
	{ 0x68, 0x5F, 0x51CC30, 0x51CC34 }, // trigger
	{ 0x6C, 0x60, 0x51CC28, 0x51CC2C }, // brake / reverse
};

X86_FUNC_IMPL(__imp__sub_243BB0);
X86_FUNC(sub_243BB0)
{
	const uint32_t self = ctx.ecx.u32;
	float before[std::size(g_vehicleInputChannels)];

	for (size_t i = 0; i < std::size(g_vehicleInputChannels); i++)
	{
		before[i] = X86_LOAD_F32(self + g_vehicleInputChannels[i].value);
	}

	__imp__sub_243BB0(ctx, base);

	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	for (size_t i = 0; i < std::size(g_vehicleInputChannels); i++)
	{
		const VehicleInputChannel& channel = g_vehicleInputChannels[i];
		const bool held = channel.flag == 0 || X86_LOAD_U8(self + channel.flag) != 0;
		const float k = X86_LOAD_F32(held ? channel.heldK : channel.releasedK);
		if (!(k > 0.0f))
		{
			continue;
		}

		const uint32_t addr = self + channel.value;
		X86_STORE_F32(addr, ScaleFirstOrderLag(before[i], X86_LOAD_F32(addr), k / (k + 1.0f), frames));
	}
}

// One wheel step
// config[14] (-9) is a flat ground-adhesion bias. Only applies while airborne
// Per-wheel sweep request
static float g_vehicleReq[2][3];

// sub_246120
// Fix not being able to move at higher fps by removing the per frame blend, then re-applying as if running at 60fps
// Also fix in-air movement
static void VehicleRelaxAxleVelocity(uint8_t* base, uint32_t axle, uint32_t prevPos, float invDt,
                                     float retention, float adhesion, float frames, const float* req)
{
	if (adhesion != 0.0f)
	{
		const float dt = 1.0f / invDt;
		constexpr float dt60 = 1.0f / 60.0f;
		const float fixH = 3.75f * (dt60 - dt) * 0.5f;
		const float fixY = 0.6666667f * (dt60 - dt) * 0.5f;
		float v0[3];
		float m[3];
		float m60[3];

		for (uint32_t i = 0; i < 3; i++)
		{
			// remove the per frame blend (prevPos)
			m[i] = (X86_LOAD_F32(axle + i * 4) - X86_LOAD_F32(prevPos + i * 4)) * invDt;
			const float blended = X86_LOAD_F32(axle + 12 + i * 4);
			v0[i] = blended + blended - m[i];

			// Trapezoid fix vertical: v - a/k, a/k = -190.5 [note: this is kind of a crap workaround, maybe should use base gravity]
			const float fix = i == 1 ? (v0[1] + 190.5f) * fixY : v0[i] * fixH;
			m60[i] = m[i] + fix;
		}

		const float axis[3] = { req[0], req[1] + adhesion * dt, req[2] };
		const float len = std::sqrt(axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2]);

		if (len > 0.000001f)
		{
			const float dot = (v0[0] * axis[0] + v0[1] * axis[1] + v0[2] * axis[2]) / len;

			for (uint32_t i = 0; i < 3; i++)
			{
				const float v60 = (axis[i] / len * dot + m60[i]) * 0.5f;
				X86_STORE_F32(axle + 12 + i * 4, v0[i] + frames * (v60 - v0[i]));
			}
		}
		return;
	}

	for (uint32_t i = 0; i < 12; i += 4)
	{
		const float measured = (X86_LOAD_F32(axle + i) - X86_LOAD_F32(prevPos + i)) * invDt;
		const float blended = X86_LOAD_F32(axle + 12 + i);
		const float previous = blended + blended - measured;
		X86_STORE_F32(axle + 12 + i, measured + (previous - measured) * retention);
	}
}

X86_FUNC_IMPL(__imp__sub_246120);
X86_FUNC(sub_246120)
{
	const uint32_t self = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);

	__imp__sub_246120(ctx, base);

	const float frames = dt * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const float retention = std::pow(0.5f, frames);
	const float invDt = 1.0f / dt;

	const uint32_t config = 0x51CCD8 + 0xAC * X86_LOAD_U32(self + 604);
	const float adhesion = X86_LOAD_F32(config + 0x38);

	const float frontAir = (X86_LOAD_U8(self + 220) & 1) == 0 ? adhesion : 0.0f;
	const float rearAir = (X86_LOAD_U8(self + 272) & 1) == 0 ? adhesion : 0.0f;

	VehicleRelaxAxleVelocity(base, self + 184, self + 468, invDt, retention, frontAir, frames, g_vehicleReq[0]);
	VehicleRelaxAxleVelocity(base, self + 236, self + 520, invDt, retention, rearAir, frames, g_vehicleReq[1]);
}

// Helper to scope our angle fix just to TVehicle
static bool VehicleShouldScaleAngleLag(uint32_t returnAddress)
{
	switch (returnAddress)
	{
	case 0x24665A: // CarType chassis roll damper @ 0x246655
	case 0x2469A7: // CarType body pitch (0xAC) @ 0x2469A2
	case 0x2469D0: // CarType body roll (0xB4) @ 0x2469CB
	case 0x246AA0: // CarType steer lean (0x4C) @ 0x246A9B
		return true;
	default:
		return false;
	}
}

// Shared angle smoother (TVehicle and other unknown objects use it)
X86_FUNC_IMPL(__imp__sub_1D860);
X86_FUNC(sub_1D860)
{
	if (VehicleShouldScaleAngleLag(X86_LOAD_U32(ctx.esp.u32)))
	{
		const uint32_t rateAddr = ctx.esp.u32 + 0xC;
		const float a = X86_LOAD_F32(rateAddr);
		const float b = X86_LOAD_F32(ctx.esp.u32 + 0x10);
		const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

		// Handle the pow that scales with higher framerate and prevent divide by 0 or endless low speed
		if (frames > 0.0f && a > 0.0f && b > 0.0f && std::fabs(frames - 1.0f) >= 0.0001f)
		{
			const float retention = std::pow(a / (a + b), frames);
			const float span = 1.0f - retention;
			X86_STORE_F32(rateAddr, span > 0.0001f ? b * (retention / span) : b * 10000.0f);
		}
	}
	__imp__sub_1D860(ctx, base);
}

// Per-frame sweep fix
// Swap below globals in sub_245FB0 only. If we ignore it constant missing, if we overcorrect it makes slopes crawl
// 0x42A18C (0.001) 'minimum' pad/slider
// 0x51CCD4 (0.02) 'pad' for ground
static const uint32_t g_carPadAddr = 0x51CCD4;
static const float g_carPadAt60 = 0.02f;
X86_FUNC_IMPL(__imp__sub_245FB0);
X86_FUNC(sub_245FB0)
{
	const uint32_t self = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	const uint32_t axle = X86_LOAD_U32(ctx.esp.u32 + 8);

	constexpr uint32_t slideGateAddr = 0x42A18C;
	constexpr float slideGateAt60 = 0.001f;

	float frames = dt * 60.0f;
	if (!(frames > 0.0f))
	{
		frames = 1.0f;
	}
	frames = std::min(frames, 1.0f);
	frames = std::max(frames, 0.05f);

	// Remove the blend and let VehicleRelaxAxleVelocity handle it
	const float rawFrames = dt * 60.0f;
	const bool correctProjection = rawFrames > 0.0f && std::fabs(rawFrames - 1.0f) >= 0.0001f;

	float velPre[3];
	for (uint32_t i = 0; i < 3; i++)
	{
		velPre[i] = X86_LOAD_F32(axle + 12 + i * 4);
	}
	{
		// Coasting fix (ignored while accelerating)
		const uint32_t prevPos = X86_LOAD_U32(ctx.esp.u32 + 0xC);
		float* const req = g_vehicleReq[axle == self + 236 ? 1 : 0];
		if (correctProjection &&
			(X86_LOAD_U8(axle + 0x24) & 1) != 0 &&
			std::fabs(X86_LOAD_F32(self + 100)) < 0.02f &&
			std::fabs(X86_LOAD_F32(self + 108)) < 0.02f)
		{
			const float hSq = (X86_LOAD_F32(axle) - X86_LOAD_F32(prevPos)) *
				(X86_LOAD_F32(axle) - X86_LOAD_F32(prevPos)) +
				(X86_LOAD_F32(axle + 8) - X86_LOAD_F32(prevPos + 8)) *
				(X86_LOAD_F32(axle + 8) - X86_LOAD_F32(prevPos + 8));
			const float freeze = std::sqrt(slideGateAt60);
			if (hSq < freeze * freeze)
			{
				X86_STORE_F32(axle, X86_LOAD_F32(prevPos));
				X86_STORE_F32(axle + 8, X86_LOAD_F32(prevPos + 8));
			}
		}
		for (uint32_t i = 0; i < 3; i++)
		{
			req[i] = X86_LOAD_F32(axle + i * 4) - X86_LOAD_F32(prevPos + i * 4);
		}
	}
	const float restoreGate = X86_LOAD_F32(slideGateAddr);
	const float restorePad = X86_LOAD_F32(g_carPadAddr);

	const float gate = slideGateAt60 * frames * frames * frames * frames;
	const float pad = g_carPadAt60 * frames;

	X86_STORE_F32(slideGateAddr, gate);
	X86_STORE_F32(g_carPadAddr, pad);

	__imp__sub_245FB0(ctx, base);

	X86_STORE_F32(slideGateAddr, restoreGate);
	X86_STORE_F32(g_carPadAddr, restorePad);

	if (correctProjection && (X86_LOAD_U8(axle + 0x24) & 1) == 0)
	{
		// Restore the original data for wheels in-air
		for (uint32_t i = 0; i < 3; i++)
		{
			X86_STORE_F32(axle + 12 + i * 4, velPre[i]);
		}
	}
}

/**	--Vehicle::TVehicleCar Patch END-- */

/**	--Library Cursor patch START-- */

 // There is an issue with the cursor of the library menu at higher fps.
 // When player held the stick, it scrolls down way too fast
 // One press make it goes 4-5 items further instead of 1 like it should.
 // This is due to a counter that use raw frames to progress.
 // We can patch this by forcing the counter to stick at 60 hz using delta time
 // and let a repeat pass only on the frame where a counter actually moved.


constexpr uint32_t g_menuGroupMasks = 0x47B698; // 8 directions group masks

// 60 Hz leftovers per pad and group, as well as the groups that ticked this frame.
// Technically only player 1 can control menu, but maybe one day we will have a way to do more so...
static float g_menuHoldCarry[4][8];
static uint8_t g_menuHoldTicked[4];

// The pad task, runs once a frame. Redo its counter bumps in 60 Hz steps.
X86_FUNC_IMPL(__imp__sub_2A7700);
X86_FUNC(sub_2A7700)
{
	const uint32_t pad = ctx.ecx.u32;
	uint16_t beforeState[4][8];

	if (pad == 0)
	{
		__imp__sub_2A7700(ctx, base);
		return;
	}

	for (uint32_t slot = 0; slot < 4; slot++)
	{
		for (uint32_t group = 0; group < 8; group++)
			beforeState[slot][group] = X86_LOAD_U16(pad + 16 * slot + 2 * group);
	}

	__imp__sub_2A7700(ctx, base);

	float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	if (frames <= 0.0f)
		frames = 1.0f;

	for (uint32_t slot = 0; slot < 4; slot++)
	{
		uint8_t ticked = 0;

		for (uint32_t group = 0; group < 8; group++)
		{
			const uint32_t addr = pad + 16 * slot + 2 * group;
			const uint16_t held = beforeState[slot][group];
			const uint16_t pressed = X86_LOAD_U16(addr);
			float &carry = g_menuHoldCarry[slot][group];

			if (pressed == 0)
			{
				carry = 0.0f; // give up if the game already reset the counter
				continue;
			}

			if (pressed != (uint16_t)(held + 1))
				continue; // pad missing this frame, the game left the counter alone

			if (held == 0)
			{
				// first frame of the hold, keep the game's own 1
				carry = 0.0f;
				ticked |= 1 << group;
				continue;
			}

			carry += frames;

			// keeps 60 fps on exactly one step per frame
			const int steps = (int)(carry + 0.0001f);
			carry -= steps;

			if (steps > 0)
				ticked |= 1 << group;

			int value = held + steps;

			if (value > 0x7000)
				value = 0x7000; // never wrap

			X86_STORE_U16(addr, (uint16_t)value);
		}

		g_menuHoldTicked[slot] = ticked;
	}
}

// small recreation of sub_59F70 for convenience
static bool MenuKeyPressed(uint8_t* base, uint32_t pad, uint32_t slot, uint32_t mask)
{
	const uint32_t pressed = X86_LOAD_U32(pad + 40 * slot + 88);

	if ((mask & pressed) == mask)
		return true;

	if ((mask & 0x3C0) != 0 && (mask & 0x3C0 & pressed) == (mask & 0x3C0))
		return true;

	if ((mask & 0x3C0000) != 0 && (mask & 0x3C0000 & pressed) == (mask & 0x3C0000))
		return true;

	return false;
}


static bool MenuHoldTicked(uint8_t* base, uint32_t slot, uint32_t mask)
{
	const uint8_t ticked = g_menuHoldTicked[slot];

	for (uint32_t group = 0; group < 8; group++)
	{
		// did one of the groups fired a tick on the 60 Hz clock
		if ((ticked & (1 << group)) != 0 && (mask & X86_LOAD_U32(g_menuGroupMasks + group * 4)) != 0)
			return true;
	}

	return false;
}

// Function that seems to handle the scrolling in the library based on input held
// We can let the og code run for the math, we simply need to prevent it to continue scrolling if we reach extra frames.

X86_FUNC_IMPL(__imp__sub_2A7560);
X86_FUNC(sub_2A7560)
{
	const uint32_t pad = ctx.ecx.u32;
	uint32_t slot = X86_LOAD_U32(ctx.esp.u32 + 4);
	const uint32_t mask = X86_LOAD_U32(ctx.esp.u32 + 8);

	__imp__sub_2A7560(ctx, base);

	if (ctx.eax.u8_lo == 0 || pad == 0)
		return;

	// 6 and 7 means "the pad this menu owns"
	if (slot == 6)
		slot = X86_LOAD_U32(pad + 404);
	else if (slot == 7)
		slot = X86_LOAD_U32(pad + 408);

	if (slot < 4)
	{
		if (MenuKeyPressed(base, pad, slot, mask) || MenuHoldTicked(base, slot, mask))
			return;
	}
	else
	{
		for (uint32_t p = 0; p < 4; p++)
		{
			if (MenuKeyPressed(base, pad, p, mask) || MenuHoldTicked(base, p, mask))
				return;
		}
	}

	ctx.eax.u8_lo = 0; // this is a repeat from a duplicated frame, give up.
}

// The Library route list also add its own cooldown on top of all of this:
// after a single cursor move, it counts 2 frames before it starts to listen again for more.
// Those two frames increment without delta time, which will also affect cursor speed in the end.

X86_FUNC_IMPL(__imp__sub_291B70);
X86_FUNC(sub_291B70)
{
	const uint32_t self = ctx.ecx.u32;
	const uint32_t before = self ? X86_LOAD_U32(self + 284) : 0;

	__imp__sub_291B70(ctx, base);

	if (self == 0)
		return;

	const uint32_t after = X86_LOAD_U32(self + 284);

	if (after <= before)
		return; // no cursor move this frame

	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	if (frames <= 0.0f || frames >= 1.0f)
		return;

	uint32_t scaled = (uint32_t)(after / frames + 0.5f);
	if (scaled > 64)
		scaled = 64;

	X86_STORE_U32(self + 284, scaled);
}

/**	--Library Cursor patch END-- */

/** --Catapult (aka Prison Island launch pads) Patch START-- */
static float g_catapultTime = 0.0f;

// Catapult CharCommand Handler
X86_FUNC_IMPL(__imp__sub_1DCCA0);
X86_FUNC(sub_1DCCA0)
{
	const uint32_t self = ctx.ecx.u32;
	const uint32_t cmd = X86_LOAD_U32(ctx.esp.u32 + 4);
	// In the past ran into junk relating to Vehicle dismount -> on Catapult on GC, so just incase, safebounds:
	const int cmdId = (cmd >= 0x10000 && cmd < 0x8000000) ? (int)(int16_t)X86_LOAD_U32(cmd + 4) : -1;

	// tracking of catapult objects (even in 4P, should never exceed this many at once)
	static constexpr int MaxCatapults = 16;
	static uint32_t s_catapultPtrs[MaxCatapults] = {};
	static float s_lastBoardTime[MaxCatapults] = {};
	int slot = -1;
	for (int i = 0; i < MaxCatapults; i++)
	{
		if (s_catapultPtrs[i] == self)
		{
			slot = i;
			break;
		}
		if (slot < 0 && s_catapultPtrs[i] == 0)
		{
			slot = i;
		}
	}
	if (slot < 0)
	{
		slot = 0;
	}
	if (s_catapultPtrs[slot] != self)
	{
		s_catapultPtrs[slot] = self;
		s_lastBoardTime[slot] = -1000.0f;
	}

	const float dt = X86_LOAD_F32(g_deltaTime);
	if (dt > 0.0f && dt * 60.0f < 0.99f && cmdId == 12 && g_catapultTime - s_lastBoardTime[slot] < 0.40f)
	{
		const uint32_t stSave = X86_LOAD_U32(self + 0x64);
		const uint32_t flSave = X86_LOAD_U32(self + 0x6C);
		X86_STORE_U32(self + 0x64, 0);

		__imp__sub_1DCCA0(ctx, base);

		X86_STORE_U32(self + 0x64, stSave);
		X86_STORE_U32(self + 0x6C, flSave);
		return;
	}

	if (cmdId == 11)
	{
		s_lastBoardTime[slot] = g_catapultTime;
	}

	__imp__sub_1DCCA0(ctx, base);
}

// Catapult per-frame update
X86_FUNC_IMPL(__imp__sub_1DD140);
X86_FUNC(sub_1DD140)
{
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	g_catapultTime += dt;
	__imp__sub_1DD140(ctx, base);
}
/** --Catapult (aka Prison Island launch pads) Patch END-- */

/** --Jump Board Patch START-- */

// Object that launch Shadow far away (far farrrrrr... away!)
// Player collision is a swept move, it takes a line from where Shadow was to where he wants to be and
// checks it consistently. The problem is, above 60 FPS, that line is too short and stays inside the ramp, 
// so collision consistently pushes Shadow creating some kind of "bump" effect.
// This can kill player under specific situation (like in space gadget). 
// We can do a hacky workaround by adding a grace method where collision will be ignored for the first 1/60s of launch, then we can call 
// the original function once to get the proper destination (like if the game was running at 60 fps).
// Would be nice to have a better way to fix this, but this is the only idea I had for now.

static constexpr float g_launchGracePeriod = 1.0f / 60.0f;

struct LaunchGrace
{
	uint32_t player;
	float elapsed;
	float sweepOrigin[3]; // (player+0xD0, which is basically old position)
	bool originValid;
	bool replayed;
	int skipped;
};

static LaunchGrace g_launchGrace[4] = {};

static LaunchGrace* FindLaunchGrace(uint32_t player)
{
	if (player == NULL)
		return nullptr;

	for (auto& slot : g_launchGrace)
	{
		if (slot.player == player)
			return &slot;
	}
	return nullptr;
}

// This is one of the function used to detect collision 
// We can trick the game that no collision were found for the first 1/60 of a second so we can progress
// Then we run the original code once to store the intended destination
// Hacky, but this is good enough for now.

X86_FUNC_IMPL(__imp__sub_84E50);
X86_FUNC(sub_84E50)
{
	const uint32_t playerData = X86_LOAD_U32(ctx.esp.u32 + 4);

	if (playerData != NULL)
	{
		const uint32_t player = X86_LOAD_U32(playerData);
		LaunchGrace* grace = FindLaunchGrace(player);

		if (grace != nullptr)
		{
			// First pass of this launch will tell us where 60 FPS would start its sweep
			if (!grace->originValid)
			{
				grace->sweepOrigin[0] = X86_LOAD_F32(player + 0xD0);
				grace->sweepOrigin[1] = X86_LOAD_F32(player + 0xD4);
				grace->sweepOrigin[2] = X86_LOAD_F32(player + 0xD8);
				grace->originValid = true;
			}

			if (grace->elapsed < g_launchGracePeriod - 1.0e-6f)
			{
				grace->skipped++;
				ctx.eax.u32 = 0;
				ctx.AddEsp(1);
				return;
			}

			// Grace just finished and we held passes back, so replay the one pass 60 FPS
			// would have give us originally. 
			if (grace->skipped > 0 && !grace->replayed)
			{
				grace->replayed = true;

				const float saved[3] =
				{
					X86_LOAD_F32(player + 0xD0),
					X86_LOAD_F32(player + 0xD4),
					X86_LOAD_F32(player + 0xD8)
				};

				X86_STORE_F32(player + 0xD0, grace->sweepOrigin[0]);
				X86_STORE_F32(player + 0xD4, grace->sweepOrigin[1]);
				X86_STORE_F32(player + 0xD8, grace->sweepOrigin[2]);

				g_launchCollisionPass++;
				__imp__sub_84E50(ctx, base);
				g_launchCollisionPass--;

				// restore value (game should overwrite this right after, but just to be safe...)
				X86_STORE_F32(player + 0xD0, saved[0]);
				X86_STORE_F32(player + 0xD4, saved[1]);
				X86_STORE_F32(player + 0xD8, saved[2]);
				return;
			}
		}
	}

	__imp__sub_84E50(ctx, base);
}

// Player::Behavior::Launch::Start
X86_FUNC_IMPL(__imp__sub_76DE0);
X86_FUNC(sub_76DE0)
{
	const uint32_t player = X86_LOAD_U32(ctx.ecx.u32 + 4);

	LaunchGrace* grace = FindLaunchGrace(player);

	if (grace == nullptr)
		grace = &g_launchGrace[0]; // either player is not found or all players are using it, force player 1 slot as a failsafe.

	grace->player = player;
	grace->elapsed = 0.0f;
	grace->originValid = false;
	grace->replayed = false;
	grace->skipped = 0;

	__imp__sub_76DE0(ctx, base);
}

// Player::Behavior::Launch::Exec
// Delta time is added before the original runs, so at 60 FPS the grace period is already spent
// by the time collision is reached and the skip above can never fire.
X86_FUNC_IMPL(__imp__sub_76B20);
X86_FUNC(sub_76B20)
{
	LaunchGrace* grace = FindLaunchGrace(X86_LOAD_U32(ctx.ecx.u32 + 4));

	if (grace != nullptr)
	{
		const float deltatime = X86_LOAD_F32(ctx.esp.u32 + 4);

		if (deltatime > 0.0f)
			grace->elapsed += deltatime;
		else
			grace->elapsed = g_launchGracePeriod; 
	}

	__imp__sub_76B20(ctx, base);
}

// Player::Behavior::Launch::End
X86_FUNC_IMPL(__imp__sub_76A70);
X86_FUNC(sub_76A70)
{
	LaunchGrace* grace = FindLaunchGrace(X86_LOAD_U32(ctx.ecx.u32 + 4));

	if (grace != nullptr)
	{
		grace->player = 0;
		grace->elapsed = 0.0f;
		grace->originValid = false;
		grace->replayed = false;
		grace->skipped = 0;
	}

	__imp__sub_76A70(ctx, base);
}

/**	--Jump Board Patch END-- */

// GUNLiftLocal / GUNLiftExpress aim speed
// ecx = vehicle
// 0x160 = vertical angle
// 0x164 = horizontal angle
// esi = command (aim / move command)
// 0x14 = stickX
// 0x1C = stickY
bool GunLiftAimMidAsmHook(X86Register& ecx, X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	// 60 FPS
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return false;
	}

	// 61+ FPS
	const uint32_t vehicle = ecx.u32;
	const uint32_t command = esi.u32;
	const float deg2rad = X86_LOAD_F32(0x47E0A4) * X86_LOAD_F32(0x42A880); // pi * 1/180
	const float delta = deg2rad * X86_LOAD_F32(0x46D830) * frames; // 1.5 GUNLift sensitivity value
	const float pitch = X86_LOAD_F32(vehicle + 0x160) + X86_LOAD_F32(command + 0x1C) * delta;
	X86_STORE_F32(vehicle + 0x164, X86_LOAD_F32(vehicle + 0x164) - X86_LOAD_F32(command + 0x14) * delta);

	const float minPitch = deg2rad * X86_LOAD_F32(0x46D828); // GUNLift min angle -10
	const float maxPitch = deg2rad * X86_LOAD_F32(0x46D82C); // GUNLift max angle 60
	X86_STORE_F32(vehicle + 0x160, std::min(std::max(pitch, minPitch), maxPitch));

	return true;
}

/** --SetWatchCamera Patch START-- */

// Exec @ 0x21C4F0 respects deltatime
//
// Subfuncs that do not (per frame instead)

// Idle behavior @ 0x21B670
void SetWatchCameraSightChaseMidAsmHook(X86Register& esi, X86Register& esp)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const uint32_t watchCamera = esi.u32;
	const uint32_t deltaAddr = esp.u32 + 0x14;

	const float ramp = std::min(X86_LOAD_F32(watchCamera + 0xC4) + 0.01f * frames, 1.0f);
	X86_STORE_F32(watchCamera + 0xC4, std::min(ramp - 0.01f, 1.0f));

	if (ramp > 0.0001f)
	{
		const float scale = (1.0f - std::pow(1.0f - ramp, frames)) / ramp;
		for (uint32_t i = 0; i < 3; i++)
		{
			X86_STORE_F32(deltaAddr + i * 4, X86_LOAD_F32(deltaAddr + i * 4) * scale);
		}
	}
}

// Position update @ 0x21BCF0
void SetWatchCameraSightMoveMidAsmHook(X86Register& edi)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const uint32_t watchCamera = edi.u32;
	for (uint32_t i = 0; i < 3; i++)
	{
		X86_STORE_F32(watchCamera + 0xE0 + i * 4, X86_LOAD_F32(watchCamera + 0xE0 + i * 4) + X86_LOAD_F32(watchCamera + 0xC8 + i * 4) * (frames - 1.0f));
	}
}

/** --SetWatchCamera Patch END-- */

// Vine::FireBall::Exec @ 0x1E3540
// AKA "Killer Plant"
void VineFireBallTravelMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const uint32_t vine = esi.u32;

	if (X86_LOAD_U32(vine + 0x60) != 1) // state check for fireball
	{
		return;
	}

	const uint32_t fireballPosition = X86_LOAD_U32(vine + 0x48) + 4;
	for (uint32_t i = 0; i < 3; i++)
	{
		X86_STORE_F32(fireballPosition + i * 4, X86_LOAD_F32(fireballPosition + i * 4) + X86_LOAD_F32(vine + 0x38 + i * 4) * (frames - 1.0f));
	}
}

/** --ScatteringWeapon weapon hang fix START-- */

// Shadow Boxes create ScatteringWeapon
// Higher FPS = higher chance for ray to hit while still mid-air. Funny enough early versions of Shadow have this
// issue where the weapons collide with each other. Katana02 tends to get stuck the most at 240FPS
// Threshold value is @ 0x2493B0
// Restore for other funcs, as this may be a shared constant
static const uint32_t g_weaponDropSettleThreshold = 0x46B380; // 0.1 (original threshold)
void WeaponDropSettleThresholdMidAsmHook()
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}
	X86_STORE_F32(g_weaponDropSettleThreshold, 0.1f * frames);
}

void WeaponDropSettleThresholdRestoreMidAsmHook()
{
	auto base = g_memory.base;
	X86_STORE_F32(g_weaponDropSettleThreshold, 0.1f);
}

/** --ScatteringWeapon landing Patch END-- */

/** --Belt::Effect (HomingAttack/TriJump/JumpDash/Slide/Spin trails) Patch START-- */

std::unordered_map<uint32_t, float> g_beltTrailCarry; // map per BeltTask to support multiplayer
// Belt::Exec
bool BeltTrailPushMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return true;
	}

	float &carry = g_beltTrailCarry[esi.u32];
	carry += frames;
	if (carry >= 1.0f)
	{
		carry -= (float)(int)carry; // floor to int first
		return true;
	}
	return false;
}

// Eff::Belt update @ 0x260670
// Fade call @ 0x260716
// Per frame fade of '8'
void BeltTrailFadeMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		return;
	}

	const int extra = std::max((int)(8.0f * (frames - 1.0f) + 0.5f), 0);
	if (extra == 0)
	{
		return;
	}

	const uint32_t beltEff = esi.u32;
	const int value = (int)X86_LOAD_U8(beltEff + 8) - extra;
	X86_STORE_U8(beltEff + 8, (uint8_t)std::max(value, 0));
}

/** --Belt::Effect Patch END-- */

/** --Destructible/Break object pieces Patch START-- */

// Gadgets destroy and create several TObjEffCrash3DChildObj and TObjEffCrash3DRChildObj
// Patch to do 60FPS step, similar to ScatterWeapon fix
static std::unordered_map<uint32_t, float> g_debrisStepCarry;
static void DebrisStepGate(X86Context& ctx, uint8_t* base, X86Func* original)
{
	const float frames = X86_LOAD_F32(g_deltaTime) * 60.0f;

	// 60FPS
	if (!(frames > 0.0f) || std::fabs(frames - 1.0f) < 0.0001f)
	{
		original(ctx, base);
		return;
	}

	// 61+
	if (g_debrisStepCarry.size() > 512) // clear if 512+ tasks
	{
		g_debrisStepCarry.clear();
	}

	const uint32_t debris = ctx.ecx.u32;
	float &carry = g_debrisStepCarry[debris];
	carry += frames;

	if (carry < 1.0f)
	{
		ctx.AddEsp(2); // skip the call
		return;
	}

	carry -= (float)(int)carry;

	original(ctx, base);

	// clean if destroyed
	if ((X86_LOAD_U8(debris + 0xC) & 1) != 0)
	{
		g_debrisStepCarry.erase(debris);
	}
}

// TObjEffCrash3DChildObj::Exec
X86_FUNC_IMPL(__imp__sub_178850);
X86_FUNC(sub_178850)
{
	DebrisStepGate(ctx, base, __imp__sub_178850);
}

// TObjEffCrash3DRChildObj::Exec
X86_FUNC_IMPL(__imp__sub_178AD0);
X86_FUNC(sub_178AD0)
{
	DebrisStepGate(ctx, base, __imp__sub_178AD0);
}

/** --Destructible/Break object pieces Patch END-- */

/**	--Global FX Alpha Fade Patch START-- */
// Every FX object (including implicit ones in GDT) are affected by this. This fixes a bunch of color bugs as a side effect.
// Fixes the Cosmic Fall 'invisible/missing' BackDrop cycle, Glypic Canyon Platform 'red' issue, and probably a bunch more
// Ended up not needing to patch it, but the Effect::PJS::EffectTask::Exec root is sub_C1080 (handles deltatime fine as is)

// alpha += (int)(fadeRate * dt) (step)
// alpha > 0x3FC00000 = fade done
// alpha < 0x3FFFFF = fade killed off

// Fade rate is 1/60 so 61FPS truncates to fade kill
// Cosmic Fall backdrop never fades in because of this at 61
// Glyphic Canyon Platform and other objs do the same thing (red FX and other weird side effects in the three channel caller)

// FX Fade Node (ecx in both funcs)
// 0x10 started flag
// 0x14 dirty flag
// 0x18 alpha target ptr
// 0x1C timer
// 0x20 duration
// 0x24 fade rate

// Per address carry leftovers to compute proper alpha for 61FPS+
static std::unordered_map<uint32_t, double> g_fxFadeCarry;
static void FxFadeCarryTrim()
{
	if (g_fxFadeCarry.size() > 4096)
	{
		g_fxFadeCarry.clear();
	}
}

// Flag parent state for redraw (completion, 1:1 with original)
static void FxFadeMarkDirty(uint8_t* base, uint32_t node)
{
	uint32_t type = X86_LOAD_U32(node + 4);
	if (type != 0 && X86_LOAD_U8(type + 2) != 0)
	{
		uint32_t owner = X86_LOAD_U32(node + 0x14);
		if (owner != 0)
		{
			X86_STORE_U32(owner, X86_LOAD_U32(owner) | 1);
		}
	}
}

// Single channel impl
// Cosmic Fall BackDrop FX DFFs
X86_FUNC_IMPL(__imp__sub_316BC0);
X86_FUNC(sub_316BC0)
{
	const uint32_t node = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);

	const uint32_t target = X86_LOAD_U32(node + 0x18);
	if (target == 0)
	{
		g_fxFadeCarry.erase(node);
		FxFadeMarkDirty(base, node);
		ctx.eax.u32 = 0;
	}
	else if (X86_LOAD_U8(node + 0x10) == 0)
	{
		X86_STORE_U8(node + 0x10, 1);
		ctx.eax.u32 = 1;
	}
	else
	{
		const float timer = X86_LOAD_F32(node + 0x1C) + dt;
		X86_STORE_F32(node + 0x1C, timer);

		if (timer > X86_LOAD_F32(node + 0x20)) // completed
		{
			g_fxFadeCarry.erase(node);
			FxFadeMarkDirty(base, node);
			ctx.eax.u32 = 0;
		}
		else
		{
			const int32_t rate = (int32_t)X86_LOAD_U32(node + 0x24);
			if (rate != 0)
			{
				if (dt >= 1.0f)
				{
					// 60 FPS or less
					const int32_t step = (int32_t)((float)rate * dt);
					X86_STORE_U32(target, (uint32_t)((int32_t)X86_LOAD_U32(target) + step));

					const int32_t alpha = (int32_t)X86_LOAD_U32(target);
					if (alpha > (int32_t)0x3FC00000)
					{
						X86_STORE_U32(target, 0x3FC00000);
						X86_STORE_U32(node + 0x24, 0);
					}
					else if (alpha < (int32_t)0x3FFFFF)
					{
						X86_STORE_U32(node + 0x18, 0);
						X86_STORE_U32(node + 0x24, 0);
					}
				}
				else
				{
					// 61 FPS+ (prevent over carry)
					FxFadeCarryTrim();
					double exact = (double)rate * (double)dt + g_fxFadeCarry[node];
					int32_t step = (int32_t)exact;
					g_fxFadeCarry[node] = exact - (double)step;

					if (step != 0)
					{
						X86_STORE_U32(target, (uint32_t)((int32_t)X86_LOAD_U32(target) + step));
					}

					const int32_t alpha = (int32_t)X86_LOAD_U32(target);
					if (alpha > (int32_t)0x3FC00000)
					{
						X86_STORE_U32(target, 0x3FC00000);
						X86_STORE_U32(node + 0x24, 0);
						g_fxFadeCarry.erase(node);
					}
					else if (rate < 0 && alpha < (int32_t)0x3FFFFF)
					{
						X86_STORE_U32(node + 0x18, 0);
						X86_STORE_U32(node + 0x24, 0);
						g_fxFadeCarry.erase(node);
					}
				}
			}
			ctx.eax.u32 = 1;
		}
	}
	ctx.esp.u32 += 0x8;
}

// Three channel impl (zeroes on completion instead of clearing)
// Glyphic Canyon Platform, RingShot bullet color
X86_FUNC_IMPL(__imp__sub_3167D0);
X86_FUNC(sub_3167D0)
{
	const uint32_t node = ctx.ecx.u32;
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);

	const uint32_t target = X86_LOAD_U32(node + 0x18);
	if (target == 0)
	{
		FxFadeMarkDirty(base, node);
		ctx.eax.u32 = 0;
	}
	else if (X86_LOAD_U8(node + 0x10) == 0)
	{
		X86_STORE_U8(node + 0x10, 1);
		ctx.eax.u32 = 1;
	}
	else
	{
		const float timer = X86_LOAD_F32(node + 0x1C) + dt;
		X86_STORE_F32(node + 0x1C, timer);

		if (timer > X86_LOAD_F32(node + 0x20))
		{
			FxFadeMarkDirty(base, node);
			ctx.eax.u32 = 0;
		}
		else
		{
			for (uint32_t i = 0; i < 3; i++)
			{
				const uint32_t rateAddr = node + 0x24 + i * 4;
				const int32_t rate = (int32_t)X86_LOAD_U32(rateAddr);
				if (rate == 0)
				{
					continue;
				}

				const uint32_t chan = target + i * 4;
				if (dt >= 1.0f)
				{
					// 60 FPS or less
					const int32_t step = (int32_t)((float)rate * dt);
					X86_STORE_U32(chan, (uint32_t)((int32_t)X86_LOAD_U32(chan) + step));

					const int32_t alpha = (int32_t)X86_LOAD_U32(chan);
					if (alpha < (int32_t)0x3FC00001)
					{
						if (alpha < (int32_t)0x3FFFFF)
						{
							X86_STORE_U32(chan, 0);
							X86_STORE_U32(rateAddr, 0);
						}
					}
					else
					{
						X86_STORE_U32(chan, 0x3FC00000);
						X86_STORE_U32(rateAddr, 0);
					}
				}
				else
				{
					// 61 FPS+ (prevent over carry)
					FxFadeCarryTrim();
					double exact = (double)rate * (double)dt + g_fxFadeCarry[rateAddr];
					int32_t step = (int32_t)exact;
					g_fxFadeCarry[rateAddr] = exact - (double)step;

					if (step != 0)
					{
						X86_STORE_U32(chan, (uint32_t)((int32_t)X86_LOAD_U32(chan) + step));
					}

					const int32_t alpha = (int32_t)X86_LOAD_U32(chan);
					if (alpha > (int32_t)0x3FC00000)
					{
						X86_STORE_U32(chan, 0x3FC00000);
						X86_STORE_U32(rateAddr, 0);
						g_fxFadeCarry.erase(rateAddr);
					}
					else if (rate < 0 && alpha < (int32_t)0x3FFFFF)
					{
						X86_STORE_U32(chan, 0);
						X86_STORE_U32(rateAddr, 0);
						g_fxFadeCarry.erase(rateAddr);
					}
				}
			}
			ctx.eax.u32 = 1;
		}
	}
	ctx.esp.u32 += 0x8;
}
/**	--Global FX Alpha Fade Patch END-- */
