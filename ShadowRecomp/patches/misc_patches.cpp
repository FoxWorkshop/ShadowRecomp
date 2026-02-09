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


