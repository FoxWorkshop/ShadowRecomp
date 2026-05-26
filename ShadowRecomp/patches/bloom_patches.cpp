#include <kernel/function.h>
#include <user/config.h>

static const uint32_t bloomAddr = 0x52E5C4;

// change bloom intensity based on user's config.
void PatchBloomIntensity(uint8_t* base)
{
	X86_STORE_F32(bloomAddr, Config::BloomIntensity.Value);
}
