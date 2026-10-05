#include "installer_wizard.h"

#include <nfd.h>
#include <audio/embedded_player.h>
#include <install/xiso_extract.h>
#include <gpu/video.h>
#include <locale/locale.h>
#include <user/config.h>
#include <ui/game_window.h>
#include <os/logger.h>
#include <input/input.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <exports.h>
#include <patches/aspect_ratio_patches.h>
#include <gpu/imgui/imgui_snapshot.h>
#include <ui/imgui_utils.h>
#include <res/images/common/TitleBg.png.h>
#include <res/images/common/Options2.png.h>

#include <decompressor.h>

static std::unique_ptr<GuestTexture> g_upTexOptionsBG{};
static std::unique_ptr<GuestTexture> g_upTexOptionsBG2{};

enum class WizardPage
{
	SelectLanguage,
	Introduction,
	SelectGame,
	Installing,
	InstallSucceeded,
	InstallFailed,
};

enum class MessagePromptSource
{
	Unknown,
	Next,
	Back
};

enum ButtonColumn
{
	ButtonColumnLeft,
	ButtonColumnMiddle,
	ButtonColumnRight
};

static ImFont* g_fntSeurat;
static constexpr double SCANLINES_ANIMATION_TIME = 0.0;
static constexpr double SCANLINES_ANIMATION_DURATION = 15.0;

static constexpr double MILES_ICON_ANIMATION_TIME = SCANLINES_ANIMATION_TIME + 10.0;
static constexpr double MILES_ICON_ANIMATION_DURATION = 15.0;

static constexpr double IMAGE_ANIMATION_TIME = MILES_ICON_ANIMATION_TIME + MILES_ICON_ANIMATION_DURATION;
static constexpr double IMAGE_ANIMATION_DURATION = 15.0;

static constexpr double TITLE_ANIMATION_TIME = SCANLINES_ANIMATION_DURATION;
static constexpr double TITLE_ANIMATION_DURATION = 30.0;

static constexpr double CONTAINER_LINE_ANIMATION_TIME = SCANLINES_ANIMATION_DURATION;
static constexpr double CONTAINER_LINE_ANIMATION_DURATION = 23.0;

static constexpr double CONTAINER_OUTER_TIME = SCANLINES_ANIMATION_DURATION + CONTAINER_LINE_ANIMATION_DURATION;
static constexpr double CONTAINER_OUTER_DURATION = 23.0;

static constexpr double CONTAINER_INNER_TIME = SCANLINES_ANIMATION_DURATION + CONTAINER_LINE_ANIMATION_DURATION + 8.0;
static constexpr double CONTAINER_INNER_DURATION = 10.f;

static constexpr double ALL_ANIMATIONS_FULL_DURATION = CONTAINER_INNER_TIME + CONTAINER_INNER_DURATION;
static constexpr double QUITTING_EXTRA_DURATION = 60.0;

static float fontSize = 24.0f;
static float smallFontSize = 18.0f;
static float fileFontSize = 16.0f;
static float descFontSize = 18.0f;

// Shadow's orig res is 640x480, use this as a base for window scaling
static const float baseWidth = 640.0f;
static const float baseHeight = 480.0f;

constexpr float CONTAINER_X = 153.0f;
constexpr float CONTAINER_Y = 100.f;
constexpr float CONTAINER_WIDTH = 526.5f;
constexpr float CONTAINER_HEIGHT = 246.0f;
constexpr float SIDE_CONTAINER_WIDTH = CONTAINER_WIDTH / 2.0f;
constexpr float CONTAINER_BUTTON_WIDTH = 75.0f;
constexpr float CONTAINER_BUTTON_GAP = 10.0f;
constexpr float BUTTON_HEIGHT = 22.0f;
constexpr float BUTTON_TEXT_GAP = 28.0f;

static double g_appearTime = 0.0;
static double g_disappearTime = DBL_MAX;
static int g_selectedBtn = -1;

static WizardPage g_currentPage = WizardPage::SelectLanguage;
static std::string g_currentMessagePrompt = "";
static MessagePromptSource g_currentMessagePromptSource = MessagePromptSource::Unknown;
static bool g_currentMessagePromptConfirmation = false;
static std::filesystem::path g_installPath;
static std::string g_isoPath;
static std::string g_statusMessage;

static bool g_currentPickerVisible = false;
static std::unique_ptr<std::thread> g_installerThread;
static std::atomic<bool> g_installerFinished{ false };
static std::atomic<bool> g_installerFailed{ false };
static std::string g_installerErrorMessage;
static float g_installerProgress = 0.0f;
static std::atomic<float> g_installerProgressTarget{ 0.0f };
static std::string g_installerCurrentFileName;
static std::atomic<XisoExtract::Phase> g_installerPhase{ XisoExtract::Phase::Hashing };
static std::atomic<bool> g_installerCompiling{ false };

static constexpr float INSTALL_EXTRACT_RATIO_END = 0.8f;

static bool g_currentCursorAccepted = false;
static bool g_currentCursorBack = false;
static int g_currentCursorIndex = -1;
static int g_currentCursorDefault = 0;
static bool g_isQuitting = false;
static bool g_browseClicked = false;

static std::vector<std::pair<ImVec2, ImVec2>> g_currentCursorRects;

static bool playedNextSound = false;
static bool playedPrevSound = false;
static bool playedLangSound[6] = {false};
static bool playedBrowseSound = false;

static std::string& GetWizardText(WizardPage page)
{
	switch (page)
	{
	case WizardPage::SelectLanguage: return Localise("Installer_Page_SelectLanguage");
	case WizardPage::Introduction: return Localise("Installer_Page_Introduction");
		// case WizardPage::CheckSpace: return Localise("Installer_Page_CheckSpace");
	case WizardPage::SelectGame: return Localise("Installer_Page_SelectGame");
	case WizardPage::Installing: return Localise("Installer_Page_Installing");
	case WizardPage::InstallSucceeded: return Localise("Installer_Page_InstallSucceeded");
	}

	return g_localeMissing;
}

// These are ordered from bottom to top in a 3x2 grid.
const char* LANGUAGE_TEXT[] =
{
	"Français", // French
	"Deutsch", // German
	"English", // English
	"Español", // Spanish
	"Italiano", // Italian
	"日本語", // Japanese
};

