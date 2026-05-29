#include <kernel/memory.h>
#include <ui/options_menu.h>

#include "aspect_ratio_patches.h"
#include <locale/locale.h>
#include <gpu/video.h>
#include <res/images/common/pause_00.png.h>
#include <res/images/common/pause_01.png.h>
#include <ui/imgui_utils.h>
#include <decompressor.h>
#include <gpu/imgui/imgui_snapshot.h>
#include <user/config.h>
#include <input/padstate.h>
#include <ui/menu_common.h>
#include <exports.h>


// We patch pause to add an extra Options in the menu
// This works by intercepting CSD data on the fly (see aspect_ratio_patches.cpp)
// Once we detect that the game is about to draw the pause text (resume, restart, quit etc.) we prevent it to display
// The options can then be re created through Imgui

enum PauseMode : uint8_t
{
	ModeRunning = 2,
	ModeQuit = 6,
	ModeContinue = 9,
	ModeOptions = 15
};

enum PauseSelection : uint8_t
{
	SelContinue = 0,
	SelRestart,
	SelOptions,
	SelQuit
};

static std::unique_ptr<GuestTexture> g_TexPauseText00{};
static std::unique_ptr<GuestTexture> g_TexPauseText01{};

static inline bool s_isVisible = false;
static inline bool s_quitRequest = false;
static inline bool s_OptionsRequest = false;
static inline bool s_quitOptionsRequest = false;
static int8_t s_index = 0;
static const uint8_t g_optionCount = 4;
static uint32_t g_pauseModeAddr = 0;
static bool g_up{};
static bool g_upWasHeld{};
static bool g_down{};
static bool g_downWasHeld{};
static bool g_left{};
static bool g_leftWasHeld{};
static bool g_right{};
static bool g_rightWasHeld{};
static bool g_isAccepted{};
static bool g_inputAllowed = false;

std::unordered_map<ELanguage, uint8_t> pauseTexMap =
{
	{ELanguage::English, 4},
	{ELanguage::Spanish, 1},
	{ELanguage::Japanese, 0},
	{ELanguage::German, 2},
	{ELanguage::French, 5},
	{ELanguage::Italian, 3},
};

void RequestQuitOptionForPause()
{
	s_quitOptionsRequest = true;
}
ImVec2 VirtualToScreen(float vx, float vy, ImVec2 displaySize)
{
	return {
		(vx / 640.0f) * displaySize.x,
		(vy / 480.0f) * displaySize.y
	};
}

static float ScaleX(float vWidth, float displayWidth) { return (vWidth / 640.0f) * displayWidth; }
static float ScaleY(float vHeight, float displayHeight) { return (vHeight / 480.0f) * displayHeight; }

void DrawPauseText()
{
	if (s_isVisible == false)
		return;


	bool upIsHeld = false;
	bool downIsHeld = false;
	bool leftIsHeld = false;
	bool rightIsHeld = false;

	

	if (auto pInputState = PJSPeripheral::GetControllerSlot(0))
	{
		if (g_inputAllowed && !s_OptionsRequest)
		{
			if (PJSPeripheral::IsHeld(0, PJSPeripheral::BTN_DPAD_UP) || -pInputState->leftY > 0.5f)
				upIsHeld = true;

			if (!g_upWasHeld && upIsHeld)
				g_up = true;

			if (PJSPeripheral::IsHeld(0, PJSPeripheral::BTN_DPAD_DOWN) || -pInputState->leftY < -0.5f)
				downIsHeld = true;

			if (!g_downWasHeld && downIsHeld)
				g_down = true;

			if (PJSPeripheral::IsHeld(0, PJSPeripheral::BTN_DPAD_LEFT) || -pInputState->leftX > 0.5f)
				leftIsHeld = true;

			if (!g_leftWasHeld && leftIsHeld)
				g_left = true;

			if (PJSPeripheral::IsHeld(0, PJSPeripheral::BTN_DPAD_RIGHT) || -pInputState->leftX < -0.5f)
				rightIsHeld = true;

			if (!g_rightWasHeld && rightIsHeld)
				g_right = true;

			if (PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_A) || PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_START))
				g_isAccepted = true;
		}
	}



	auto scrollUp = g_up;
	auto scrollDown = g_down;
	if (g_inputAllowed && !s_OptionsRequest)
	{
		if (scrollUp)
		{
			--s_index;

			if (s_index < SelContinue)
			{
				Game_PlaySound("cursor");
				s_index = g_optionCount - 1;
			}
			else if (s_index == SelOptions)
			{
				Game_PlaySound("cursor");
			}
		}
		else if (scrollDown)
		{
			++s_index;

			if (s_index == SelQuit)
			{
				Game_PlaySound("cursor");
			}
			else if (s_index >= g_optionCount)
			{
				Game_PlaySound("cursor");
				s_index = SelContinue;
			}
		}
		else if (g_isAccepted && s_index == SelQuit)
		{

			s_quitRequest = true;
		}
		else if (g_isAccepted && s_index == SelOptions)
		{
			Game_PlaySound("decide");
			s_OptionsRequest = true;
		}
	}


	auto drawList = ImGui::GetBackgroundDrawList();
	auto& res = ImGui::GetIO().DisplaySize;
	ImVec2 screenCentre = { res.x / 2.0f, res.y / 2.0f };
	
	// Continue / Resume

	// virtual size of the texture sprite
	float vWidth = 120.0f;
	float vHeight = 26.0f;

	const float offsetY = 30.f;

	auto uvs = PIXELS_TO_UV_COORDS(256, 256, 0, offsetY * pauseTexMap.find(Config::Language.Value)->second, 100, 27);
	float virtualItemSpacing = 20.0f; // pixels in 640x480 space
	float virtualStartY = 125.f;

	// convert to screen space, centered horizontally
	float screenW = ScaleX(vWidth, res.x);
	float screenH = ScaleY(vHeight, res.y);
	float screenY = VirtualToScreen(0, virtualStartY, res).y;

	ImVec2 itemMin = { screenCentre.x - screenW / 2.0f, screenY };
	ImVec2 itemMax = { itemMin.x + screenW, itemMin.y + screenH };

	drawList->AddImage(g_TexPauseText00.get(), itemMin, itemMax,
		GET_UV_COORDS(uvs), IM_COL32(s_index == SelContinue ? 255 : 127, s_index == SelContinue ? 255 : 127, s_index == SelContinue ? 255 : 127, 255));

	// Restart 
	screenY = VirtualToScreen(0, virtualStartY + 25.f, res).y;

	vWidth = 130.0f;
	screenW = ScaleX(vWidth, res.x);
	itemMin = { screenCentre.x - screenW / 2.0f, screenY };
	itemMax = { itemMin.x + screenW, itemMin.y + screenH };

	drawList->AddImage(g_TexPauseText01.get(), itemMin, itemMax,
		GET_UV_COORDS(uvs), IM_COL32(s_index == SelRestart ? 255 : 127, s_index == SelRestart ? 255 : 127, s_index == SelRestart ? 255 : 127, 255));

	// Options

	uint32_t uvLanguagePos =  pauseTexMap.find(Config::Language.Value)->second;
	uvs = PIXELS_TO_UV_COORDS(256, 256, 159, offsetY * uvLanguagePos, 100, 27);
	screenY = VirtualToScreen(0, virtualStartY + 50.f, res).y;

	screenW = ScaleX(vWidth, res.x);
	itemMin = { screenCentre.x - screenW / 2.0f, screenY };
	itemMax = { itemMin.x + screenW, itemMin.y + screenH };

	drawList->AddImage(g_TexPauseText00.get(), itemMin, itemMax,
		GET_UV_COORDS(uvs), IM_COL32(s_index == SelOptions ? 255 : 127, s_index == SelOptions ? 255 : 127, s_index == SelOptions ? 255 : 127, 255));

	// Quit

	screenY = VirtualToScreen(0, virtualStartY + 75.f, res).y;

	itemMin = { screenCentre.x - screenW / 2.0f, screenY };
	itemMax = { itemMin.x + screenW, itemMin.y + screenH };

	drawList->AddImage(g_TexPauseText01.get(), itemMin, itemMax,
		GET_UV_COORDS(uvs), IM_COL32(s_index == SelQuit ? 255 : 127 , s_index == SelQuit ? 255 : 127, s_index == SelQuit ? 255 : 127, 255));


	g_up = false;
	g_upWasHeld = upIsHeld;
	g_down = false;
	g_downWasHeld = downIsHeld;
	g_left = false;
	g_leftWasHeld = leftIsHeld;
	g_right = false;
	g_rightWasHeld = rightIsHeld;
	g_isAccepted = false;
}

