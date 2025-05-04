#include <kernel/function.h>

//stub unnecessary startup functions to force the game to boot


NTSTATUS TryOpenVolume(uint32_t* FileHandle)
{
	printf("TryOpenVolume be gone\n");
	return X_STATUS_SUCCESS;
}

NTSTATUS MapLetterToDirectory(uint32_t* FileHandle)
{
	printf("MapLetterToDirectory be gone\n");
	return X_STATUS_SUCCESS;
}

int XMountUtilityDrive(int a1)
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

int sub_3864B8_r(int a1, int a2)
{
	//RtlInitializeCriticalSection(a1);
	return TRUE;
}

int sub_32B5C0_r(char* Str1)
{
	return 1;
}

int sub_32B900_r(char* Str1, int a2)
{
	return 1;
}

//GUEST_FUNCTION_HOOK(sub_32B900, sub_32B900_r);

//GUEST_FUNCTION_HOOK(sub_32B5C0, sub_32B5C0_r);
GUEST_FUNCTION_HOOK(sub_2B0CE1, TryOpenVolume);
GUEST_FUNCTION_HOOK_STDCALL(sub_2B0C27, MapLetterToDirectory);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AF4B3, XMountUtilityDrive);
GUEST_FUNCTION_HOOK(sub_2B17C3, XAPILIBXGetParentalControlSetting);
GUEST_FUNCTION_HOOK(sub_2B0164, sub_2B0164_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3864B8, sub_3864B8_r);

