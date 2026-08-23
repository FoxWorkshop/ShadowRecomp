/*
    Precision Sleep (Windows 10)

        A sleep_for() that accepts sub-millisecond durations on a 0.5ms
        quantum, via a high-resolution waitable timer (Windows 10 1803+).
        Sleep() only does whole milliseconds (1ms quantum), so this saves
        ~0.5ms of yielding per frame in the pacer's final approach.
*/

#pragma once

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <thread>

namespace PrecisionSleep
{
    /* Returns whether the high-resolution waitable timer is available.
       Cheap after the first call. */
    inline bool IsAvailable();

    /* Sleeps for the given duration in milliseconds, sleep_for-style.
       Wakes after the requested time on the timer grid (no earlier). */
    inline void Sleep(double durationMs);

    namespace Detail
    {
        /*
         * The timer handle is per-thread because SetWaitableTimer arms it
         * for a single waiter. Leaked on purpose; one handle per pacing
         * thread for the process lifetime.
         */
        inline HANDLE TimerSlot()
        {
            thread_local HANDLE timer = nullptr;
            thread_local bool initialized = false;

            if (!initialized)
            {
                initialized = true;
                // CREATE_WAITABLE_TIMER_HIGH_RESOLUTION needs Windows 10
                // 1803+; on older systems the flag is rejected and callers
                // fall back to regular Sleep(). [null handle]
                timer = CreateWaitableTimerExW(
                    nullptr, nullptr, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
            }

            return timer;
        }
    }

    inline bool IsAvailable()
    {
        return Detail::TimerSlot() != nullptr;
    }

    inline void Sleep(double durationMs)
    {
        if (durationMs <= 0.0)
            return;

        const HANDLE timer = Detail::TimerSlot();
        if (timer == nullptr)
            return;

        // Due time is in 100ns units; negative means relative to now.
        LARGE_INTEGER dueTime;
        dueTime.QuadPart = -static_cast<LONGLONG>(durationMs * 10000.0);
        if (!SetWaitableTimer(timer, &dueTime, 0, nullptr, nullptr, FALSE))
            return;

        WaitForSingleObject(timer, INFINITE);
    }
}

#endif // _WIN32
