#include "achievement_menu.h"
#include <gpu/imgui/imgui_snapshot.h>
#include <gpu/video.h>
#include <input/input.h>
#include <locale/achievement_locale.h>
#include <locale/locale.h>
#include <patches/aspect_ratio_patches.h>
#include <ui/imgui_utils.h>
#include <res/images/common/arrow.png.h>
#include <res/images/common/achievementBG.png.h>
#include <res/images/common/achievementBtn.png.h>
#include <decompressor.h>
#include <user/config.h>
#include <app.h>
#include <exports.h>
#include <user/achievement_data.h>
#include <input/padstate.h>

static double g_time{};
static double g_rowSelectionTime{};
static double g_scrollArrowsTime{};
static double g_lastIncrementTime{};
static double g_lastTappedTime{};

static bool g_up{};
static bool g_upWasHeld{};
static bool g_down{};
static bool g_downWasHeld{};
static bool g_hasSwitched{};
static bool g_isDeclined{};

static int g_rowCount{};
static int g_selectedIndex{};

static std::unique_ptr<GuestTexture> g_TexAchievementBG{};
static std::unique_ptr<GuestTexture> g_TexAchievementBtn{};
static std::vector<std::tuple<Achievement, time_t>> g_achievements{};

static void MoveCursor(int& cursorIndex, int min = 0, int max = INT_MAX)
{
    auto time = ImGui::GetTime();

    auto scrollUp = g_up;
    auto scrollDown = g_down;

    if (scrollUp || scrollDown)
        g_lastTappedTime = time;

    static constexpr auto FAST_SCROLL_THRESHOLD = 0.3;
    static constexpr auto FAST_SCROLL_SPEED = 1.0 / 6.5;

    auto fastScroll = (time - g_lastTappedTime) > FAST_SCROLL_THRESHOLD;

    if (fastScroll)
    {
        if ((time - g_lastIncrementTime) < FAST_SCROLL_SPEED)
        {
            fastScroll = false;
        }
        else
        {
            g_lastIncrementTime = time;

            scrollUp = g_upWasHeld;
            scrollDown = g_downWasHeld;
        }
    }

    if (scrollUp)
    {
        --cursorIndex;

        if (cursorIndex < min)
            cursorIndex = max - 1;
    }
    else if (scrollDown)
    {
        ++cursorIndex;

        if (cursorIndex >= max)
            cursorIndex = min;
    }

    if (scrollUp || scrollDown)
    {
        Game_PlaySound("move");

        g_rowSelectionTime = time;
        g_scrollArrowsTime = g_rowSelectionTime;
    }
}

void AchievementMenu::Open()
{

    if (s_isVisible)
        return;

    g_achievements.clear();

    /**for (auto& achievement : g_xdbfWrapper.GetAchievements((EXDBFLanguage)Config::Language.Value))
    {
        achievement.LockedDesc = xdbf::FixInvalidSequences(achievement.LockedDesc);

        g_achievements.push_back(std::make_tuple(achievement, AchievementManager::GetTimestamp(achievement.ID)));
    }*/

    std::sort(g_achievements.begin(), g_achievements.end(), [](const auto& a, const auto& b)
        {
            return std::get<1>(a) > std::get<1>(b);
        });

    g_rowCount = g_achievements.size();


    s_isVisible = true;
    s_state = AchievementMenuState::Transition;
	s_nextState = AchievementMenuState::Achievements;
    g_time = ImGui::GetTime();
    g_hasSwitched = false;
    g_selectedIndex = 0;

}


void AchievementMenu::Close()
{
    if (s_state == AchievementMenuState::ClosingAchievements)
        return;

    s_state = AchievementMenuState::ClosingAchievements;
    g_time = ImGui::GetTime();

}

