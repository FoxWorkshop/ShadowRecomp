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
