/*
    Timer Resolution

        Raises the platform timer resolution and reports the achieved
        granularity, used by the frame pacer to decide when sleeping is
        accurate enough.
*/

#pragma once

class TimerResolution
{
public:
    /**
     * \brief Raises the timer resolution to the finest value the platform
     * supports. Safe to call again; only the first successful call applies.
     * \return True when the platform supports raising the resolution.
     */
    static bool SetHighestResolution();

    /**
     * \brief Gets the timer granularity in milliseconds. Raises the
     * resolution on first call if SetHighestResolution was never called.
     * \return The timer granularity in milliseconds; 1.0 when the platform
     * does not support raising it.
     */
    static double GetGranularityMs();

    /**
     * \brief Releases the timer resolution request made by
     * SetHighestResolution. No-op when the resolution was never raised.
     */
    static void Release();

private:
    /* Achieved granularity in milliseconds; 1.0 until proven finer. */
    static double s_granularityMs;

    /* Whether SetHighestResolution successfully raised the resolution. */
    static bool s_isActive;

    /* Whether a raise was attempted, so failed attempts are not retried. */
    static bool s_isResolved;
};
