#include "frame_pacer.h"
#include "timer_resolution.h"
#include <algorithm>
#include <chrono>
#include <thread>

#ifdef _WIN32
#include "precision_sleep_win10.h"
#endif

void FramePacer::SetFpsLimit(double fpsLimit)
{
    if (fpsLimit > 0.0)
        m_frameTimeTargetMs = 1000.0 / fpsLimit;
}

void FramePacer::EndFrame(WaitMode waitMode)
{
    // Summarize stats for the current frame.
    const double renderTimeMs = ElapsedMsSince(m_frameStart);

    double sleepTimeMs = m_frameTimeTargetMs - m_catchUpMs - renderTimeMs;
    double overrunMs = 0.0;
    if (sleepTimeMs < 0.0)
    {
        // Late frames (FPS drop) can't sleep.
        // Instead, carry this delay to the next frame.
        // That way, next frame can compensate by sleeping less..
        overrunMs = std::min(-sleepTimeMs, m_frameTimeTargetMs);
        sleepTimeMs = 0.0;
    }

    // Sleep
    Sleep(sleepTimeMs, waitMode);
    m_catchUpMs += overrunMs;

    // Restart calculation for new frame.
    m_frameStart = std::chrono::steady_clock::now();
}

void FramePacer::Sleep(double durationMs, WaitMode waitMode)
{
    // First pacer use on a thread raises its timer resolution; finer timer
    // ticks mean every mode wakes closer to its deadline.
    TimerResolution::GetGranularityMs();

    m_sleepStart = std::chrono::steady_clock::now();

    switch (waitMode)
    {
    case WaitMode::SleepOnly:
        SleepOnly(durationMs);
        break;
    case WaitMode::SleepThenSpin:
        SleepThenSpin(durationMs);
        break;
    case WaitMode::SleepThenYield:
        SleepThenYield(durationMs);
        break;
    }

    m_catchUpMs = ElapsedMsSince(m_sleepStart) - durationMs;
}

void FramePacer::SleepOnly(double durationMs)
{
    // Hands the OS the full remaining time instead of spinning/yielding
    // through the final slice; it can wake up to one sleep quantum late,
    // never early.
    double timeLeftMs;
    while ((timeLeftMs = durationMs - ElapsedMsSince(m_sleepStart)) > 0.0)
    {
#ifdef _WIN32
        // High-resolution waitable timer; see precision_sleep_win10.h.
        if (PrecisionSleep::IsAvailable())
        {
            PrecisionSleep::Sleep(timeLeftMs);
            continue;
        }
#endif
        std::this_thread::sleep_for(
            std::chrono::duration<double, std::milli>(timeLeftMs));
    }
}

void FramePacer::SleepThenSpin(double durationMs)
{
    const double timerGranularity = TimerResolution::GetGranularityMs();

    double timeLeftMs;
    while ((timeLeftMs = durationMs - ElapsedMsSince(m_sleepStart)) > 0.0)
    {
        // Spin if below allocation granularity.
        if (timeLeftMs < timerGranularity)
        {
            Spin(durationMs);
            break;
        }

        // One granularity is subtracted because a sleep can wake up to one
        // granularity tick late (per Windows docs); the spin finishes the
        // rest. The same logic applies to Linux.
        const double sleepTimeLeftMs = timeLeftMs - timerGranularity;

#ifdef _WIN32
        // High-resolution waitable timer; see precision_sleep_win10.h.
        if (PrecisionSleep::IsAvailable())
        {
            PrecisionSleep::Sleep(sleepTimeLeftMs);
            continue;
        }
#else
        if (sleepTimeLeftMs > 0.0)
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(sleepTimeLeftMs));
#endif
    }
}

void FramePacer::SleepThenYield(double durationMs)
{
    const double timerGranularity = TimerResolution::GetGranularityMs();

    double timeLeftMs;
    while ((timeLeftMs = durationMs - ElapsedMsSince(m_sleepStart)) > 0.0)
    {
        // One granularity is subtracted because a sleep can wake up to one
        // granularity tick late (per Windows docs); the yield finishes the
        // rest. The same logic applies to Linux.
        const double sleepTimeLeftMs = timeLeftMs - timerGranularity;

#ifdef _WIN32
        // High-resolution waitable timer; see precision_sleep_win10.h.
        if (PrecisionSleep::IsAvailable() && sleepTimeLeftMs > 0.0)
        {
            PrecisionSleep::Sleep(sleepTimeLeftMs);
            continue;
        }
#endif
        if (sleepTimeLeftMs > 0.0)
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(sleepTimeLeftMs));
        else
            std::this_thread::yield();
    }
}

void FramePacer::Spin(double durationMs)
{
    double lastYieldTimeMs = 0.0;
    double timeLeftMs;

    // Spin!!
    auto runSpinIteration = [&]()
    {
        const auto yieldStart = std::chrono::steady_clock::now();
        std::this_thread::yield();
        lastYieldTimeMs = ElapsedMsSince(yieldStart);
    };

    while ((timeLeftMs = durationMs - ElapsedMsSince(m_sleepStart)) > 0.0)
    {
        if (lastYieldTimeMs > 0.0)
        {
            if (timeLeftMs > lastYieldTimeMs)
                runSpinIteration();
        }
        else
        {
            runSpinIteration();
        }
    }
}
