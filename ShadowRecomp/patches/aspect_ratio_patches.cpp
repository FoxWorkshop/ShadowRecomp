#include <kernel/function.h>
#include <patches/aspect_ratio_patches.h>
#include <gpu/video.h>

bool readyToDebugConstant = false;
void PatchResolutionCapMidAsmHook(X86Register& eax)
{

}

const uint32_t GuestDevicePtrAddr = 0x3E0A18;
//GetViewportOffsetAndScale
X86_FUNC_IMPL(__imp__sub_3D24B0);
X86_FUNC(sub_3D24B0)
{
	float* offsetOut = (float*)GetHostParameter(1, CallConvention::Stdcall, ParameterType::Pointer);
	float* scaleOut = (float*)GetHostParameter(2, CallConvention::Stdcall, ParameterType::Pointer);

	__imp__sub_3D24B0(ctx, base);
	auto device = X86_LOAD_U32(GuestDevicePtrAddr);
	device = device;

	if (readyToDebugConstant)
	{

		/**auto padding610 = X86_LOAD_F32(device + 0x94C + 0x10);
		auto padding614 = X86_LOAD_F32(device + 0x94C + 0x14);
		auto padding636 = X86_LOAD_F32(device + 0x970);
		auto padding68 = X86_LOAD_U32(device + 0x954);
		auto padding7 = X86_LOAD_F32(device + 0xEF8);*/

		//printf("viewport horizontal %d, viewport vertical %d\n", X86_LOAD_U32(device + 0xEE8), X86_LOAD_U32(device + 0xEEC));



		//printf("\n");

		readyToDebugConstant = false;

	}
}


X86_FUNC_IMPL(__imp__sub_2C89D0);
X86_FUNC(sub_2C89D0)
{
	readyToDebugConstant = true;
	__imp__sub_2C89D0(ctx, base);
}

//draw CSD element, this patches HUD to implement 16:9 support
// this handle title screen, menu and basic hud in game, but not all of it
X86_FUNC_IMPL(__imp__sub_38B5E0);
X86_FUNC(sub_38B5E0)
{

	UpdateUserConfigConstant(true);
	__imp__sub_38B5E0(ctx, base);
	UpdateUserConfigConstant(false);
}

//draw rest of the hud
X86_FUNC_IMPL(__imp__sub_281550);
X86_FUNC(sub_281550)
{

	UpdateUserConfigConstant(true);
	__imp__sub_281550(ctx, base);
	UpdateUserConfigConstant(false);
}


// menu / goal ring fadeout fix
// currently shader math seems to break pos w so fadeout doesn't work
// this force the vertex pos w to 1 to fix it as a workaround.
// also add support for HD resolution

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

	UpdateUserConfigConstant(true);
	__imp__sub_19730(ctx, base);
	UpdateUserConfigConstant(false);
}

// patch background size for message
X86_FUNC_IMPL(__imp__sub_2A57E0);
X86_FUNC(sub_2A57E0)
{

	UpdateUserConfigConstant(true);
	__imp__sub_2A57E0(ctx, base);
	UpdateUserConfigConstant(false);
}


X86_FUNC_IMPL(__imp__sub_2DD9C0);
X86_FUNC(sub_2DD9C0)
{
	auto rwCam = X86_LOAD_U32(ctx.esp.u32 + 8);
	auto recipX = X86_LOAD_F32(rwCam + 0x70);
	auto recipY = X86_LOAD_F32(rwCam + 0x74);
	if (fabs(recipY - 2.0f) < 0.001f)
	{
		float originalAspect = 4.0f / 3.0f;
		float screenAspect = (float)Video::s_viewportWidth / (float)Video::s_viewportHeight;

		float aspectFix = screenAspect / originalAspect;

	}
	//printf("recipViewWindow.x %f, recipViewWindow.y %f\n", recipX, recipY);

	__imp__sub_2DD9C0(ctx, base);

}


X86_FUNC_IMPL(__imp__sub_1AF2E0);
X86_FUNC(sub_1AF2E0)
{


	__imp__sub_1AF2E0(ctx, base);

}


