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
		X86_STORE_U32(g_refRate, Config::FPS);
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
	return Config::FPS.Value >= 61.0f;
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