#include <gpu/video.h>
#include <kernel/function.h>

// menu / goal ring fadeout fix
// currently shader math seems to break pos w so fadeout doesn't work
// this force the vertex pos w to 1 to fix it as a workaround.

X86_FUNC_IMPL(__imp__sub_19730);
X86_FUNC(sub_19730)
{
	int32_t addr = ctx.ecx.u32;
	UpVertex* FadeoutvertexData = (UpVertex*)(base + addr + 24);

	if (FadeoutvertexData[0].w > 1.0f)
	{
		int32_t count = 10;
		do
		{
			for (int32_t i = 0; i < 4; i++)
			{
				FadeoutvertexData[i].w = 1.0f;
			}

			FadeoutvertexData += 4;
			--count;
		} while (count);


		UpVertex* LoadingvertexData = (UpVertex*)(base + addr + 1156);

		for (int32_t i = 0; i < 4; i++)
		{
			LoadingvertexData[i].w = 1.0f;
		}
	}

	__imp__sub_19730(ctx, base);
}



// Patch out logs Artificial Chaos can spew (causes crashes)
X86_FUNC(sub_37EBE6)
{
	ctx.esp.u32 += 4;
	return;
}

//leftover debugprint functions be gone!
X86_FUNC(sub_3DA150)
{
	ctx.esp.u32 += 4;
	return;
}

X86_FUNC(sub_41D78A)
{
	ctx.esp.u32 += 4;
	return;
}

X86_FUNC(sub_41A46B)
{
	ctx.esp.u32 += 4;
	return;
}


X86_FUNC(sub_14710)
{
	ctx.esp.u32 += 4;
	return;
}

X86_FUNC(sub_40634F)
{
	ctx.esp.u32 += 4;
	return;
}

//this one is a hard lock, we will let it run
X86_FUNC(sub_2B1A00)
{
	printf("Not inititalized m_eDealloctor\n");
	while (1)
		;
}

X86_FUNC(sub_41C0D5)
{
	ctx.esp.u32 += 4;
	return;
}

X86_FUNC(sub_41CD65)
{
	ctx.esp.u32 += 4;
	return;
}


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

