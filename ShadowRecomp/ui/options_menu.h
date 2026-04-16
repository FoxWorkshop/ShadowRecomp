#pragma once

enum class OptionsMenuState
{
    Opening,
    Idle,
    Closing,
    Restarting
};

enum class OptionsMenuCategory
{
    System,
    Input,
    Audio,
    Video,
    Misc,
    Debug,
    Count
};

class OptionsMenu
{
public:
    static inline bool s_isVisible = false;
    static inline bool s_isPause = false;
    static inline bool s_isRestartRequired = false;



    static void Init();
    static void Draw();
    static void Open(bool isPause = false);
    static void Close();

    static bool CanClose();
};