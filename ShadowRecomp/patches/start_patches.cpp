#include <kernel/function.h>
#include <user/config.h>

extern bool QuickBootConfig;
extern int Ts_StageID;

//stub unnecessary startup functions to force the game to boot

NTSTATUS TryOpenVolume(uint32_t* FileHandle)
{
	return X_STATUS_SUCCESS;
}

NTSTATUS MapLetterToDirectory(uint32_t* FileHandle, WORD* a2)
{
	return X_STATUS_SUCCESS;
}

int XMountUtilityDrive(int32_t a1)
{
	return TRUE;
}

int XAPILIBXGetParentalControlSetting(int a1)
{
	return X_STATUS_SUCCESS;
}

int sub_2B0164_r(int a1)
{
	return TRUE;
}

//infinite lives Cheat
X86_FUNC_IMPL(__imp__sub_22D120);
X86_FUNC(sub_22D120)
{
	if (Config::InfiniteLives)
	{
		if (Config::InfiniteLives && (int32_t)X86_LOAD_U32(ctx.esp.u32 + 4) < 0)
			X86_STORE_U32(ctx.esp.u32 + 4, 0);
	}

	__imp__sub_22D120(ctx, base);
}

// Quick Boot
// Force mode 3 on the boot intro to reach directly the title screen
static constexpr uint32_t g_MenuModeRequest = 0x54BA10;
static bool s_quickBootDone = false;
X86_FUNC_IMPL(__imp__sub_29EA60);
X86_FUNC(sub_29EA60)
{
	const bool quickBoot = Config::QuickBoot || QuickBootConfig;
	auto ptr = X86_LOAD_U32(ctx.ecx.u32 + 4);

	if (!s_quickBootDone && quickBoot)
	{
		if (X86_LOAD_U32(ptr + 0x64) < 3)
			X86_STORE_U32(ptr + 0x64, 3);

		s_quickBootDone = true;
	}

	__imp__sub_29EA60(ctx, base);

	// mode 3 asks again every frame until the menu manager swaps the task, so check every call
	if (quickBoot && X86_LOAD_U32(ptr + 0x64) == 3)
	{
		auto menuMode = X86_LOAD_U32(g_MenuModeRequest);

		if (menuMode && X86_LOAD_U32(menuMode) == 8)
			X86_STORE_U32(menuMode, 1);
	}
}

// Always allow skipping the title screen logos
void AlwaysAllowLogoSkipMidAsmHook() {}

// SwapGameMode
// boot directly into a stage when --l <stage id> is used
// we first force game mode to 5 (StageSequence::Manager)
// then assign the new stage id
static bool StageBootDone = false;
X86_FUNC_IMPL(__imp__sub_5C5E0);
X86_FUNC(sub_5C5E0)
{
	if (!StageBootDone && Ts_StageID != -1 && X86_LOAD_U32(ctx.esp.u32 + 4) == 4)
	{
		StageBootDone = true;

		X86_STORE_U32(ctx.esp.u32 + 4, 5); // force game mode to 5 for levels
		X86_STORE_U32(0x5022A4, 0);
		X86_STORE_U32(0x54C910, Ts_StageID);

		// reset the stage sequence state
		GuestToHostFunction<void, CallConvention::Thiscall>(sub_258610, 0x54DDFC);

		// we need to assign the pad num to the controller data
		// otherwise Vibration::Manager will crash the game with a vector out of range
		uint32_t padState = X86_LOAD_U32(0x5356F0);

		if (!padState)
			padState = GuestToHostFunction<uint32_t, CallConvention::Cdecl>(sub_11410);

		if (padState) // set pad num per player
		{
			X86_STORE_U32(padState + 404, 0);
			X86_STORE_U32(padState + 408, 1);
		}
	}

	__imp__sub_5C5E0(ctx, base);
}

GUEST_FUNCTION_HOOK_STDCALL(sub_2B0CE1, TryOpenVolume);
GUEST_FUNCTION_HOOK_STDCALL(sub_2B0C27, MapLetterToDirectory); //this stub will trigger the lack of space after pressing start
GUEST_FUNCTION_HOOK_STDCALL(sub_2AF4B3, XMountUtilityDrive);

GUEST_FUNCTION_HOOK(sub_2B0164, sub_2B0164_r);