const ELanguage LANGUAGE_ENUM[] =
{
	ELanguage::French,
	ELanguage::German,
	ELanguage::English,
	ELanguage::Spanish,
	ELanguage::Italian,
	ELanguage::Japanese,
};

static double ComputeMotionInstaller(double timeAppear, double timeDisappear, double offset, double total)
{
	return ComputeMotion(timeAppear, offset, total, false) * (1.0 - ComputeMotion(timeDisappear, ALL_ANIMATIONS_FULL_DURATION - offset - total, total, false));
}

static double ComputeMotionInstallerLoop(double timeAppear, double speed, double offset)
{
	return std::clamp(fmodf((ImGui::GetTime() - timeAppear) * speed, 1.0f + offset) - offset, 0.0, 1.0) / 1.0;
}

static double ComputeHermiteMotionInstallerLoop(double timeAppear, double speed, double offset)
{
	return (cosf(M_PI * ComputeMotionInstallerLoop(timeAppear, speed, offset) + M_PI) + 1) / 2;
}
static bool PushCursorRect(ImVec2 min, ImVec2 max, bool& cursorPressed, bool makeDefault = false)
{
	int currentIndex = int(g_currentCursorRects.size());
	g_currentCursorRects.emplace_back(min, max);

	if (makeDefault)
	{
		g_currentCursorDefault = currentIndex;
	}

	if (g_currentCursorIndex == currentIndex)
	{
		if (g_currentCursorAccepted)
		{
			Game_PlaySound("decide");
			cursorPressed = true;
			g_currentCursorAccepted = false;
		}

		return true;
	}
	else
	{
		return false;
	}
}

static void ResetCursorRects()
{
	g_currentCursorDefault = 0;
	g_currentCursorRects.clear();
}


static void DrawButtonContainer(ImVec2 min, ImVec2 max, int baser, int baseg, float alpha)
{
	auto& res = ImGui::GetIO().DisplaySize;
	auto drawList = ImGui::GetBackgroundDrawList();
	SetShaderModifier(IMGUI_SHADER_MODIFIER_SCANLINE_BUTTON);
	drawList->AddRectFilledMultiColor(
		min, max,
		IM_COL32(180, 0, 0, 223 * alpha),
		IM_COL32(140, 0, 0, 178 * alpha),
		IM_COL32(160, 0, 0, 223 * alpha),
		IM_COL32(120, 0, 0, 178 * alpha) 
	);

	drawList->AddRectFilledMultiColor(
		min, max,
		IM_COL32(60, 0, 0, 180 * alpha),  
		IM_COL32(40, 0, 0, 100 * alpha),  
		IM_COL32(80, 0, 0, 200 * alpha),  
		IM_COL32(30, 0, 0, 80 * alpha)    
	);


	drawList->AddRectFilledMultiColor(
		min, max,
		IM_COL32(220, 0, 0, 60 * alpha),    
		IM_COL32(200, 0, 0, 30 * alpha),    
		IM_COL32(255, 0, 0, 80 * alpha),    
		IM_COL32(180, 0, 0, 20 * alpha)     
	);
	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);
}


// static void ComputeButtonColumnCoordinates(ButtonColumn buttonColumn, ImVec2 panelMin, ImVec2 panelMax,
// 	float& minX, float& maxX)
// {
// 	float panelWidth = panelMax.x - panelMin.x;
//
// 	// Scale button width proportionally to panel, with a cap so it
// 	// doesn't get absurdly wide on large panels.
// 	float buttonWidth = std::min(panelWidth * 0.38f, Scale(CONTAINER_BUTTON_WIDTH));
// 	float gap = panelWidth * 0.04f; // ~4% margin from edges
//
// 	switch (buttonColumn)
// 	{
// 	case ButtonColumnLeft:
// 		minX = panelMin.x + gap;
// 		maxX = panelMin.x + gap + buttonWidth;
// 		break;
// 	case ButtonColumnMiddle:
// 		minX = panelMin.x + panelWidth * 0.5f - buttonWidth * 0.5f;
// 		maxX = panelMin.x + panelWidth * 0.5f + buttonWidth * 0.5f;
// 		break;
// 	case ButtonColumnRight:
// 		minX = panelMax.x - gap - buttonWidth;
// 		maxX = panelMax.x - gap;
// 		break;
// 	}
// }

static ImVec2 ComputeTextSize(ImFont* font, const char* text, float size, float& squashRatio, float maxTextWidth = FLT_MAX)
{
	ImVec2 textSize = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text);
	if (textSize.x > maxTextWidth)
	{
		squashRatio = maxTextWidth / textSize.x;
	}
	else
	{
		squashRatio = 1.0f;
	}
	return textSize;
}


static void DrawButton(ImVec2 min, ImVec2 max, const char* buttonText, bool sourceButton, int i, bool buttonEnabled, bool& buttonPressed, float maxTextWidth = FLT_MAX, bool makeDefault = false)
{
	buttonPressed = false;

	// default to english
	if (i == 2 && g_selectedBtn == -1)
	{
		buttonPressed = true;
		buttonEnabled = true;
	}

	auto& res = ImGui::GetIO().DisplaySize;
	auto drawList = ImGui::GetBackgroundDrawList();
	float alpha = 255;
	if (!buttonEnabled)
	{
		alpha *= 0.5f;
	}
	auto& io = ImGui::GetIO();
	bool btnHovered = false;

	if (io.MousePos.x >= min.x && io.MousePos.x <= max.x &&
		io.MousePos.y >= min.y && io.MousePos.y <= max.y)
	{
		btnHovered = true;
		if(!playedLangSound[i] && !buttonEnabled){
			Game_PlaySound("cursor");
			playedLangSound[i] = true;
		}
	}
	else { playedLangSound[i] = false; }



	if (btnHovered && io.MouseClicked[0] && !buttonEnabled)
	{
		Game_PlaySound("decide");
		buttonPressed = true;
		g_selectedBtn = i;
	}

	int baser = 0;
	int baseg = 0;
	//DrawButtonContainer(min, max, baser, baseg, alpha);

	ImFont* font = g_fntSeurat;
	float size = Scale(18.0f);
	float squashRatio;
	ImVec2 textSize = ComputeTextSize(font, buttonText, size, squashRatio, Scale(maxTextWidth));
	min.x += ((max.x - min.x) - textSize.x) / 2.0f;
	min.y += ((max.y - min.y) - textSize.y) / 2.0f;

	if (!sourceButton)
	{
		// Fixes slight misalignment caused by this particular font.
		min.y -= Scale(1.0f);
	}

	SetOrigin({ min.x + textSize.x / 2.0f, min.y });
	SetScale({ squashRatio, 1.0f });

	ImU32 textCol = IM_COL32(255, 255, 255, alpha);
	if (buttonPressed || btnHovered || buttonEnabled)
	{
		textCol = IM_COL32(255, 255, 0, alpha);
		drawList->AddText(font,
						  size,
						  min,
						  textCol,
						  buttonText);
	}
	else
	{
		DrawTextWithShadow(
			font,
			size,
			min,
			textCol,
			buttonText,
			1.5,
			1,
			IM_COL32(0, 0, 0, alpha)
		);
	}

	ResetGradient();
	SetScale({ 1.0f, 1.0f });
	SetOrigin({ 0.0f, 0.0f });
}


