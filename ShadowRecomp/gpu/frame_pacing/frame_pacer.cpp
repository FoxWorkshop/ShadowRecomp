#include "frame_pacer.h"
#include "timer_resolution.h"
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

void FramePacer::EndFrame(bool spin, bool allowThreadYieldInSpin)
{
    // Summarize stats for the current frame.
    const double renderTimeMs = ElapsedMsSince(m_frameStart);

    double sleepTimeMs = m_frameTimeTargetMs - m_oversleptMs - renderTimeMs;
    if (sleepTimeMs < 0.0)
        sleepTimeMs = 0.0;

    // Sleep
    Sleep(sleepTimeMs, spin, allowThreadYieldInSpin);

    // Restart calculation for new frame.
    m_frameStart = std::chrono::steady_clock::now();
}

void FramePacer::Sleep(double durationMs, bool spin, bool allowThreadYieldInSpin)
{
    const double timerGranularity = TimerResolution::GetGranularityMs();
    m_sleepStart = std::chrono::steady_clock::now();

    double timeLeftMs;
    while ((timeLeftMs = durationMs - ElapsedMsSince(m_sleepStart)) > 0.0)
    {
        // Spin if below allocation granularity.
        if (spin && timeLeftMs < timerGranularity)
        {
            Spin(durationMs, allowThreadYieldInSpin);
            break;
        }

        // One granularity is subtracted because a sleep can wake up to one
        // granularity tick late (per Windows docs); the yield/spin finishes
        // the rest. (Same logic applies to Linux.
        const double sleepTimeLeftMs = timeLeftMs - timerGranularity;

#ifdef _WIN32
        // Waitable timer (Windows 10 1803+) accepts 0.5ms (0.5-1.0ms) sleep where
        // sleep_for on MSVC rounds up and does Sleep(1) 1ms (1.0-1.5ms).
        // So we can save some more CPU here.
        if (PrecisionSleep::IsAvailable())
        {
            PrecisionSleep::Sleep(sleepTimeLeftMs);
            continue;
        }
#else
        if (sleepTimeLeftMs > 0.0)
            std::this_thread::sleep_for(std::chrono::duration<double, std::milli>(sleepTimeLeftMs));
        else
            std::this_thread::yield();
#endif
    }

    m_oversleptMs = ElapsedMsSince(m_sleepStart) - durationMs;
}

void FramePacer::Spin(double durationMs, bool allowThreadYieldInSpin)
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
        if (!allowThreadYieldInSpin)
            continue;

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
