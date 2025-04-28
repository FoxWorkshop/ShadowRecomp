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


GUEST_FUNCTION_HOOK_STDCALL(sub_2AAF10, QueryPerformanceFrequencyImpl);
GUEST_FUNCTION_HOOK(sub_388458, GetKeTickCountImpl);

GUEST_FUNCTION_HOOK(sub_37FDA0, memcpy);
GUEST_FUNCTION_HOOK(sub_380F60, memset);
GUEST_FUNCTION_HOOK(sub_380720, strncat);
GUEST_FUNCTION_HOOK(sub_37E820, memmove);
//GUEST_FUNCTION_HOOK(sub_37E7BB, vsprintf); -> used for logs
GUEST_FUNCTION_HOOK(sub_37FC10, strcpy);
GUEST_FUNCTION_HOOK(sub_37FC20, strcat);
GUEST_FUNCTION_HOOK(sub_37F1E0, strncpy);
GUEST_FUNCTION_HOOK(sub_37DEC0, strncmp);
GUEST_FUNCTION_HOOK(sub_37EB60, strstr_);

//GUEST_FUNCTION_HOOK(sub_37E577, sprintf);
//GUEST_FUNCTION_HOOK(sub_37F0C6, sscanf);


GUEST_FUNCTION_HOOK(sub_2AAEDE, OutputDebugStringA);