static void InstallerThread()
{
	auto progressCallback = [](const XisoExtract::Progress& progress)
		{
			g_installerCurrentFileName = progress.currentFileName;
			g_installerPhase.store(progress.phase);
			g_installerProgressTarget.store(progress.ratio * INSTALL_EXTRACT_RATIO_END);
		};

	bool success = XisoExtract::Extract(g_isoPath, g_installPath, progressCallback);
	if (success)
	{
		g_installerCompiling.store(true);
		g_installerCurrentFileName.clear();

		Video::PrecompileShadersAndPipelinesForInstall([](float progress)
			{
				g_installerProgressTarget.store(INSTALL_EXTRACT_RATIO_END + (1.0f - INSTALL_EXTRACT_RATIO_END) * progress);
			});

		g_installerCompiling.store(false);
		g_installerProgressTarget.store(1.0f);
	}
	else
	{
		g_installerProgressTarget.store(1.0f);
		g_installerFailed.store(true);
		g_installerErrorMessage = Localise("Installer_Message_InvalidFileSelected");
	}
	g_installerFinished.store(true);
}

static void DrawLanguagePicker(ImVec2 panelMin, ImVec2 panelMax)
{
	bool buttonPressed = false;
	float panelWidth = panelMax.x - panelMin.x;
	float panelHeight = panelMax.y - panelMin.y;

	float buttonWidth = std::min(panelWidth * 0.38f, Scale(CONTAINER_BUTTON_WIDTH));
	float buttonH = Scale(BUTTON_HEIGHT);
	float gap = panelWidth * 0.3f;
	float bottomMargin = panelHeight * 0.375f;
	float rowStride = buttonH + Scale(CONTAINER_BUTTON_GAP);

	float leftMinX = panelMin.x + gap;
	float leftMaxX = leftMinX + buttonWidth;
	float rightMinX = panelMax.x - gap - buttonWidth;
	float rightMaxX = panelMax.x - gap;

	for (int i = 0; i < 6; i++)
	{
		float minX = (i < 3) ? leftMinX : rightMinX;
		float maxX = (i < 3) ? leftMaxX : rightMaxX;
		float minusY = rowStride * float(i % 3);
		float btnMaxY = panelMax.y - bottomMargin - minusY;
		float btnMinY = btnMaxY - buttonH;

		ImVec2 min = { minX, btnMinY };
		ImVec2 max = { maxX, btnMaxY };

		DrawButton(min, max, LANGUAGE_TEXT[i], false, i,
			i == g_selectedBtn, buttonPressed, FLT_MAX,
			LANGUAGE_ENUM[i] == ELanguage::English);

		if (buttonPressed)
		{
			Config::Language = LANGUAGE_ENUM[i];
		}
	}
}

static void DrawNextButton(ImVec2& panelMin, ImVec2& panelMax, float scale)
{
	auto& io = ImGui::GetIO();
	auto drawList = ImGui::GetBackgroundDrawList();
	auto& displaySize = io.DisplaySize;
	auto* font = g_fntSeurat;

	auto nextText = Localise("Installer_Button_Next").c_str();
	auto nextSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, nextText);

	float nextBtnWidth = 45.0f * scale;
	float nextBtnHeight = 20.0f * scale;
	float nextBtnX = panelMin.x + ((panelMax.x - panelMin.x) - nextBtnWidth) * 0.5f;
	float nextBtnY = panelMax.y - nextBtnHeight - (70.0f * scale);

	bool nextHovered = false;

	if (io.MousePos.x >= nextBtnX && io.MousePos.x <= nextBtnX + nextBtnWidth &&
		io.MousePos.y >= nextBtnY && io.MousePos.y <= nextBtnY + nextBtnHeight)
	{
		nextHovered = true;
		if(!playedNextSound)
		{
			Game_PlaySound("cursor");
			playedNextSound = true;
		}
	}
	else { playedNextSound = false; }

	if (nextHovered)
	{
		drawList->AddText(
			font,
			smallFontSize,
			ImVec2(nextBtnX + (nextBtnWidth - nextSize.x) * 0.5f, nextBtnY + (nextBtnHeight - nextSize.y) * 0.5f),
			IM_COL32(255, 255, 0, 255),
			nextText
		);
	}
	else
	{
		DrawTextWithShadow(
			font,
			smallFontSize,
			ImVec2(nextBtnX + (nextBtnWidth - nextSize.x) * 0.5f, nextBtnY + (nextBtnHeight - nextSize.y) * 0.5f),
			IM_COL32(255, 255, 255, 127),
			nextText,
			1.5,
			1,
			IM_COL32(0, 0, 0, 127)
		);
	}

	if (nextHovered && io.MouseClicked[0])
	{
		Game_PlaySound("decide");
		g_currentPage = (WizardPage)((int)g_currentPage + 1);
	}
}

