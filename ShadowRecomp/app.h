#pragma once

class App
{
public:
    static inline bool s_isInit;
    static inline bool s_isLoading;
    static inline bool s_isSaving;

    static inline double s_deltaTime;
    static inline double s_time = 0.0; // How much time elapsed since the game started.
    static void Exit();
};
