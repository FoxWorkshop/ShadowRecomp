#include "timer_resolution.h"

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

/*
 * NtQueryTimerResolution/NtSetTimerResolution are technically internal APIs
 * but have been used in many pieces of software for years, including the
 * Chrome browser.
 */
typedef LONG NTSTATUS;
typedef NTSTATUS(NTAPI* QueryTimerResolutionFn)(PULONG, PULONG, PULONG);
typedef NTSTATUS(NTAPI* SetTimerResolutionFn)(ULONG, BOOLEAN, PULONG);
#endif

double TimerResolution::s_granularityMs = 1.0;
bool TimerResolution::s_isActive = false;
bool TimerResolution::s_isResolved = false;

bool TimerResolution::SetHighestResolution()
{
#ifdef _WIN32
    if (s_isActive)
        return true;

    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll == nullptr)
        return false;

    const auto query = reinterpret_cast<QueryTimerResolutionFn>(
        GetProcAddress(ntdll, "NtQueryTimerResolution"));
    const auto set = reinterpret_cast<SetTimerResolutionFn>(
        GetProcAddress(ntdll, "NtSetTimerResolution"));
    if (query == nullptr || set == nullptr)
        return false;

    // The maximum resolution value is the finest tick the OS supports.
    ULONG minResolution, maxResolution, actualResolution;
    query(&minResolution, &maxResolution, &actualResolution);

    if (set(maxResolution, TRUE, &actualResolution) == 0)
    {
        s_granularityMs = actualResolution / 10000.0; // 100ns ticks to ms
        s_isActive = true;
        return true;
    }
#endif

    return false;
}

double TimerResolution::GetGranularityMs()
{
    if (!s_isResolved)
    {
        s_isResolved = true;
        SetHighestResolution();
    }

    return s_granularityMs;
}

void TimerResolution::Release()
{
#ifdef _WIN32
    if (!s_isActive)
        return;

    const HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (ntdll != nullptr)
    {
        const auto set = reinterpret_cast<SetTimerResolutionFn>(
            GetProcAddress(ntdll, "NtSetTimerResolution"));
        if (set != nullptr)
        {
            ULONG actualResolution;
            set(0, FALSE, &actualResolution);
        }
    }

    s_isActive = false;
    s_granularityMs = 1.0;
    s_isResolved = false;
#endif
}