static void DrawQuitButton(ImVec2& panelMin, ImVec2& panelMax, float scale)
{
	auto& io = ImGui::GetIO();
	auto drawList = ImGui::GetBackgroundDrawList();
	auto& displaySize = io.DisplaySize;
	auto* font = g_fntSeurat;

	auto quitText = Localise("Installer_Button_Quit").c_str();
	auto quitSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, quitText);

	float quitBtnWidth = 45.0f * scale;
	float quitBtnHeight = 20.0f * scale;
	float quitBtnX = panelMin.x + ((panelMax.x - panelMin.x) - quitBtnWidth) * 0.5f;
	float quitBtnY = panelMax.y - quitBtnHeight - (35.0f * scale);

	bool quitHovered = false;
	if (io.MousePos.x >= quitBtnX && io.MousePos.x <= quitBtnX + quitBtnWidth &&
		io.MousePos.y >= quitBtnY && io.MousePos.y <= quitBtnY + quitBtnHeight)
	{
		quitHovered = true;
		if(!playedPrevSound){
			Game_PlaySound("cursor");
			playedPrevSound = true;
		}
	}
	else { playedPrevSound = false; }

	if (quitHovered)
	{
		drawList->AddText(
			font,
			smallFontSize,
			ImVec2(quitBtnX + (quitBtnWidth - quitSize.x) * 0.5f, quitBtnY + (quitBtnHeight - quitSize.y) * 0.5f),
			IM_COL32(255, 255, 0, 255),
			quitText
		);
	}
	else
	{
		DrawTextWithShadow(
			font,
			smallFontSize,
			ImVec2(quitBtnX + (quitBtnWidth - quitSize.x) * 0.5f, quitBtnY + (quitBtnHeight - quitSize.y) * 0.5f),
			IM_COL32(255, 255, 255, 127),
			quitText,
			1.5,
			1,
			IM_COL32(0, 0, 0, 127)
		);
	}

	if (quitHovered && io.MouseClicked[0])
	{
		g_isQuitting = true;
		InstallerWizard::s_isVisible = false;
	}
}


static void DrawPrevButton(ImVec2& panelMin, ImVec2& panelMax, float scale)
{
	auto& io = ImGui::GetIO();
	auto drawList = ImGui::GetBackgroundDrawList();
	auto& displaySize = io.DisplaySize;
	auto* font = g_fntSeurat;

	auto prevText = Localise("Installer_Button_Back").c_str();
	auto prevSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, prevText);

	float prevBtnWidth = 45.0f * scale;
	float prevBtnHeight = 20.0f * scale;
	float prevBtnX = panelMin.x + ((panelMax.x - panelMin.x) - prevBtnWidth) * 0.5f;
	float prevBtnY = panelMax.y - prevBtnHeight - (35.0f * scale);

	bool prevHovered = false;
	if (io.MousePos.x >= prevBtnX && io.MousePos.x <= prevBtnX + prevBtnWidth &&
		io.MousePos.y >= prevBtnY && io.MousePos.y <= prevBtnY + prevBtnHeight)
	{
		prevHovered = true;
		if(!playedPrevSound){
			Game_PlaySound("cursor");
			playedPrevSound = true;
		}
	}
	else { playedPrevSound = false; }

	if (prevHovered)
	{
		drawList->AddText(
			font,
			smallFontSize,
			ImVec2(prevBtnX + (prevBtnWidth - prevSize.x) * 0.5f, prevBtnY + (prevBtnHeight - prevSize.y) * 0.5f),
			IM_COL32(255, 255, 0, 255),
			prevText
		);
	}
	else
	{
		DrawTextWithShadow(
			font,
			smallFontSize,
			ImVec2(prevBtnX + (prevBtnWidth - prevSize.x) * 0.5f, prevBtnY + (prevBtnHeight - prevSize.y) * 0.5f),
			IM_COL32(255, 255, 255, 127),
			prevText,
			1.5,
			1,
			IM_COL32(0, 0, 0, 127)
		);
	}

	if (prevHovered && io.MouseClicked[0])
	{
		Game_PlaySound("decide");
		g_currentPage = (WizardPage)((int)g_currentPage - 1);
	}

}

static void DrawDescription(std::string s, float alpha = 1.0f)
{
	static std::string prevText;
	static double scrollStartTime = 0.0;
	constexpr double scrollDelay = 0.5; // seconds to wait before scrolling
	double elapsed = ImGui::GetTime() - scrollStartTime;

	std::replace(s.begin(), s.end(), '\n', ' ');

	if (prevText != s)
	{
		prevText = s;
		scrollStartTime = ImGui::GetTime() + scrollDelay;
	}

	auto drawList = ImGui::GetBackgroundDrawList();
	auto& res = ImGui::GetIO().DisplaySize;

	// First we calculate the position of the black bar of the menu BG
	float barTexX = 74.0f;
	float barTexY = 38.f;
	float barTexH = 48.f;
	float texHeight = 512.0f;

	// Then we map to screen space proportionally
	float barScreenX = (barTexX / texHeight) * res.x;
	float barScreenY = (barTexY / texHeight) * res.y;
	float barScreenH = (barTexH / texHeight) * res.y;
	ImVec2 barMin = { barScreenX + Scale(30), barScreenY };
	ImVec2 barMax = { res.x, barScreenY + barScreenH };
	drawList->PushClipRect(barMin, barMax);
	float fontSize = Scale(18.0f) - 2.0f;
	auto textSize = g_fntSeurat->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, s.c_str());
	float barWidth = res.x - barScreenX;

	ImVec2 textPos = {
		barScreenX + barWidth * 0.5f - textSize.x * 0.5f + Scale(15.f),  // centered within bar bounds
		barScreenY + barScreenH * 0.6f - textSize.y * 0.5f
	};

	float offsetScroll = 0.0f;
	float totalWidth = 0.0f;
	bool isScrolling = textSize.x > barWidth - Scale(55.f);
	if (isScrolling)
	{
		float gap = Scale(80.0f);
		totalWidth = textSize.x + gap;
		float scrollSpeed = Scale(70.0f);

		// Anchor to left edge of the bar so scroll always starts at beginning of text
		textPos.x = barMin.x + Scale(15.f);
		offsetScroll = elapsed > 0.0 ? fmod(elapsed * scrollSpeed, totalWidth) : 0.0f;
	}
	textPos.x -= offsetScroll;
	ImU32 descColor = IM_COL32(255, 255, 255, (int)(255 * alpha));
	drawList->AddText(g_fntSeurat, fontSize, textPos, descColor, s.c_str());

	if (isScrolling)
	{
		// Draw the looping copy trailing behind
		ImVec2 loopPos = { textPos.x + totalWidth, textPos.y };
		drawList->AddText(g_fntSeurat, fontSize, loopPos, descColor, s.c_str());
	}
	drawList->PopClipRect();
}

