#include <kernel/function.h>


bool QuickBoot = true;

//stub unnecessary startup functions to force the game to boot
//to do find a workdaround this is needed for savefile.

NTSTATUS TryOpenVolume(uint32_t* FileHandle)
{
	printf("TryOpenVolume be gone\n");
	return X_STATUS_SUCCESS;
}

NTSTATUS MapLetterToDirectory(uint32_t* FileHandle, WORD* a2)
{
	printf("MapLetterToDirectory be gone\n");
	return X_STATUS_SUCCESS;
}

int XMountUtilityDrive(int32_t a1)
{
	printf("XMountUtilityDrive be gone\n");
	return TRUE;
}


int XAPILIBXGetParentalControlSetting(int a1)
{

	printf("XAPILIBXGetParentalControlSetting be gone\n");
	return X_STATUS_SUCCESS;
}

int sub_2B0164_r(int a1)
{
	return TRUE;
}

//infinite lives todo implement properly as a cheat
bool infiniteLives = true;
X86_FUNC_IMPL(__imp__sub_22D120);
X86_FUNC(sub_22D120)
{
	if (infiniteLives)
	{
		auto lives = X86_LOAD_U32(ctx.esp.u32 + 4);
		lives = 99;
		X86_STORE_U32(ctx.esp.u32 + 4, lives);
	}

	__imp__sub_22D120(ctx, base);
}

//skip FMV
X86_FUNC_IMPL(__imp__sub_24F80);
X86_FUNC(sub_24F80)
{
	if (QuickBoot)
	{
		ctx.eax.u32 = 0;
		ctx.esp.u32 += 4;
		return;
	}
	else
	{
		__imp__sub_24F80(ctx, base);
	}

}
//force menu mode to title screen
X86_FUNC_IMPL(__imp__sub_29EA60);
X86_FUNC(sub_29EA60)
{

	if (QuickBoot)
	{
		auto ptr = X86_LOAD_U32(ctx.ecx.u32 + 4);
		auto mode = X86_LOAD_U32(ptr + 0x64);

		if (mode < 2)
		{
			X86_STORE_U8(ptr + 0x64, 0x2);
		}
	}

	__imp__sub_29EA60(ctx, base);
}


// test spawn level
static bool changeLevelOnce = false;
X86_FUNC_IMPL(__imp__sub_258550);
X86_FUNC(sub_258550)
{
	if (!changeLevelOnce)
	{
		X86_STORE_U32(ctx.esp.u32 + 4, 710);
		changeLevelOnce = true;
	}
	__imp__sub_258550(ctx, base);
}



GUEST_FUNCTION_HOOK_STDCALL(sub_2B0CE1, TryOpenVolume);
GUEST_FUNCTION_HOOK_STDCALL(sub_2B0C27, MapLetterToDirectory); //this stub will trigger the lack of space after pressing start
GUEST_FUNCTION_HOOK_STDCALL(sub_2AF4B3, XMountUtilityDrive);

GUEST_FUNCTION_HOOK(sub_2B0164, sub_2B0164_r);
