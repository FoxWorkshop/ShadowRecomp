#pragma once

#include <user/config.h>

enum ELedAura
{
    LED_AURA_NEUTRAL = 0, 
    LED_AURA_HERO,
    LED_AURA_VILAIN
};

class App
{
public:
    static inline bool s_isInit;
    static inline bool s_isLoading;
    static inline bool s_isSaving;
	static inline ELedAura s_chaosMode = LED_AURA_NEUTRAL;

    static inline ELanguage s_language;
    static inline double s_deltaTime;
    static inline double s_time = 0.0; // How much time elapsed since the game started.

    static void Restart(std::vector<std::string> restartArgs = {});
    static void Exit();
};