static void DrawBackground(ImGuiViewport* viewport, ImVec2 winPos, ImVec2 winSize, float scale, float offsetX, float offsetY)
{
	auto drawList = ImGui::GetBackgroundDrawList();

	// Colours
	ImU32 bevCol = IM_COL32(14, 14, 110, 255); // Bevel Edge
	ImU32 shaCol = IM_COL32(0, 0, 0, 255); // 'Shadow' (top + left)
	ImU32 ligCol = IM_COL32(41, 41, 182, 255); // 'Light' (bottom + right)
	ImU32 winCol = IM_COL32(0, 0, 69, 255);  // Main Window (nice) (yes that is the real colour Shadow uses)

	// Bevel at edges
	DrawScaledRect(drawList, winPos, scale, offsetX, offsetY, 21.0f, 54.0f, 598.0f, 362.0f, bevCol);

	// Black shadow (top + left)
	DrawScaledRect(drawList, winPos, scale, offsetX, offsetY, 25.0f, 58.0f, 589.0f, 353.0f, shaCol);

	// Blue highlight (bottom + right)
	drawList->AddTriangleFilled(
		ScaleTriangleVert(winPos, scale, offsetX, offsetY, 25.0f, 411.0f),
		ScaleTriangleVert(winPos, scale, offsetX, offsetY, 614.0f, 58.0f),
		ScaleTriangleVert(winPos, scale, offsetX, offsetY, 614.0f, 411.0f),
		ligCol
	);

	// Main window background
	DrawScaledRect(drawList, winPos, scale, offsetX, offsetY, 26.0f, 59.0f, 587.0f, 351.0f, winCol);
}

static void DrawWizardParagraph(const char* text, float maxWidth, float lineMargin, const ImVec2& pos)
{
	auto draw = [](const char* str, float size, ImVec2 p) { DrawTextWithShadow(g_fntSeurat, size, p, IM_COL32(255, 255, 255, 255), str, 1.5, 1, IM_COL32(0, 0, 0, 127)); };

	DrawRubyAnnotatedText(g_fntSeurat, descFontSize, maxWidth, pos, lineMargin, text,
		[&](const char* str, ImVec2 p) { draw(str, descFontSize, p); },
		draw);
}

