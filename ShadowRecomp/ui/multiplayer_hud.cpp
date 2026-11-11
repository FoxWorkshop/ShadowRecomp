#include "multiplayer_hud.h"
#include <gpu/video.h>
#include <decompressor.h>
#include <patches/global.h>
#include <patches/multiplayer_mod_patches.h>
#include <res/images/common/mp_hud_ring.png.h>
#include <res/images/common/mp_hud_ammo.png.h>
#include <res/images/common/mp_hud_ammo_types.png.h>
#include <res/images/common/mp_hud_digits.png.h>

// Note: Might do the mission indicators/gauge/bosshealthbar later, but low priority

// 3P / 4P ring and ammo counters
static std::unique_ptr<GuestTexture> g_upTexRing;
static std::unique_ptr<GuestTexture> g_upTexAmmo;
static std::unique_ptr<GuestTexture> g_upTexAmmoTypes;
static std::unique_ptr<GuestTexture> g_upTexDigits;
struct HudRect
{
    float x, y, w, h;
};

static bool IsHudActive()
{
    auto base = g_memory.base;
    if (X86_LOAD_U8(0x54DE78) == 0)
    {
        return false;
    }
    const uint32_t manager = X86_LOAD_U32(0x5496E4);
    if (!manager)
    {
        return false;
    }
    switch (X86_LOAD_U32(manager + 0x8)) // hiding based on stageaction
    {
        case 2: // init
        case 3: // play
        case 7: // restart
        case 8: // continue
        case 9: // dead
            return true;
        default:
            return false;
    }
}

static float GetHudAlpha()
{
    auto base = g_memory.base;
    return 1.0f - std::clamp(float(X86_LOAD_F32(0x5394B8)), 0.0f, 1.0f);
}

static HudRect GetPlayerRect(const uint32_t number, const ImVec2& display)
{
    auto base = g_memory.base;
    const float screenW = float(int32_t(X86_LOAD_U32(0x4E5D88)));
    const float screenH = float(int32_t(X86_LOAD_U32(0x4E5D8C)));
    const uint32_t rect = 0x4E5D78 + (number + 1) * 0x30 + 0x8;
    const float x = float(int32_t(X86_LOAD_U32(rect)));
    const float y = float(int32_t(X86_LOAD_U32(rect + 0x4)));
    const float w = float(int32_t(X86_LOAD_U32(rect + 0x8)));
    const float h = float(int32_t(X86_LOAD_U32(rect + 0xC)));
    if (screenW > 0.0f && screenH > 0.0f && w > 0.0f && h > 0.0f)
    {
        return { x / screenW * display.x, y / screenH * display.y, w / screenW * display.x, h / screenH * display.y };
    }
    if (g_multiplayerPlayerCount == 4)
    {
        const float halfW = display.x / 2.0f;
        const float halfH = display.y / 2.0f;
        return { (number & 1) * halfW, (number >> 1) * halfH, halfW, halfH };
    }
    const float thirdW = display.x / 3.0f;
    return { number * thirdW, 0.0f, thirdW, display.y };
}

static void DrawIcon(ImDrawList* drawList, GuestTexture* texture, const ImVec2& min, const float size, const float alpha, const ImVec2& uvMin = { 0.0f, 0.0f }, const ImVec2& uvMax = { 1.0f, 1.0f })
{
    drawList->AddImage(texture, min, { min.x + size, min.y + size }, uvMin, uvMax, IM_COL32(255, 255, 255, int32_t(alpha * 255.0f)));
}

static void DrawAmmoIcon(ImDrawList* drawList, uint32_t weaponType, const ImVec2& min, const float size, const float alpha)
{
    static constexpr int32_t s_columns[] = { -1, 1, 2, 3, 4, 0, 5 };
    const int32_t column = weaponType < std::size(s_columns) ? s_columns[weaponType] : -1;
    if (column < 0)
    {
        DrawIcon(drawList, g_upTexAmmo.get(), min, size, alpha);
        return;
    }
    DrawIcon(drawList, g_upTexAmmoTypes.get(), min, size, alpha, { column / 6.0f, 0.0f }, { (column + 1) / 6.0f, 1.0f });
}

