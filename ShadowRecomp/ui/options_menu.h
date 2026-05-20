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

enum class OptionsMenuFlowState
{
    CategoryCursor,
    OptionCursor,
    OptionSelected
};


class OptionsMenu
{
public:
    static inline OptionsMenuState s_state{};
    static inline OptionsMenuFlowState s_flowState{};
    static inline bool s_isVisible = false;
    static inline bool s_isPause = false;
    static inline bool s_isRestartRequired = false;
    static inline bool s_isDebugUnlocked = false;
    static inline bool s_GoToLegacyMenu = false;


    static void Init();
    static void Draw();
    static void Open(bool isPause = false);
    static void Close();

    static bool IsRestartRequired();
    static void SetFlowState(OptionsMenuFlowState flowState);

    static bool CanClose();
};