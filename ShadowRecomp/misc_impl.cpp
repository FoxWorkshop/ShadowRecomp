#include "stdafx.h"
#include <kernel/function.h>

uint32_t GetKeTickCountImpl()
{
    return uint32_t(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());
}


BOOL QueryPerformanceFrequencyImpl(LARGE_INTEGER* lpPerformanceCount)
{
    lpPerformanceCount->QuadPart = std::chrono::steady_clock::period::den / std::chrono::steady_clock::period::num;
    return TRUE;
}


const char* strstr_(const char* src, const char* subStr)
{
    return strstr(src, subStr);
}

int strcmp_(const char* Str1, const char* Str2)
{
    return strcmp(Str1, Str2);
}

int __cdecl strncmp_(const char* Str1, const char* Str2, size_t MaxCount)
{
    return strncmp(Str1, Str2, MaxCount);
}

int sub_2C0390_r(int Str1)
{

    const char** t = (const char**)(g_memory.base + Str1);
    return 0;
}

//example of how to grab usercall args, no need to use X86_LOAD here.
//int __usercall FillDeviceName@<eax>(char* a1@<eax>, int(__thiscall* a2)(unsigned int))
X86_FUNC_IMPL(__imp__sub_32B4F0);
X86_FUNC(sub_32B4F0)
{
    auto a1 = ctx.eax.u32;
    char* oldPath = (char*)(base + a1);

	__imp__sub_32B4F0(ctx, base);
    a1 = 0x560624;
    oldPath = (char*)(base + a1);
    oldPath = oldPath;
    oldPath += 16;
    oldPath = oldPath;

	return;
}

char* __cdecl _StrChr(char* a1, char a2)
{
    return strchr(a1, a2);
}

char* __cdecl _StrrChr(char* a1, char a2)
{
    return strrchr(a1, a2);
}

//GUEST_FUNCTION_HOOK(sub_2C0390, sub_2C0390_r);
#include<string.h>
GUEST_FUNCTION_HOOK_STDCALL(sub_2AAF10, QueryPerformanceFrequencyImpl);
GUEST_FUNCTION_HOOK(sub_388458, GetKeTickCountImpl);

GUEST_FUNCTION_HOOK(sub_37FDA0, memcpy);
GUEST_FUNCTION_HOOK(sub_380F60, memset);

GUEST_FUNCTION_HOOK(sub_37FC10, strcpy); //invalid size because ida share some part of this function with another
GUEST_FUNCTION_HOOK(sub_37F1E0, strncpy);
GUEST_FUNCTION_HOOK(sub_37FC20, strcat); //same as above
GUEST_FUNCTION_HOOK(sub_380720, strncat);
GUEST_FUNCTION_HOOK(sub_2EC170, _StrChr);
GUEST_FUNCTION_HOOK(sub_2EC190, _StrrChr);
GUEST_FUNCTION_HOOK(sub_37EB60, strstr_);
GUEST_FUNCTION_HOOK(sub_37E820, memmove);
//GUEST_FUNCTION_HOOK(sub_37E7BB, vsprintf); -> used for logs

GUEST_FUNCTION_HOOK(sub_3800E0, strcmp_);
GUEST_FUNCTION_HOOK(sub_37DEC0, strncmp_);
GUEST_FUNCTION_HOOK(sub_2EC0C0, _stricmp);
GUEST_FUNCTION_HOOK(sub_37FD10, strlen);
GUEST_FUNCTION_HOOK(sub_2EC110, _strupr);
GUEST_FUNCTION_HOOK(sub_2EC140, _strlwr);
GUEST_FUNCTION_HOOK(sub_38066D, strtok);

//GUEST_FUNCTION_HOOK(sub_37E577, sprintf);
//GUEST_FUNCTION_HOOK(sub_37F0C6, sscanf);


GUEST_FUNCTION_HOOK(sub_2AAEDE, OutputDebugStringA);