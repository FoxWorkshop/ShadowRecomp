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

bool ShouldApplyVsyncPatch()
{
	return g_isEventPlaying == false;
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

std::unordered_map<uint32_t, float> g_sfxTimers;
// SFX duration are hardcoded without using any kind of delta time
// We need to patch them so they don't cut too early
void SFXTimingMidAsmHook(X86Register &esi)
{
	auto base = g_memory.base; // use base for the load and store macro.
	g_sfxTimers[esi.u32] += X86_LOAD_F32(g_deltaTime) * 60.f; 

	uint32_t timerAddr = esi.u32 + 0xc;
	uint16_t res = X86_LOAD_U16(timerAddr) + 1; // restore og value that the game removed using hardcoded 60 fps.
	X86_STORE_U16(timerAddr, res); // this is needed in case steps condition below doesn't pass even though it's redundant.

	int steps = (int)g_sfxTimers[esi.u32];
	if (steps > 0) 
	{
		g_sfxTimers[esi.u32] -= steps;
		res -= (uint16_t)steps;
		X86_STORE_U16(timerAddr, res);
	}
}

void FreeSFXMapMidAsmHook(X86Register& esi)
{
	g_sfxTimers.erase(esi.u32);
}