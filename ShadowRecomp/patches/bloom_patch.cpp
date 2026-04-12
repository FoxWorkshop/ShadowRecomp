#include <kernel/function.h>
#include <user/config.h>

static const uint32_t bloomAddr = 0x52E5C4;
static bool isPatchEnabled = false;

// change bloom intensity based on user's config.
void PatchBloom(uint8_t* base)
{
	X86_STORE_F32(bloomAddr, Config::BloomIntensity);
}

// EffectPreGlareTask
X86_FUNC_IMPL(__imp__sub_C11C0);
X86_FUNC(sub_C11C0)
{
	if (isPatchEnabled == false)
	{
		PatchBloom(base);
		isPatchEnabled = true;
	}

	__imp__sub_C11C0(ctx, base);
}
