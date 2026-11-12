#include "shadow.h"

#include <user/config.h>
#include <patches/fps_patches.h>
#include <patches/guest_variable.h>

#include "utilities/menu_name_dictionary.h"
#include "utilities/mode_name_dictionary.h"
#include "utilities/stage_name_dictionary.h"

namespace RichPresence
{
    /* Pointers */
    static const GuestVariable<int32_t>  g_stageId{ 0x54DDD8 };          // GC: 0x805EF958
    static const GuestVariable<uint32_t> g_menuManager{ 0x54BA10 };      // GC: 0x80583ACC
    static const GuestVariable<int32_t>  g_currentMode{ 0x5022A4 };      // GC: 0x805EC170
    static const GuestVariable<uint32_t> g_missionManager{ 0x541FA4 };   // GC: 0x80575F1F
    static const GuestVariable<uint8_t>  g_isGameplayFrozen{ 0x5415A0 }; // GC: 0x8057D768
    static const GuestVariable<float>    g_stageTimer{ 0x54DDC4 };       // GC: 0x8057D734
    static const GuestVariable<uint32_t> g_movieManager{ 0x541808 };

    StageID Shadow::StageId()
    {
        return (StageID)(int32_t)g_stageId;
    }

    std::optional<MenuState> Shadow::GetMenuState()
    {
        // The menu manager is created on first use, so it can be missing early on boot.
        uint32_t menuManager = g_menuManager;
        if (menuManager == 0)
            return std::nullopt;

        return (MenuState)(int32_t)GuestVariable<int32_t>{ menuManager };
    }

    Modes Shadow::CurrentMode()
    {
        return (Modes)(int32_t)g_currentMode;
    }

    CurrentMission Shadow::GetCurrentMission()
    {
        uint32_t missionManager = g_missionManager;
        if (missionManager == 0)
            return CurrentMission::Neutral;

        return (CurrentMission)(uint32_t)GuestVariable<uint32_t>{ missionManager + 0x20 };
    }

    // GC: 0x8057FA85.
    bool Shadow::IsWatchingCutscene()
    {
        return IsEventPlaying() || g_movieManager != 0;
    }

    bool Shadow::IsGameplayFrozen()
    {
        return g_isGameplayFrozen != 0;
    }

    float Shadow::StageTimer()
    {
        return g_stageTimer;
    }

    /*
        ---------------
        Utility Methods
        ---------------
    */

    std::string Shadow::GetStageName()
    {
        if (auto stageName = StageNameDictionary::TryGetValue(StageId()))
            return stageName;

        return "";
    }

    std::string Shadow::GetMenuStateName()
    {
        if (auto value = GetMenuState())
            if (auto menuState = MenuNameDictionary::TryGetValue(*value))
                return menuState;

        return "";
    }

    std::string Shadow::GetModeName()
    {
        if (auto modeState = ModeNameDictionary::TryGetValue(CurrentMode()))
            return modeState;

        return "";
    }

    std::string Shadow::GetCurrentMissionName()
    {
        switch (GetCurrentMission())
        {
            case CurrentMission::Dark:    return "Dark Mission";
            case CurrentMission::Neutral: return "Neutral Mission";
            case CurrentMission::Hero:    return "Hero Mission";
        }

        return "";
    }
}
