#include <kernel/function.h>
#include <user/config.h>


const uint32_t g_deltaTime = 0x541FA8;
const uint32_t g_refRate = 0x54DD80;
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

double GetFPS();
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

// reel goes with fixed speed instead of delta time

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