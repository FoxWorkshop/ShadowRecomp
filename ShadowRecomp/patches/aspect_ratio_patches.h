#pragma once

#include <xxHashMap.h>

#define MAKE_BITFLAG32(bit) 1U << bit
#define MAKE_BITFLAG64(bit) 1ULL << bit

inline constexpr float NARROW_ASPECT_RATIO = 4.0f / 3.0f;
inline constexpr float WIDE_ASPECT_RATIO = 16.0f / 9.0f;
inline constexpr float STEAM_DECK_ASPECT_RATIO = 16.0f / 10.0f;

inline float g_aspectRatio;
inline float g_aspectRatioOffsetX;
inline float g_aspectRatioOffsetY;
inline float g_aspectRatioScale;
inline float g_aspectRatioGameplayScale;
inline float g_aspectRatioNarrowScale;
inline float g_aspectRatioNarrowMargin;
inline float g_horzCentre;
inline float g_vertCentre;

// 1.0 at 4:3 and below, 0.75 at 16:9, ~0.5625 at 21:9.
inline float g_hudUnstretchX = 1.0f;

struct AspectRatioPatches
{
    static void ComputeOffsets();
};
void UpdateUserConfigConstant(bool toggle);

// Widens the world matrix of a bitmap-font string back to the vanilla 4:3 layout.
// No-op unless the current draw opted in (see s_unstretchMenuText in aspect_ratio_patches.cpp).
void UnstretchMenuTextTransform(uint32_t pMatrix);

// -------------- CSD MODIFIERS --------------- //

enum CsdFlags : uint64_t
{
    CSD_ALIGN_CENTER = 0,

    CSD_ALIGN_TOP = MAKE_BITFLAG64(0),
    CSD_ALIGN_LEFT = MAKE_BITFLAG64(1),
    CSD_ALIGN_BOTTOM = MAKE_BITFLAG64(2),
    CSD_ALIGN_RIGHT = MAKE_BITFLAG64(3),

    CSD_ALIGN_TOP_LEFT = CSD_ALIGN_TOP | CSD_ALIGN_LEFT,
    CSD_ALIGN_TOP_RIGHT = CSD_ALIGN_TOP | CSD_ALIGN_RIGHT,
    CSD_ALIGN_BOTTOM_LEFT = CSD_ALIGN_BOTTOM | CSD_ALIGN_LEFT,
    CSD_ALIGN_BOTTOM_RIGHT = CSD_ALIGN_BOTTOM | CSD_ALIGN_RIGHT,

    CSD_STRETCH_HORIZONTAL = MAKE_BITFLAG64(4),
    CSD_STRETCH_VERTICAL = MAKE_BITFLAG64(5),

    CSD_STRETCH = CSD_STRETCH_HORIZONTAL | CSD_STRETCH_VERTICAL,

    // Scale the element uniformly around the centre of the screen so a whole layout keeps its 4:3
    // proportions, whatever the user picked for UI alignment and whatever the display resolution
    // is. Meant for screens that read as one picture (menus, results, pause) rather than for HUD
    // widgets, which follow the alignment setting instead. Usually set for a whole file through
    // g_csdFileModifiers; CSD_STRETCH_HORIZONTAL on an individual cast opts it back out.
    CSD_SCALE = MAKE_BITFLAG64(6),

    CSD_EXTEND_LEFT = MAKE_BITFLAG64(7),
    CSD_EXTEND_RIGHT = MAKE_BITFLAG64(8),

    CSD_STORE_LEFT_CORNER = MAKE_BITFLAG64(9),
    CSD_STORE_RIGHT_CORNER = MAKE_BITFLAG64(10),

    CSD_SKIP = MAKE_BITFLAG64(11),

    CSD_OFFSET_SCALE_LEFT = MAKE_BITFLAG64(12),
    CSD_OFFSET_SCALE_RIGHT = MAKE_BITFLAG64(13),

    CSD_REPEAT_LEFT = MAKE_BITFLAG64(14),
    CSD_REPEAT_RIGHT = MAKE_BITFLAG64(15),
    CSD_REPEAT_FLIP_HORIZONTAL = MAKE_BITFLAG64(16),
    CSD_REPEAT_FLIP_VERTICAL = MAKE_BITFLAG64(17),
    CSD_REPEAT_EXTEND = MAKE_BITFLAG64(18),

    CSD_UV_MODIFIER = MAKE_BITFLAG64(19),
    CSD_COLOUR_MODIFIER = MAKE_BITFLAG64(20),
    CSD_REPEAT_UV_MODIFIER = MAKE_BITFLAG64(21),
    CSD_REPEAT_COLOUR_MODIFIER = MAKE_BITFLAG64(22),

    CSD_BLACK_BAR = MAKE_BITFLAG64(23),
    CSD_PROHIBIT_BLACK_BAR = MAKE_BITFLAG64(24),

    CSD_UNSTRETCH_HORIZONTAL = MAKE_BITFLAG64(25),

    CSD_CORNER_EXTRACT = MAKE_BITFLAG64(26),

    CSD_RADARMAP = MAKE_BITFLAG64(27),

    CSD_POD_BASE = MAKE_BITFLAG64(28),
    CSD_POD_CLONE = MAKE_BITFLAG64(29),

    CSD_MULTIPLAYER = MAKE_BITFLAG64(30),

    CSD_CHEVRON = MAKE_BITFLAG64(31),

    CSD_MODIFIER_ULTRAWIDE_ONLY = MAKE_BITFLAG64(32),
    CSD_MODIFIER_NARROW_ONLY = MAKE_BITFLAG64(33),

	CSD_MODIFIER_PRESEVE_4_3_SCALE = MAKE_BITFLAG64(34),
	CSD_MODIFIER_CB = MAKE_BITFLAG64(35),


    CSD_BUTTON_WINDOW = MAKE_BITFLAG64(41),
    CSD_HIDE_PAUSE = MAKE_BITFLAG64(42),
    CSD_PAUSE_BOSS = MAKE_BITFLAG64(43),

    CSD_EXTEND_BOTTOM = MAKE_BITFLAG64(44),
    CSD_MOVE_BOTTOM = MAKE_BITFLAG64(45),

    // Unstretch a primitive even though it spans the whole virtual screen, for the rare
    // full-width element that is a widget rather than a background.
    CSD_FORCE_UNSTRETCH = MAKE_BITFLAG64(46),
};


struct CsdModifier
{
    int64_t Flags{};
    /**CsdUVs UVs{};
    CsdColours Colours{};
    CsdUVs RepeatUVs{};
    CsdColours RepeatColours{};*/
    float CornerMax{};
    uint32_t CornerIndex{};
};

std::optional<CsdModifier> FindCsdModifier(uint32_t data);