void InstallerWizard::Draw()
{
	if (!s_isVisible)
		return;

	auto& io = ImGui::GetIO();
	auto drawList = ImGui::GetBackgroundDrawList();
	auto& displaySize = io.DisplaySize;
	ImGuiViewport* viewport = ImGui::GetMainViewport();

	// Scale using the actual whole viewport instead of the many other ways googling will give you
	ImVec2 winPos = viewport->Pos;
	ImVec2 winSize = viewport->Size;

	// Scale values based on window size
	float scaleX = winSize.x > 1.0f ? (winSize.x / baseWidth) : 1.0f;
	float scaleY = winSize.y > 1.0f ? (winSize.y / baseHeight) : 1.0f;
	float scale = fminf(scaleX, scaleY);

	// Center to screen based on Shadow res
	float canvasW = baseWidth * scale;
	float canvasH = baseHeight * scale;
	float offsetX = (winSize.x - canvasW) * 0.5f;
	float offsetY = (winSize.y - canvasH) * 0.5f;

	drawList->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(0, 0, 0, 255));

	auto* font = g_fntSeurat;

	float verticalOffset = 40.0f * scale;

	ImVec2 panelMin = ImVec2(
						winPos.x + offsetX + (26.0f * scale),
						winPos.y + offsetY + (59.0f * scale)
					  );
	ImVec2 panelMax = ImVec2(
						panelMin.x + (587.0f * scale),
						panelMin.y + (351.0f * scale)
					  );

	float panelWidth = panelMax.x - panelMin.x;
	float centerX = (panelMin.x + panelMax.x) * 0.5f;
	float centerY = (panelMin.y + panelMax.y) * 0.5f;

	float baseFontSize = 18.0f * scale;
	fontSize = baseFontSize;
	descFontSize = baseFontSize + std::floor(baseFontSize * 0.2f);
	smallFontSize = std::floor(baseFontSize * 0.9f);
	fileFontSize = std::floor(baseFontSize * 0.7f);

	// Background, description banner, and body text fade in from black on
	// the language-selection screen, matching the fade used by the
	// language-picker button containers.
	//float fadeAlpha = 1.0f;
	//if (g_currentPage == WizardPage::SelectLanguage)
	//{
	//	fadeAlpha = ComputeMotionInstaller(g_appearTime, g_disappearTime, CONTAINER_INNER_TIME, CONTAINER_INNER_DURATION);
	//}

	DrawBackground(viewport, winPos, winSize, scale, offsetX, offsetY);

	auto titleText = "Shadow the Hedgehog Recompiled - Setup";
	auto titleSize = font->CalcTextSizeA(fontSize + 2.f, FLT_MAX, 0, titleText);
	float titleX = centerX - titleSize.x * 0.5f;

	std::string wizardText = GetWizardText(g_currentPage);
	char descriptionText[512];
	strncpy(descriptionText, wizardText.c_str(), sizeof(descriptionText) - 1);
	auto originMargin = 65.0f * scale;
	auto originMin = ImVec2(panelMin.x + originMargin, panelMin.y + originMargin);
	auto originMax = ImVec2(panelMax.x - originMargin, panelMax.y - originMargin);
	auto textMargin = 5.0f;

	auto textMaxWidth = originMax.x - (descFontSize / 2.0f) - originMin.x;
	auto textSize = MeasureCentredParagraph(g_fntSeurat, descFontSize, textMaxWidth, textMargin, wizardText.c_str());
	ImVec2 textPosition = ImVec2(panelMin.x + (panelWidth - textSize.x) * 0.5f, originMin.y);

	if (g_currentPage != WizardPage::Installing && g_currentPage != WizardPage::InstallFailed)
	{
		DrawWizardParagraph(wizardText.c_str(), textMaxWidth, textMargin, textPosition);
	}

	switch (g_currentPage)
	{
	case WizardPage::SelectLanguage:
	{
		DrawLanguagePicker(panelMin, panelMax);

		DrawNextButton(panelMin, panelMax, scale);

		DrawQuitButton(panelMin, panelMax, scale);
	}
	break;
	case WizardPage::Introduction:
	{
		DrawPrevButton(panelMin, panelMax, scale);

		DrawNextButton(panelMin, panelMax, scale);
	}
	break;
	case WizardPage::SelectGame:
	{
		ImVec2 inputMin = ImVec2(
							panelMin.x + (74.0f * scale),
							panelMin.y + (141.0f * scale)
						  );
		ImVec2 inputMax = ImVec2(
							inputMin.x + (350.0f * scale),
							inputMin.y + (32.0f * scale)
						  );

		DrawScaledRect(drawList, winPos, scale, offsetX, offsetY, 100.0f, 200.0f, 350.0f, 32.0f, IM_COL32(0, 0, 40, 255));

		float pathClipWidth = (inputMax.x - (8.0f * scale)) - (inputMin.x + (8.0f * scale));

		const std::string displayPathStorage = g_isoPath.empty() ? Localise("Installer_Page_NoFileSelected") : std::string(g_isoPath);
		const char* displayPath = displayPathStorage.c_str();

		auto displayTextSize = font->CalcTextSizeA(fileFontSize, FLT_MAX, 0.0f, displayPath);
		bool pathOverflows = displayTextSize.x > pathClipWidth;
		std::string overflowRender;
		const char* pathToShow = displayPath;

		if (pathOverflows)
		{
			// Find the longest tail of displayPath that, when prefixed with
			// "...", still fits within pathClipWidth.
			const std::string ellipsis = "...";
			size_t dlen = strlen(displayPath);
			size_t tailLen = dlen;
			while (tailLen > 1)
			{
				std::string candidate = ellipsis + std::string(displayPath + (dlen - tailLen), tailLen);
				auto candSize = font->CalcTextSizeA(fileFontSize, FLT_MAX, 0.0f, candidate.c_str());
				if (candSize.x <= pathClipWidth)
				{
					overflowRender = std::move(candidate);
					break;
				}
				tailLen--;
			}
			if (overflowRender.empty())
				overflowRender = ellipsis;
			pathToShow = overflowRender.c_str();
		}

		drawList->AddText(font, fileFontSize, ImVec2(inputMin.x + (10.0f * scale), inputMin.y + ((inputMax.y - inputMin.y) - displayTextSize.y) * 0.5f), IM_COL32(255, 255, 255, 127), pathToShow, nullptr, pathClipWidth);

		const std::string browseTextStorage = Localise("Installer_Header_Browse");
		const char* browseText = browseTextStorage.c_str();
		auto browseSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, browseText);

		float btnWidth = 80.0f * scale;
		float btnHeight = 20.0f * scale;
		float btnX = inputMax.x + (15.0f * scale);
		float btnY = inputMin.y + ((inputMax.y - inputMin.y) - btnHeight) * 0.5f;

		bool btnHovered = false;
		if (io.MousePos.x >= btnX && io.MousePos.x <= btnX + btnWidth &&
			io.MousePos.y >= btnY && io.MousePos.y <= btnY + btnHeight)
		{
			btnHovered = true;
			if(!playedBrowseSound){
				Game_PlaySound("cursor");
				playedBrowseSound = true;
			}
		}
		else { playedBrowseSound = false; }

		if (btnHovered)
		{
			drawList->AddText(
				font,
				smallFontSize,
				ImVec2(btnX + (btnWidth - browseSize.x) * 0.5f, btnY + (btnHeight - browseSize.y) * 0.5f),
				IM_COL32(255, 255, 0, 255),
				browseText
			);
		}
		else
		{
			DrawTextWithShadow(
				font,
				smallFontSize,
				ImVec2(btnX + (btnWidth - browseSize.x) * 0.5f, btnY + (btnHeight - browseSize.y) * 0.5f),
				IM_COL32(255, 255, 255, 127),
				browseText,
				1.5,
				1,
				IM_COL32(0, 0, 0, 127)
			);
		}

		if (btnHovered && io.MouseClicked[0])
		{
			Game_PlaySound("decide");
			g_browseClicked = true;
		}

		if (!g_statusMessage.empty())
		{
			drawList->AddText(font, smallFontSize, ImVec2(panelMin.x, panelMin.y), IM_COL32(255, 100, 100, 255), g_statusMessage.c_str());
		}

		if (!g_isoPath.empty())
		{
			float nextBtnWidth = 45.0f * scale;
			float nextBtnHeight = 20.0f * scale;
			float nextBtnX = panelMin.x + ((panelMax.x - panelMin.x) - nextBtnWidth) * 0.5f;
			float nextBtnY = panelMax.y - nextBtnHeight - (70.0f * scale);

			auto nextText = Localise("Common_Install").c_str();
			auto nextSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, nextText);

			bool nextHovered = false;

			if (io.MousePos.x >= nextBtnX && io.MousePos.x <= nextBtnX + nextBtnWidth &&
				io.MousePos.y >= nextBtnY && io.MousePos.y <= nextBtnY + nextBtnHeight)
			{
				nextHovered = true;
				if(!playedNextSound)
				{
					Game_PlaySound("cursor");
					playedNextSound = true;
				}
			}
			else { playedNextSound = false; }

			if (nextHovered)
			{
				drawList->AddText(
					font,
					smallFontSize,
					ImVec2(nextBtnX + (nextBtnWidth - nextSize.x) * 0.5f, nextBtnY + (nextBtnHeight - nextSize.y) * 0.5f),
								  IM_COL32(255, 255, 0, 255),
								  nextText
				);
			}
			else
			{
				DrawTextWithShadow(
					font,
					smallFontSize,
					ImVec2(nextBtnX + (nextBtnWidth - nextSize.x) * 0.5f, nextBtnY + (nextBtnHeight - nextSize.y) * 0.5f),
								   IM_COL32(255, 255, 255, 127),
								   nextText,
					   1.5,
					   1,
					   IM_COL32(0, 0, 0, 127)
				);
			}

			if (nextHovered && io.MouseClicked[0])
			{
				Game_PlaySound("decide");
				g_currentPage = WizardPage::Installing;
				g_installerFinished.store(false);
				g_installerFailed.store(false);
				g_installerCompiling.store(false);
				g_installerProgress = 0.0f;
				g_installerProgressTarget.store(0.0f);
				g_installerThread = std::make_unique<std::thread>(InstallerThread);
			}
		}

		DrawPrevButton(panelMin, panelMax, scale);
	}
	break;
	case WizardPage::Installing:
	{
		auto phase = g_installerPhase.load();
		const char* headerText;
		if (g_installerCompiling.load())
		{
			headerText = Localise("Installer_Page_Compiling").c_str();
		}
		else if (phase == XisoExtract::Phase::Hashing)
		{
			headerText = Localise("Installer_Page_Verifying").c_str();
		}
		else
		{
			headerText = Localise("Installer_Page_Installing").c_str();
		}

		auto installTextSize = MeasureCentredParagraph(g_fntSeurat, descFontSize, textMaxWidth, textMargin, headerText);
		ImVec2 installTextPosition = ImVec2(panelMin.x + (panelWidth - installTextSize.x) * 0.5f, originMin.y);

		DrawWizardParagraph(headerText, textMaxWidth, textMargin, installTextPosition);

		float barPadding = 20.0f * scale;
		float barX = panelMin.x + barPadding;
		float barWidth = panelWidth - barPadding * 2.0f;
		float barHeight = 24.0f * scale;

		if (phase != XisoExtract::Phase::Hashing && !g_installerCompiling.load())
		{
			if (!g_installerCurrentFileName.empty())
			{
				char fileText[512];
				auto fileTxt = Localise("Common_File") + ": %s";
				snprintf(fileText, sizeof(fileText), fileTxt.c_str(), g_installerCurrentFileName.c_str());
				ImVec2 fileTextPosition = ImVec2(barX, panelMin.y + (10.0f * scale));

				DrawTextParagraph(font, fileFontSize, barWidth, fileTextPosition, textMargin, fileText,
								  [&](const char* str, ImVec2 pos) { DrawTextWithShadow(g_fntSeurat, fileFontSize, pos, IM_COL32(255, 255, 255, 127), str, 1.5, 1, IM_COL32(0, 0, 0, 127)); });
			}
		}

		ImVec2 barMin(barX, centerY - barHeight * 0.5f);
		ImVec2 barMax(barX + barWidth, centerY + barHeight * 0.5f);

		g_installerProgress = g_installerProgressTarget.load();

		drawList->AddRectFilled(barMin, barMax, IM_COL32(0, 0, 40, 255));

		float fillWidth = barWidth * std::min(g_installerProgress, 1.0f);
		if (fillWidth > 0)
		{
			ImVec2 fillMax(barX + fillWidth, centerY + barHeight * 0.5f);
			drawList->AddRectFilled(barMin, fillMax, IM_COL32(255, 255, 0, 255));
		}

		char percentText[32];
		snprintf(percentText, sizeof(percentText), "%.1f%%", g_installerProgress * 100.0f);
		auto percentSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, percentText);

		DrawTextWithShadow(font, smallFontSize, ImVec2(barX + (barWidth - percentSize.x) * 0.5f, (centerY - percentSize.y * 0.5f) + barHeight), IM_COL32(255, 255, 255, 255), percentText, 1.5, 1, IM_COL32(0, 0, 0, 127));

		if (g_installerFinished.load())
		{
			if (g_installerFailed.load())
			{
				g_currentPage = WizardPage::InstallFailed;
			}
			else
			{
				g_currentPage = WizardPage::InstallSucceeded;
			}
		}

		break;
	}

	case WizardPage::InstallSucceeded:
	{
		auto foxDev = "FoxWorkshop";
		bool playHovered = false;

		auto foxSize = font->CalcTextSizeA(descFontSize, FLT_MAX, 0, foxDev);
		auto foxPositionY = textSize.y + textPosition.y + (25.0f * scale);
		DrawTextWithShadow(font, descFontSize, ImVec2(centerX - foxSize.x * 0.5f, foxPositionY), IM_COL32(255, 255, 255, 255), foxDev, 1.5, 1, IM_COL32(0, 0, 0, 127));

		DrawQuitButton(panelMin, panelMax, scale);

		float playBtnWidth = 75.0f * scale;
		float playBtnHeight = 20.0f * scale;
		float playBtnX = panelMin.x + ((panelMax.x - panelMin.x) - playBtnWidth) * 0.5f;
		float playBtnY = panelMax.y - playBtnHeight - (70.0f * scale);

		auto playText = Localise("Installer_Button_PlayGame").c_str();
		auto playSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, playText);

		if (io.MousePos.x >= playBtnX && io.MousePos.x <= playBtnX + playBtnWidth &&
			io.MousePos.y >= playBtnY && io.MousePos.y <= playBtnY + playBtnHeight)
		{
			playHovered = true;
			if(!playedNextSound)
			{
				Game_PlaySound("cursor");
				playedNextSound = true;
			}
		}
		else { playedNextSound = false; }

		if (playHovered)
		{
			drawList->AddText(
				font,
				smallFontSize,
				ImVec2(playBtnX + (playBtnWidth - playSize.x) * 0.5f, playBtnY + (playBtnHeight - playSize.y) * 0.5f),
				IM_COL32(255, 255, 0, 255),
				playText
			);
		}
		else
		{
			DrawTextWithShadow(
				font,
				smallFontSize,
				ImVec2(playBtnX + (playBtnWidth - playSize.x) * 0.5f, playBtnY + (playBtnHeight - playSize.y) * 0.5f),
				IM_COL32(255, 255, 255, 127),
				playText,
				1.5,
				1,
				IM_COL32(0, 0, 0, 127)
			);
		}

		//ImU32 playColor = playHovered ? IM_COL32(30, 180, 255, 255) : IM_COL32(0, 140, 200, 255);
		//drawList->AddRectFilled(ImVec2(playBtnX, playBtnY), ImVec2(playBtnX + playBtnWidth, playBtnY + playBtnHeight), playColor, Scale(8.0f));


		//drawList->AddText(font, descFontSize, ImVec2(playBtnX + (playBtnWidth - playSize.x) * 0.5f, playBtnY + (playBtnHeight - playSize.y) * 0.5f), IM_COL32(220, 235, 255, 255), playText);

		if (playHovered && io.MouseClicked[0])
		{
			Game_PlaySound("decide");
			s_isVisible = false;
		}
		break;
	}

	case WizardPage::InstallFailed:
	{
		const char* errorMsg = g_installerErrorMessage.empty() ? Localise("Installer_Page_InstallFailed").c_str() : g_installerErrorMessage.c_str();

		ImVec2 errorTextSize = MeasureCentredParagraph(g_fntSeurat, descFontSize, textMaxWidth, textMargin, errorMsg);
		ImVec2 errorTextPosition = ImVec2(panelMin.x + (panelWidth - errorTextSize.x) * 0.5f, originMin.y);

		DrawWizardParagraph(errorMsg, textMaxWidth, textMargin, errorTextPosition);

		auto retryText = Localise("Installer_Button_Retry").c_str();
		auto retrySize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, retryText);

		float retryBtnWidth = 50.0f * scale;
		float retryBtnHeight = 20.0f * scale;
		float retryBtnX = panelMin.x + ((panelMax.x - panelMin.x) - retryBtnWidth) * 0.5f;
		float retryBtnY = panelMax.y - retryBtnHeight - (70.0f * scale);

		bool retryHovered = false;
		if (io.MousePos.x >= retryBtnX && io.MousePos.x <= retryBtnX + retryBtnWidth &&
			io.MousePos.y >= retryBtnY && io.MousePos.y <= retryBtnY + retryBtnHeight)
		{
			retryHovered = true;
			if(!playedNextSound)
			{
				Game_PlaySound("cursor");
				playedNextSound = true;
			}
		}
		else { playedNextSound = false; }

		if (retryHovered)
		{
			drawList->AddText(
				font,
				smallFontSize,
				ImVec2(retryBtnX + (retryBtnWidth - retrySize.x) * 0.5f, retryBtnY + (retryBtnHeight - retrySize.y) * 0.5f),
				IM_COL32(255, 255, 0, 255),
				retryText
			);
		}
		else
		{
			DrawTextWithShadow(
				font,
				smallFontSize,
				ImVec2(retryBtnX + (retryBtnWidth - retrySize.x) * 0.5f, retryBtnY + (retryBtnHeight - retrySize.y) * 0.5f),
				IM_COL32(255, 255, 255, 127),
				retryText,
				1.5,
				1,
				IM_COL32(0, 0, 0, 127)
			);
		}

		if (retryHovered && io.MouseClicked[0])
		{
			Game_PlaySound("decide");
			if (g_installerThread && g_installerThread->joinable())
				g_installerThread->join();
			g_installerThread.reset();

			g_currentPage = WizardPage::SelectGame;
			g_statusMessage.clear();
			g_installerFinished.store(false);
			g_installerFailed.store(false);
			g_installerCompiling.store(false);
			g_installerProgress = 0.0f;
		}

		DrawQuitButton(panelMin, panelMax, scale);

		break;
	}
	}
}

