#include <kernel/function.h>

static const uint32_t bloomAddr = 0x52E5C4;
static bool isPatchEnabled = false;


void DisableBloom(uint8_t* base)
{
	X86_STORE_F32(bloomAddr, 0.0f);
}

X86_FUNC_IMPL(__imp__sub_C11C0);
X86_FUNC(sub_C11C0)
{
	if (isPatchEnabled == false)
	{
		DisableBloom(base);
		isPatchEnabled = true;
	}

	__imp__sub_C11C0(ctx, base);
}
