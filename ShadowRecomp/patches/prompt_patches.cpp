#include <kernel/function.h>


//skip error message save
X86_FUNC_IMPL(__imp__sub_2A6530);
X86_FUNC(sub_2A6530)
{
	auto mode = X86_LOAD_U32(ctx.ecx.u32);

	if (mode == 6)
	{
		X86_STORE_U32(ctx.ecx.u32, 14);
	}

	__imp__sub_2A6530(ctx, base);
}



// player quit prompt
X86_FUNC_IMPL(__imp__sub_5A7B0);
X86_FUNC(sub_5A7B0)
{


	__imp__sub_5A7B0(ctx, base);
}