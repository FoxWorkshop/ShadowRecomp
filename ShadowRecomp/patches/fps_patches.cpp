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

// Pass the first collision sweep @ 0x85420 to run 0x84E50
// Only enabling for 61+ FPS to avoid the slight camera 'drop' for slide while this is active.
bool HighFramerateCollisionFixMidAsmHook()
{
	return GetTargetFPS() >= 61.0;
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
// TODO: May need to alter when implementing Air Saucer
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
