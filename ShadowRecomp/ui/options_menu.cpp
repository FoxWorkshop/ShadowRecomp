#include <ui/options_menu.h>
#include <locale/locale.h>
#include <app.h>
#include <exports.h>
#include <gpu/video.h>
#include <res/images/common/TitleBg.png.h>
#include <decompressor.h>
#include <patches/aspect_ratio_patches.h>
#include <user/config.h>
#include "imgui_utils.h"

static std::unique_ptr<GuestTexture> g_upTexOptions{};

static std::string& GetCategoryName(OptionsMenuCategory category)
{
    switch (category)
    {
    case OptionsMenuCategory::System: return Localise("Options_Category_System");
    case OptionsMenuCategory::Input: return Localise("Options_Category_Input");
    case OptionsMenuCategory::Audio: return Localise("Options_Category_Audio");
    case OptionsMenuCategory::Video: return Localise("Options_Category_Video");
    case OptionsMenuCategory::Misc: return Localise("Options_Category_Misc");
    case OptionsMenuCategory::Debug: return Localise("Options_Category_Debug");
    }

    return g_localeMissing;
}

static std::string& GetCategoryDescription(OptionsMenuCategory category)
{
    switch (category)
    {
    case OptionsMenuCategory::System: return Localise("Options_Desc_Category_System");
    case OptionsMenuCategory::Input: return Localise("Options_Desc_Category_Input");
    case OptionsMenuCategory::Audio: return Localise("Options_Desc_Category_Audio");
    case OptionsMenuCategory::Video: return Localise("Options_Desc_Category_Video");
    case OptionsMenuCategory::Misc: return Localise("Options_Desc_Category_Misc");
    case OptionsMenuCategory::Debug: return Localise("Options_Desc_Category_Debug");
    }

    return g_localeMissing;
}

void DrawBackground()
{
    auto& res = ImGui::GetIO().DisplaySize;
    auto drawList = ImGui::GetBackgroundDrawList();

    // Full screen stretch
    ImVec2 min = { g_aspectRatioOffsetX, g_aspectRatioOffsetY };
    ImVec2 max = { res.x - g_aspectRatioOffsetX, res.y - g_aspectRatioOffsetY };

    auto lightOffUVs = PIXELS_TO_UV_COORDS(512, 512, 0, 0, 512, 512);
    drawList->AddImage(g_upTexOptions.get(), min, max, GET_UV_COORDS(lightOffUVs), IM_COL32(255, 255, 255, 255));
}

void OptionsMenu::Draw()
{
    if (!s_isVisible)
        return;


    DrawBackground();
}

void OptionsMenu::Open(bool isPause)
{
    s_isVisible = true;

}

// to do
void OptionsMenu::Close()
{
    Config::Save();
}

bool OptionsMenu::CanClose()
{
    return true; //to do
}

void OptionsMenu::Init()
{
    g_upTexOptions = LOAD_ZSTD_TEXTURE(g_options_bg);
}
