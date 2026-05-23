#include <kernel/function.h>
#include <user/config.h>


const uint32_t g_refRate = 0x54DD80;

void SetFramerateMidAsmHook()
{
	auto base = g_memory.base;
	X86_STORE_U32(g_refRate, Config::FPS);
}