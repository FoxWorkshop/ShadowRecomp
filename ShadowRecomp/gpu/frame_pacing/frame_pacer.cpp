#include "frame_pacer.h"
#include "timer_resolution.h"
#include <thread>

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

        // Sleep() below 1ms it floors to Sleep(0).
        // That just performs a yield. We made that clear here.
        // 
        // One granularity is subtracted because Sleep(n) can wake
        // up to one tick late (per Windows docs).
        const double sleepTimeLeftMs = timeLeftMs - timerGranularity;
        if (sleepTimeLeftMs >= 1.0)
        {
            // Sleep the thread if above granularity.
            std::this_thread::sleep_for(
                std::chrono::milliseconds(static_cast<long long>(sleepTimeLeftMs)));
        }
        else
        {
            // Yield the thread if below granularity.
            std::this_thread::yield();
        }
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