void InstallerWizard::Shutdown()
{
	if (g_installerThread && g_installerThread->joinable())
		g_installerThread->join();
	g_installerThread.reset();

	g_currentPage = WizardPage::SelectLanguage;
	g_isoPath.clear();
	g_statusMessage.clear();
	g_installerFinished.store(false);
	g_installerFailed.store(false);
	g_installerProgress = 0.0f;
}

bool InstallerWizard::Run(std::filesystem::path installPath)
{
	g_installPath = installPath / "game";
	g_isQuitting = false;
	g_currentPage = WizardPage::SelectLanguage;

	EmbeddedPlayer::Init();
	NFD_Init();

	GameWindow::SetFullscreenCursorVisibility(true);
	s_isVisible = true;

	constexpr auto frameTime = std::chrono::duration<float, std::milli>(1000.0f / 30.0f);

	while (s_isVisible)
	{
		auto frameStart = std::chrono::steady_clock::now();

		Video::WaitOnSwapChain();

		//Ideally, no music. Always found music on installers incredibly annoying
		//EmbeddedPlayer::PlayMusic();

		SDL_PumpEvents();

		if (g_browseClicked)
		{
			g_browseClicked = false;

			nfdu8filteritem_t filters[] = { {"Xbox ISO", "iso"} };
			nfdu8char_t* outPath = nullptr;
			nfdresult_t result = NFD_OpenDialogU8(&outPath, filters, 1, nullptr);

			if (result == NFD_OKAY && outPath != nullptr)
			{
				g_isoPath = outPath;
				g_statusMessage.clear();
				NFD_FreePathU8(outPath);
			}
			else if (result == NFD_ERROR)
			{
				g_statusMessage = NFD_GetError();
			}
		}

		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT)
			{
				if (g_currentPage == WizardPage::Installing)
					continue;

				g_isQuitting = true;
				s_isVisible = false;
			}
		}

		GameWindow::Update();
		input::Update();
		Video::Present();

		std::this_thread::sleep_until(frameStart + frameTime);
	}

	SDL_CaptureMouse(SDL_FALSE);

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);

	GameWindow::SetFullscreenCursorVisibility(false);

	Config::Fullscreen = GameWindow::IsFullscreen();
	if (!GameWindow::IsFullscreen() && !GameWindow::IsMaximised())
	{
		Config::WindowX = GameWindow::s_x;
		Config::WindowY = GameWindow::s_y;
		Config::WindowWidth = GameWindow::s_width;
		Config::WindowHeight = GameWindow::s_height;
	}
	Config::Save();

	NFD_Quit();
	InstallerWizard::Shutdown();
	EmbeddedPlayer::Shutdown();

	return !g_isQuitting;
}



void InstallerWizard::Init()
{
	g_fntSeurat = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-DB.otf");
	g_upTexOptionsBG = LOAD_ZSTD_TEXTURE(g_options_bg);
	g_upTexOptionsBG2 = LOAD_ZSTD_TEXTURE(g_options_bg2);
}
