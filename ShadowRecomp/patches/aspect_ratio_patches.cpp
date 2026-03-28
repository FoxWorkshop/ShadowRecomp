#include <kernel/function.h>
#include <patches/aspect_ratio_patches.h>
#include <gpu/video.h>

bool readyToDebugConstant = false;
#define widthMin 640.f
#define heightMin 480.f

static float ComputeScale(float aspectRatio)
{
	return ((aspectRatio * heightMin) / widthMin) / sqrt((aspectRatio * heightMin) / widthMin);
}

void AspectRatioPatches::ComputeOffsets()
{
	float width = Video::s_viewportWidth;
	float height = Video::s_viewportHeight;

	g_aspectRatio = width / height;
	g_aspectRatioGameplayScale = 1.0f;

	if (g_aspectRatio >= NARROW_ASPECT_RATIO)
	{
		g_aspectRatioOffsetX = (width - height * WIDE_ASPECT_RATIO) / 2.0f;
		g_aspectRatioOffsetY = 0.0f;
		g_aspectRatioScale = height / heightMin;

		// keep same scale above Steam Deck aspect ratio
		if (g_aspectRatio < WIDE_ASPECT_RATIO)
		{
			// interpolate to original 4:3 scale
			float steamDeckScale = g_aspectRatio / WIDE_ASPECT_RATIO;
			float narrowScale = ComputeScale(NARROW_ASPECT_RATIO);

			float lerpFactor = std::clamp((g_aspectRatio - NARROW_ASPECT_RATIO) / (STEAM_DECK_ASPECT_RATIO - NARROW_ASPECT_RATIO), 0.0f, 1.0f);
			g_aspectRatioGameplayScale = narrowScale + (steamDeckScale - narrowScale) * lerpFactor;
		}
	}
	else
	{
		// 4:3 crop
		g_aspectRatioOffsetX = (width - width * NARROW_ASPECT_RATIO) / 2.0f;
		g_aspectRatioOffsetY = (height - width / NARROW_ASPECT_RATIO) / 2.0f;
		g_aspectRatioScale = width / widthMin;
		g_aspectRatioGameplayScale = ComputeScale(NARROW_ASPECT_RATIO);
	}

	g_aspectRatioNarrowScale = std::clamp((g_aspectRatio - NARROW_ASPECT_RATIO) / (WIDE_ASPECT_RATIO - NARROW_ASPECT_RATIO), 0.0f, 1.0f);
}

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

		auto padding610 = X86_LOAD_F32(device + 0x94C + 0x10);
		auto padding614 = X86_LOAD_F32(device + 0x94C + 0x14);

		auto padding636 = X86_LOAD_F32(device + 0x970);
		auto padding68 = X86_LOAD_U32(device + 0x954);

		auto padding616 = X86_LOAD_F32(device + 0x95C);
		auto padding7 = X86_LOAD_F32(device + 0xEF8);
		auto padding74 = X86_LOAD_F32(device + 0xEFC);
		//printf("&D3D_g_pDevice->padding6[16] %f D3D_g_pDevice->padding7 %f D3D_g_pDevice->padding7[4] %f\n", padding616, padding7, padding74);



		//printf("\n");

		readyToDebugConstant = false;

	}
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

X86_FUNC_IMPL(__imp__sub_19730);
X86_FUNC(sub_19730)
{

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

// remove the infamous half-pixel offset issue 
// see https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#3.3.1%20Pixel%20Coordinate%20System
void D3DDevice_SetScreenSpaceOffset(float a1, float a2)
{
	;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3D1320, D3DDevice_SetScreenSpaceOffset);
