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
     * \brief Sets the current framerate cap.
     * \param fpsLimit The framerate cap in frames per second. Values of zero
     * or below are ignored.
     */
    void SetFpsLimit(double fpsLimit);

    /**
     * \brief Marks the end of an individual frame/recurring piece of logic
     * to be performed/executed. You should put this at the end of a
     * reoccuring loop.
     * \param spin
     *     If true, uses an alternative timing method where CPU briefly spins
     *     (performs junk calculations) after sleeping slightly less time
     *     until it is precisely the time to start the next frame.
     * 
     *     Increases accuracy at the expense of negligible CPU load.
     * \param allowThreadYieldInSpin
     *     If true allows the thread to yield when spinning.
     *     Only set this to false if the CPU is maxed out (>95% usage).
     */
    void EndFrame(bool spin, bool allowThreadYieldInSpin = true);

private:
    /**
     * \brief Pauses execution for the remaining of the time until the next
     * frame begins.
     * \param durationMs The time in milliseconds to pause for.
     * \param spin See EndFrame.
     * \param allowThreadYieldInSpin See EndFrame.
     */
    void Sleep(double durationMs, bool spin, bool allowThreadYieldInSpin);

    /* Spins until it is time to begin the next frame. */
    void Spin(double durationMs, bool allowThreadYieldInSpin);

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