// matching the gradient the game applies to the numbers
static void DrawCounter(ImDrawList* drawList, float centerX, float y, float unit, uint32_t value, float alpha)
{
    char text[8];
    snprintf(text, sizeof(text), "%03u", std::min(value, 999u));
    const auto count = int32_t(strlen(text));
    const float glyphW = 16.0f * unit;
    const float glyphH = 18.0f * unit;
    float x = centerX - glyphW * count * 0.5f;
    const auto appliedAlpha = int32_t(alpha * 255.0f);
    const ImU32 light = IM_COL32(255, 255, 255, appliedAlpha);
    const ImU32 dark = IM_COL32(128, 128, 128, appliedAlpha);
    drawList->PushTextureID(g_upTexDigits.get());
    for (int32_t i = 0; i < count; i++, x += glyphW)
    {
        const int32_t digit = text[i] - '0';
        const ImVec2 uvMin = { ((digit % 4) * 16 + 0.5f) / 64.0f, ((digit / 4) * 18 + 0.5f) / 64.0f };
        const ImVec2 uvMax = { ((digit % 4 + 1) * 16 + 0.5f) / 64.0f, ((digit / 4 + 1) * 18 + 0.5f) / 64.0f };
        const ImDrawIdx index = ImDrawIdx(drawList->_VtxCurrentIdx);
        drawList->PrimReserve(6, 4);
        drawList->PrimWriteIdx(index);
        drawList->PrimWriteIdx(index + 1);
        drawList->PrimWriteIdx(index + 3);
        drawList->PrimWriteIdx(index + 1);
        drawList->PrimWriteIdx(index + 2);
        drawList->PrimWriteIdx(index + 3);
        drawList->PrimWriteVtx({ x, y }, uvMin, light);
        drawList->PrimWriteVtx({ x + glyphW, y }, { uvMax.x, uvMin.y }, dark);
        drawList->PrimWriteVtx({ x + glyphW, y + glyphH }, uvMax, light);
        drawList->PrimWriteVtx({ x, y + glyphH }, { uvMin.x, uvMax.y }, dark);
    }
    drawList->PopTextureID();
}

static void DrawPlayerCounters(ImDrawList* drawList, const HudRect& rect, const MultiplayerHudCounters& counters, const float alpha)
{
    const float unit = std::min(rect.w / 640.0f, rect.h / 480.0f);
    const float iconSize = 36.0f * unit;
    const float margin = 20.0f * unit;

    // draw on top slightly
    const float textY = rect.y + rect.h - margin - 18.0f * unit;
    const float iconY = textY + 7.0f * unit - iconSize;

    const float ringX = rect.x + margin;
    DrawIcon(drawList, g_upTexRing.get(), { ringX, iconY }, iconSize, alpha);
    DrawCounter(drawList, ringX + iconSize * 0.5f, textY, unit, counters.rings, alpha);
    if (counters.weaponType == 0)
    {
        return;
    }
    const float ammoX = rect.x + rect.w - margin - iconSize;
    DrawAmmoIcon(drawList, counters.weaponType, { ammoX, iconY }, iconSize, alpha);
    DrawCounter(drawList, ammoX + iconSize * 0.5f, textY, unit, counters.ammo, alpha);
}

void MultiplayerHud::Init()
{
    g_upTexRing = LOAD_ZSTD_TEXTURE(g_mpHudRing);
    g_upTexAmmo = LOAD_ZSTD_TEXTURE(g_mpHudAmmo);
    g_upTexAmmoTypes = LOAD_ZSTD_TEXTURE(g_mpHudAmmoTypes);
    g_upTexDigits = LOAD_ZSTD_TEXTURE(g_mpHudDigits);
}

void MultiplayerHud::Draw()
{
    if (!IsExtraPlayerMode() || !IsHudActive())
    {
        return;
    }
    const float alpha = GetHudAlpha();
    if (alpha <= 0.0f)
    {
        return;
    }
    const auto drawList = ImGui::GetBackgroundDrawList();
    const ImVec2 display = ImGui::GetIO().DisplaySize;
    for (uint32_t i = 0; i < g_multiplayerPlayerCount; i++)
    {
        MultiplayerHudCounters counters{};
        if (GetMultiplayerHudCounters(i, counters))
        {
            DrawPlayerCounters(drawList, GetPlayerRect(i, display), counters, alpha);
        }
    }
}
