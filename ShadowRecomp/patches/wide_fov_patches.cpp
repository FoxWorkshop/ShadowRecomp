#include <kernel/function.h>
#include <user/config.h>

// Widescreen FOV Hack (Global)
static constexpr uint32_t globalFOV = 0x4E5D6C;

void WideFOVConfigValueChangedCallback(const IConfigDef* config)
{
	if (config == &Config::WideFOV)
	{
		const auto base = g_memory.base;

		if (Config::WideFOV.Value)
		{
			X86_STORE_F32(globalFOV, 1.33333337306976f);
		}
		else
		{
			X86_STORE_F32(globalFOV, 1.0f);
		}
	}
}

// CameraDemo = CC/CB Camera - Patch CC camera to original FOV due to compression on CC cam
static int32_t g_cameraDemoDepth = 0;

static void PushCameraDemoFOV()
{
	const auto base = g_memory.base;
	if (!Config::WideFOV.Value || Config::ExtendedMultiplayer)
	{
		return;
	}

	if (g_cameraDemoDepth++ == 0)
	{
		X86_STORE_F32(globalFOV, 1.0f);
	}
}

static void PopCameraDemoFOV()
{
	const auto base = g_memory.base;
	if (!Config::WideFOV.Value || Config::ExtendedMultiplayer)
	{
		return;
	}

	if (g_cameraDemoDepth > 0 && --g_cameraDemoDepth == 0)
	{
		X86_STORE_F32(globalFOV, 1.33333337306976f);
	}
}

// CameraDemo constructor
X86_FUNC_IMPL(__imp__sub_90700);
X86_FUNC(sub_90700)
{
	uint32_t thisPtr = ctx.ecx.u32;
	__imp__sub_90700(ctx, base);

	// 0 = ChaosBlast, 1 = ChaosControl
	uint32_t demoType = X86_LOAD_U32(thisPtr + 0x30);
	if (demoType == 1)
	{
		PushCameraDemoFOV();
	}
}

// CameraDemo destructor
X86_FUNC_IMPL(__imp__sub_8D2B0);
X86_FUNC(sub_8D2B0)
{
	uint32_t demoType = X86_LOAD_U32(ctx.ecx.u32 + 0x30);
	__imp__sub_8D2B0(ctx, base);
	if (demoType == 1)
	{
		PopCameraDemoFOV();
	}
}

bool SkipCameraDemoMidAsmHook()
{
	return Config::ExtendedMultiplayer;
}
