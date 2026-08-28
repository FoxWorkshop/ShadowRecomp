/*
    Sewer's Frame Pacer (2021)

    Allows frame pacing control of arbitrary code, such as a game logic loop (here)
    or animations.

    To use this class, simply place EndFrame at the end/exit points of a
    recurring logic loop. The FramePacer will do the rest.

    The Eggman Empire thanks you for your precise timing.
*/

#pragma once

#include <chrono>

class FramePacer
{
public:
    /**
     * \brief How the remaining time to the next frame is waited out.
     */
    enum class WaitMode
    {
        /** \brief Sleeps most of it, then spins (yielding between
         * iterations) through the final slice. Most precise; negligible
         * CPU load. */
        SleepThenSpin,

        /** \brief Sleeps most of it, then yields through the final slice. */
        SleepThenYield,

        /** \brief Sleeps all of it; never spins or yields. Cheapest for
         * background threads; can wake up to one sleep quantum late. */
        SleepOnly
    };

    /**
     * \brief Sets the current framerate cap.
     * \param fpsLimit The framerate cap in frames per second. Values of zero
     * or below are ignored.
     */
    void SetFpsLimit(double fpsLimit);

    /**
     * \brief Marks the end of an individual frame/recurring piece of logic
     * to be performed/executed. You should put this at the end of a
     * reoccuring loop.
     * \param waitMode How the remaining wait time is spent. See WaitMode.
     */
    void EndFrame(WaitMode waitMode);

private:
    /**
     * \brief Pauses execution for the remaining of the time until the next
     * frame begins.
     * \param durationMs The time in milliseconds to pause for.
     * \param waitMode See EndFrame.
     */
    void Sleep(double durationMs, WaitMode waitMode);

    /* Waits out the duration per WaitMode::SleepOnly. */
    void SleepOnly(double durationMs);

    /* Waits out the duration per WaitMode::SleepThenSpin. */
    void SleepThenSpin(double durationMs);

    /* Waits out the duration per WaitMode::SleepThenYield. */
    void SleepThenYield(double durationMs);

    /* Spins until it is time to begin the next frame. */
    void Spin(double durationMs);

    static double ElapsedMsSince(std::chrono::steady_clock::time_point start)
    {
        return std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - start).count();
    }

    /* [Milliseconds] The current set maximum allowed time that a frame
       should be rendered in. */
    double m_frameTimeTargetMs = 1000.0 / 60.0;

    /* [Milliseconds] How much more time the CPU has spent sleeping than
       requested on the last frame. */
    double m_oversleptMs = 0.0;

    /* Contains the stopwatch used for timing the frame time. */
    std::chrono::steady_clock::time_point m_frameStart =
        std::chrono::steady_clock::now();

    /* Contains the stopwatch used for timing sleep periods. */
    std::chrono::steady_clock::time_point m_sleepStart {};
};