static void DrawBackground()
{
    auto& res = ImGui::GetIO().DisplaySize;
    auto drawList = ImGui::GetBackgroundDrawList();

    ImVec2 centre = { res.x / 2.f, res.y / 2.f };

    float scale = 1.0f;

    if (AchievementMenu::s_state == AchievementMenuState::ClosingAchievements)
        scale = (float)ComputeEaseOut(AchievementMenu::g_BackgroundAnimClose, 0.5, true);
    else
        scale = (float)ComputeEaseOut(AchievementMenu::g_BackgroundAnimIntro, 0.5, false);

    // animate from centre outward to fullscreen
    ImVec2 bgHalfSize = { (res.x / 2.f) * scale, (res.y / 2.f) * scale };
    ImVec2 bgMin = { centre.x - bgHalfSize.x, centre.y - bgHalfSize.y };
    ImVec2 bgMax = { centre.x + bgHalfSize.x, centre.y + bgHalfSize.y };

    drawList->AddImage
    (
        g_TexAchievementBG.get(),
        bgMin,
        bgMax,
        { 0, 0},
        { 1, 1},
        IM_COL32(255, 255, 255, 255)
    );

    if (scale <= 0.1f && AchievementMenu::s_state == AchievementMenuState::ClosingAchievements)
    {
        AchievementMenu::s_isVisible = false;
    }
}

static bool CheckAndDiscard(bool& value)
{
    if (value)
    {
        value = false;
        return true;
    }

    return false;
}

void AchievementMenu::Draw()
{

    if (!s_isVisible)
        return;

    auto drawList = ImGui::GetBackgroundDrawList();

    switch (s_state)
    {
    case AchievementMenuState::Transition:
    {

        if (AchievementMenu::s_nextState == AchievementMenuState::Achievements)
            AchievementMenu::g_BackgroundAnimIntro = ImGui::GetTime();

        if (AchievementMenu::s_nextState == AchievementMenuState::ClosingAchievements)
        {
            AchievementMenu::g_BackgroundAnimClose = ImGui::GetTime();
            AchievementMenu::Close();
        }
        else
        {
            s_state = AchievementMenu::s_nextState;
        }


        break;
    }
    case AchievementMenuState::Achievements:
    {

       // DrawContainer(min, max);

        //if (containerMotionTime >= 1.0)
        {
            auto rowIndex = 0;

            for (auto& tpl : g_achievements)
            {
                auto& achievement = std::get<0>(tpl);

                /**if (AchievementManager::IsUnlocked(achievement.ID))
                    DrawAchievement(rowIndex++, achievement, true);*/
            }

            for (auto& tpl : g_achievements)
            {
                auto& achievement = std::get<0>(tpl);

                /**if (!AchievementManager::IsUnlocked(achievement.ID))
                    DrawAchievement(rowIndex++, achievement, false);*/
            }

  

            auto scrollArrowsOffsetX = Scale(17);
            auto scrollArrowsOffsetTop = Scale(40);
            auto scrollArrowsOffsetBottom = Scale(60);
           // ImVec2 scrollArrowsMin = { max.x + scrollArrowsOffsetX, min.y + scrollArrowsOffsetTop };
           // ImVec2 scrollArrowsMax = { scrollArrowsMin.x + scrollArrowsOffsetX, max.y - scrollArrowsOffsetBottom };

            //DrawScrollArrows(scrollArrowsMin, scrollArrowsMax, Scale(25), g_scrollArrowsTime, g_selectedIndex > MAX_VISIBLE_ROWS / 2, g_selectedIndex < g_rowCount - MAX_VISIBLE_ROWS / 2);

            auto upIsHeld = false;
            auto downIsHeld = false;

            if (auto pInputState = PJSPeripheral::GetControllerSlot(0))
            {


                if (PJSPeripheral::IsHeld(0, PJSPeripheral::BTN_DPAD_UP) || -pInputState->leftY > 0.5f)
                    upIsHeld = true;

                if (!g_upWasHeld && upIsHeld)
                    g_up = true;

                if (PJSPeripheral::IsHeld(0, PJSPeripheral::BTN_DPAD_DOWN) || -pInputState->leftY < -0.5f)
                    downIsHeld = true;

                if (!g_downWasHeld && downIsHeld)
                    g_down = true;


                if (PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_B) || PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_Y))
                    g_isDeclined = true;

            }

            if (CheckAndDiscard(g_isDeclined))
            {
                s_state = AchievementMenuState::Transition;
                s_nextState = AchievementMenuState::ClosingAchievements;
            }

            MoveCursor(g_selectedIndex, 0, g_rowCount);



            g_up = false;
            g_upWasHeld = upIsHeld;
            g_down = false;
            g_downWasHeld = downIsHeld;
        }


        break;
    }

        break;


    }


    DrawBackground();

}

void AchievementMenu::Init()
{
    g_TexAchievementBG = LOAD_ZSTD_TEXTURE(g_achievementBG);
    g_TexAchievementBtn = LOAD_ZSTD_TEXTURE(g_achievementBtn);
}