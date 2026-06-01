#include <kernel/function.h>
#include <user/config.h>

//HintRingInit aka the capsule with "?" 
X86_FUNC_IMPL(__imp__sub_1A2350);
X86_FUNC(sub_1A2350)
{
	if (Config::Hints.Value == false)
	{
		ctx.AddEsp(1);
	}
	else
	{
		__imp__sub_1A2350(ctx, base);
	}
}

//HintRingExec this handle the collision that trigger the display of the message
X86_FUNC_IMPL(__imp__sub_1A1E60);
X86_FUNC(sub_1A1E60)
{
	if (Config::Hints.Value == false)
	{
		ctx.AddEsp(2); 
	}
	else
	{
		__imp__sub_1A1E60(ctx, base);
	}
}

// sub_17E290 (ObjectUpdateVisibility (gadgets)) calls sub_17C130 (SetFadeAlpha)
// with a fade value that gets added to skinned_pos.w in the vertex shader, causing bugged objects to flash bright during fade-in/fade-out
// Only stub the fade alpha write when called from the object fade-in/fade-out
static thread_local bool g_inSub17E290 = false;

X86_FUNC_IMPL(__imp__sub_17C130);
X86_FUNC(sub_17C130)
{
	if (g_inSub17E290)
	{
		ctx.esp.u32 += 0x8;
		return;
	}
	__imp__sub_17C130(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_17E290);
X86_FUNC(sub_17E290)
{
	g_inSub17E290 = true;
	__imp__sub_17E290(ctx, base);
	g_inSub17E290 = false;
}
