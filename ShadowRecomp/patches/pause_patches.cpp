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

static const uint8_t pauseItemsCount = 4u;
static const uint8_t addedPauseItemsCount = 1u;

enum PauseMode : uint8_t
{
	ModeRunning = 2,
	ModeRestart = 5,
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


static bool g_pauseCSD = false; // ensure our CSD patch worked
static bool g_pauseBoss = false; // CSD pause for boss is different and require pos adjust
static std::unique_ptr<GuestTexture> g_TexPauseText00{};
static std::unique_ptr<GuestTexture> g_TexPauseText01{};
static ImFont* g_fntDFPZong;

static inline bool s_isVisible = false;
static inline bool s_quitRequest = false;
static inline bool s_OptionsRequest = false;
static inline bool s_quitOptionsRequest = false;
static int8_t s_index = 0;
static const uint8_t g_optionCount = 4;
static uint32_t g_pauseModeAddr = 0;
static uint32_t g_CursorModeAddr = 0;
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

void TogglePauseCSD(bool toggle)
{
	g_pauseCSD = toggle;
}

void TogglePauseBossCSD(bool toggle)
{
	g_pauseBoss = toggle;
}

const uint8_t GetAddedPauseItemsCount()
{
	return addedPauseItemsCount;
}

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

void AddTextWithOutline(ImDrawList* drawList, ImFont* font, float fontSize,
	ImVec2 pos, ImU32 textColor, ImU32 outlineColor,
	const char* text, float outlineSize = 2.0f)
{
	// two passes for thicker outline
	for (float thickness = 1.0f; thickness <= outlineSize; thickness += 1.0f)
	{
		for (int dx = -1; dx <= 1; dx++)
		{
			for (int dy = -1; dy <= 1; dy++)
			{
				if (dx == 0 && dy == 0) continue;
				drawList->AddText(font, fontSize,
					ImVec2(pos.x + dx * thickness, pos.y + dy * thickness),
					outlineColor, text);
			}
		}
	}
	drawList->AddText(font, fontSize, pos, textColor, text);
}

void AddTextCentered(ImDrawList* drawList, ImFont* font, float fontSize,
	float centerX, float y, ImU32 textColor, ImU32 outlineColor,
	const char* text)
{
	ImVec2 textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text);
	ImVec2 pos = { centerX - textSize.x / 2.0f, y };
	AddTextWithOutline(drawList, font, fontSize, pos, textColor, outlineColor, text);
}
static float ScaleX(float vWidth, float displayWidth) { return (vWidth / 640.0f) * displayWidth; }
static float ScaleY(float vHeight, float displayHeight) { return (vHeight / 480.0f) * displayHeight; }

void DrawPauseText()
{
	if (s_isVisible == false || g_pauseCSD == false) // ensure our CSD patch worked so we can draw pause text safely
		return;

	if (g_pauseModeAddr)
	{
		auto base = g_memory.base;
		auto mode = X86_LOAD_U32(g_pauseModeAddr);
		if (mode == ModeQuit || mode == ModeRestart)
		{
			return;
		}
	}


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
				s_index = g_optionCount - 1;
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

			if (s_index >= g_optionCount)
			{
				s_index = SelContinue;
				Game_PlaySound("cursor");
			}
		}

		if (g_isAccepted)
		{
			switch (s_index)
			{
			case SelOptions:
				Game_PlaySound("decide");
				s_OptionsRequest = true;
				break;
			case SelQuit:
				s_quitRequest = true;
				break;
			}
		}
	}


	auto drawList = ImGui::GetBackgroundDrawList();
	auto& res = ImGui::GetIO().DisplaySize;
	ImVec2 screenCentre = { res.x / 2.0f, res.y / 2.0f };
	
	// virtual size of the texture sprite
	float vWidth = 120.0f;
	float vHeight = 26.0f;

	const float offsetY = 30.f;

	float virtualItemSpacing = 20.0f; // pixels in 640x480 space
	float virtualStartY = g_pauseBoss ? 200.f : 125.f;

	// convert to screen space, centered horizontally

	float screenY = VirtualToScreen(0, virtualStartY, res).y;


	const char* menuItems[] = { Localise("Pause_Menu_Continue").c_str(), 
		Localise("Pause_Menu_Restart").c_str(), Localise("Pause_Menu_Options").c_str(),
		Localise("Pause_Menu_Quit").c_str()
	};
	float startY = screenY;
	float spacing = ScaleY(20.0f, res.y);

	for (int i = 0; i < 4; i++)
	{
		uint8_t color = i == s_index ? 255 : 127;
		float fontSize = Scale(Config::Language == ELanguage::Japanese ? 20 : 19);
		//auto textSize = g_fntDFPZong->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, s.c_str());
		AddTextCentered(drawList, g_fntDFPZong, fontSize,
			res.x / 2.0f,
			startY + i * spacing,
			IM_COL32(color, color, color, 255),  // white text
			IM_COL32(0, 0, 0, 255),        // black outline
			menuItems[i]);
	}

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

// TPauseManagerExec
X86_FUNC_IMPL(__imp__sub_5AEE0);
X86_FUNC(sub_5AEE0)
{
	auto flagdisabled = X86_LOAD_U8(ctx.ecx.u32 + 0x3C);

	g_pauseModeAddr = ctx.ecx.u32 + 0x30;
	auto mode = X86_LOAD_U32(g_pauseModeAddr);

	if (mode == 2 && g_inputAllowed == false)
	{
		g_inputAllowed = true; //to do find a better way, only enable it when opening anim is complete
	}


	// handle new options in pause menu by tricking the game that we shouldn't quit
	if (!s_quitOptionsRequest && s_index == SelOptions && s_OptionsRequest && OptionsMenu::s_isVisible == false)
	{
		OptionsMenu::Open(true);
		s_isVisible = false;
		ctx.AddEsp(1);
		
		return;
	}
	else if (mode == ModeContinue || mode == ModeRestart)
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

	g_CursorModeAddr = ctx.ecx.u32 + 0x38;
	auto mode = X86_LOAD_U8(g_CursorModeAddr);
	if (s_index == SelOptions || s_index == SelQuit)
	{
		ctx.AddEsp(1);
		return;
	}
	else
	{
		__imp__sub_5AAF0(ctx, base);
		mode = X86_LOAD_U8(g_CursorModeAddr);
	}

	if (s_index != mode && s_index < SelOptions )
	{
		X86_STORE_U32(g_CursorModeAddr, s_index);
	}
}


void PausePatchesInit()
{
	g_TexPauseText00 = LOAD_ZSTD_TEXTURE(g_pauseText00);
	g_TexPauseText01 = LOAD_ZSTD_TEXTURE(g_pauseText01);
	g_fntDFPZong = ImFontAtlasSnapshot::GetFont("DFPZongYiW5-B5.ttc");
}