// Draw Pause Menu
X86_FUNC_IMPL(__imp__sub_281A70);
X86_FUNC(sub_281A70)
{
	__imp__sub_281A70(ctx, base);
	auto isValid = X86_LOAD_U8(0x54DE78);
	if (isValid)
	{
		DrawPauseText();
	}
}

// TPauseManagerExec
X86_FUNC_IMPL(__imp__sub_5AEE0);
X86_FUNC(sub_5AEE0)
{
	auto flagdisabled = X86_LOAD_U8(ctx.ecx.u32 + 0x3C);

	g_pauseModeAddr = ctx.ecx.u32 + 0x30;
	auto mode = X86_LOAD_U32(g_pauseModeAddr);

	if (mode == 2 && g_inputAllowed == false)
	{
		g_inputAllowed = true;
	}


	// handle new options in pause menu by tricking the game that we shouldn't quit
	if (!s_quitOptionsRequest && s_index == SelOptions && s_OptionsRequest && GameMenuCommon::g_OptionsOpen == false)
	{
		OptionsMenu::Open(true);
		GameMenuCommon::g_OptionsOpen = true;

		//X86_STORE_U32(g_pauseModeAddr, ModeOptions); // custom mode for options
	
		s_isVisible = false;
		ctx.esp.u32 += 4;
		
		return;
	}
	else if (mode == ModeContinue)
	{
		g_inputAllowed = false;
		s_index = 0;
	}
	else if (s_OptionsRequest)
	{
		if (OptionsMenu::s_isVisible == false)
		{
			X86_STORE_U32(g_pauseModeAddr, ModeRunning);
			s_OptionsRequest = false;
			s_quitOptionsRequest = false;
			s_isVisible = true;
		}
	}
	else if (s_quitRequest)
	{
		s_quitRequest = false;
		X86_STORE_U32(g_pauseModeAddr, ModeQuit);
	}


	__imp__sub_5AEE0(ctx, base);

	s_isVisible = !flagdisabled && mode == ModeRunning;
}

// This function check input and set cursor for the pause menu
// Because we add a new option not intended, we don't want this to mess with the real cursor pose 
// Thus we want this function to not run once our cursor reach higher index than vanilla.
X86_FUNC_IMPL(__imp__sub_5AAF0);
X86_FUNC(sub_5AAF0)
{

	if (s_index != SelOptions && s_index != SelQuit && !s_quitRequest)
	{
		__imp__sub_5AAF0(ctx, base);

	}
	else
	{
		ctx.esp.u32 += 4;
	}



}


void PausePatchesInit()
{
	g_TexPauseText00 = LOAD_ZSTD_TEXTURE(g_pauseText00);
	g_TexPauseText01 = LOAD_ZSTD_TEXTURE(g_pauseText01);
}