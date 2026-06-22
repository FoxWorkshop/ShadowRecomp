#include "installer_wizard.h"

#include <nfd.h>

#include <audio/embedded_player.h>
#include <install/xiso_extract.h>
#include <gpu/video.h>
#include <locale/locale.h>
#include <user/config.h>
#include <ui/game_window.h>
#include <ui/imgui_utils.h>
#include <os/logger.h>

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
static std::unique_ptr<GuestTexture> g_upTexBGMenuList{};

enum class WizardPage
{
	SelectLanguage,
	Introduction,
	SelectISO,
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
static float descFontSize = 18.0f;

constexpr float CONTAINER_X = 153.0f;
constexpr float CONTAINER_Y = 100.f;
constexpr float CONTAINER_WIDTH = 526.5f;
constexpr float CONTAINER_HEIGHT = 246.0f;
constexpr float SIDE_CONTAINER_WIDTH = CONTAINER_WIDTH / 2.0f;


constexpr float CONTAINER_BUTTON_WIDTH = 250.0f;
constexpr float CONTAINER_BUTTON_GAP = 9.0f;
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

static bool g_currentCursorAccepted = false;
static bool g_currentCursorBack = false;
static int g_currentCursorIndex = -1;
static int g_currentCursorDefault = 0;
static bool g_isQuitting = false;
static bool g_browseClicked = false;

static std::vector<std::pair<ImVec2, ImVec2>> g_currentCursorRects;

static std::string& GetWizardText(WizardPage page)
{
	switch (page)
	{
	case WizardPage::SelectLanguage: return Localise("Installer_Page_SelectLanguage");
	case WizardPage::Introduction: return Localise("Installer_Page_Introduction");
		// case WizardPage::CheckSpace: return Localise("Installer_Page_CheckSpace");
	case WizardPage::SelectISO: return Localise("Installer_Page_SelectGame");
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


static void ComputeButtonColumnCoordinates(ButtonColumn buttonColumn, ImVec2 panelMin, ImVec2 panelMax,
	float& minX, float& maxX)
{

	float panelWidth = panelMax.x - panelMin.x;

	// Scale button width proportionally to panel, with a cap so it
	// doesn't get absurdly wide on large panels.
	float buttonWidth = std::min(panelWidth * 0.38f, Scale(CONTAINER_BUTTON_WIDTH));
	float gap = panelWidth * 0.04f; // ~4% margin from edges

	switch (buttonColumn)
	{
	case ButtonColumnLeft:
		minX = panelMin.x + gap;
		maxX = panelMin.x + gap + buttonWidth;
		break;
	case ButtonColumnMiddle:
		minX = panelMin.x + panelWidth * 0.5f - buttonWidth * 0.5f;
		maxX = panelMin.x + panelWidth * 0.5f + buttonWidth * 0.5f;
		break;
	case ButtonColumnRight:
		minX = panelMax.x - gap - buttonWidth;
		maxX = panelMax.x - gap;
		break;
	}
}

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
	float alpha = ComputeMotionInstaller(g_appearTime, g_disappearTime, CONTAINER_INNER_TIME, CONTAINER_INNER_DURATION);
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

	}


	if (btnHovered && io.MouseClicked[0])
	{
		Game_PlaySound("cursor");
		buttonPressed = true;
		g_selectedBtn = i;
	}

	int baser = 0;
	int baseg = 0;


	DrawButtonContainer(min, max, baser, baseg, alpha);

	ImFont* font = g_fntSeurat;
	float size = Scale(sourceButton ? 12.5f : 16.0f);
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



	DrawTextWithOutline
	(
		font,
		size,
		min,
		buttonPressed || btnHovered || buttonEnabled ? IM_COL32(255, 255, 0, 255) : IM_COL32(255, 255, 255, 255),
		buttonText,
		4,
		IM_COL32(baser, baseg, 0, 255)
	);

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

			if (progress.phase == XisoExtract::Phase::Hashing)
			{
				g_installerProgressTarget.store(progress.ratio);
			}
			else
			{
				g_installerProgressTarget.store(progress.ratio);
			}
		};

	bool success = XisoExtract::Extract(g_isoPath, g_installPath, progressCallback);

	g_installerProgressTarget.store(1.0f);

	if (!success)
	{
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
	float gap = panelWidth * 0.08f;


	float bottomMargin = panelHeight * 0.25f;
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
			Config::Language = LANGUAGE_ENUM[i];
	}
}

static void DrawNextButton(ImVec2& panelMin, ImVec2& panelMax)
{

	auto& io = ImGui::GetIO();
	auto drawList = ImGui::GetBackgroundDrawList();
	auto& displaySize = io.DisplaySize;
	auto* font = g_fntSeurat;

	float nextBtnWidth = Scale(100.0f);
	float nextBtnHeight = Scale(36.0f);
	float nextBtnX = panelMax.x - nextBtnWidth - Scale(10);
	float nextBtnY = panelMax.y - nextBtnHeight - Scale(10);

	bool nextHovered = false;
	if (io.MousePos.x >= nextBtnX && io.MousePos.x <= nextBtnX + nextBtnWidth &&
		io.MousePos.y >= nextBtnY && io.MousePos.y <= nextBtnY + nextBtnHeight)
	{
		nextHovered = true;
	}

	ImU32 nextColor = nextHovered ? IM_COL32(30, 180, 255, 255) : IM_COL32(0, 140, 200, 255);
	drawList->AddRectFilled(ImVec2(nextBtnX, nextBtnY), ImVec2(nextBtnX + nextBtnWidth, nextBtnY + nextBtnHeight), nextColor, Scale(6.0f));

	auto nextText = Localise("Installer_Button_Next").c_str();
	auto nextSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, nextText);
	drawList->AddText(font, smallFontSize, ImVec2(nextBtnX + (nextBtnWidth - nextSize.x) * 0.5f, nextBtnY + Scale(9.0f)), IM_COL32(220, 235, 255, 255), nextText);

	if (nextHovered && io.MouseClicked[0])
	{
		Game_PlaySound("decide");
		g_currentPage = (WizardPage)((int)g_currentPage + 1);
	}

}

static void DrawPrevButton(ImVec2& panelMin, ImVec2& panelMax)
{

	auto& io = ImGui::GetIO();
	auto drawList = ImGui::GetBackgroundDrawList();
	auto& displaySize = io.DisplaySize;
	auto* font = g_fntSeurat;

	float prevBtnWidth = Scale(100.0f);
	float prevBtnHeight = Scale(36.0f);
	float prevBtnX = panelMin.x + Scale(10);
	float prevBtnY = panelMax.y - prevBtnHeight - Scale(10);

	bool prevHovered = false;
	if (io.MousePos.x >= prevBtnX && io.MousePos.x <= prevBtnX + prevBtnWidth &&
		io.MousePos.y >= prevBtnY && io.MousePos.y <= prevBtnY + prevBtnHeight)
	{
		prevHovered = true;
	}

	ImU32 prevColor = prevHovered ? IM_COL32(140, 50, 50, 255) : IM_COL32(100, 30, 30, 255);
	drawList->AddRectFilled(ImVec2(prevBtnX, prevBtnY), ImVec2(prevBtnX + prevBtnWidth, prevBtnY + prevBtnHeight), prevColor, Scale(6.0f));

	auto prevText = Localise("Common_Back").c_str();
	auto prevSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, prevText);
	drawList->AddText(font, smallFontSize, ImVec2(prevBtnX + (prevBtnWidth - prevSize.x) * 0.5f, prevBtnY + Scale(9.0f)), IM_COL32(255, 200, 200, 255), prevText);

	if (prevHovered && io.MouseClicked[0])
	{
		Game_PlaySound("decide");
		g_currentPage = (WizardPage)((int)g_currentPage - 1);
	}

}

static void DrawDescription(std::string s)
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


	float fontSize = g_fntSeuratSize - 2.f;
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


	SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);

	drawList->AddText(g_fntSeurat, fontSize, textPos, IM_COL32(255, 255, 255, 255), s.c_str());

	if (isScrolling)
	{
		// Draw the looping copy trailing behind
		ImVec2 loopPos = { textPos.x + totalWidth, textPos.y };
		drawList->AddText(g_fntSeurat, fontSize, loopPos, IM_COL32(255, 255, 255, 255), s.c_str());
	}

	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);

	drawList->PopClipRect();
}



static void DrawBackground()
{

	auto& res = ImGui::GetIO().DisplaySize;
	auto drawList = ImGui::GetBackgroundDrawList();

	ImVec2 min = { g_horzCentre + g_aspectRatioNarrowMargin, g_vertCentre };
	ImVec2 max = { res.x - min.x, res.y - min.y };

	float borderPixels = 32.f; // Options BG has an extra black border at the bottom, we don't want that.
	float uvMaxY = 1.0f - (borderPixels / 512.0f);

	ImVec2 centre = { res.x / 2.f, res.y / 2.f };
	float scale = 1.0f;




	drawList->AddImage
	(
		g_upTexOptionsBG.get(),
		{ 0.0f, 0.0f },
		{ res.x, res.y },
		{ 0, 0 },
		{ 1, uvMaxY },
		IM_COL32(255, 255, 255, 255)
	);


	// draw Options Logo

	if (scale >= 0.8f)
	{
		auto optionsUVs = PIXELS_TO_UV_COORDS(512, 512, 0, 0, 147, 448);
		auto optionsWidth = res.x * 0.16f;
		auto optionsHeight = res.y * 0.96f;

		ImVec2 optionsMin = { 0, min.y + Scale(10) };
		ImVec2 optionsMax = { optionsMin.x + optionsWidth, optionsMin.y + optionsHeight };

		//drawList->AddImage(g_upTexBGOptions.get(), optionsMin, optionsMax, GET_UV_COORDS(optionsUVs), IM_COL32(255, 255, 255, 128));
	}
}

void InstallerWizard::Draw()
{
	if (!s_isVisible)
		return;

	auto& io = ImGui::GetIO();
	auto drawList = ImGui::GetBackgroundDrawList();
	auto& displaySize = io.DisplaySize;


	drawList->AddRectFilled(ImVec2(0, 0), displaySize, IM_COL32(20, 20, 30, 255));

	auto* font = g_fntSeurat;

	// Panel is centred in the viewport and capped so it doesn't get
	// absurdly wide on ultrawide / high-resolution displays. All sizes
	// below are derived from the panel rectangle so the layout scales
	// correctly with both window resizing and High DPI.
	float panelWidth = std::min(displaySize.x * 0.7f, Scale(800.0f));
	float panelHeight = std::min(displaySize.y * 0.72f, Scale(640.0f));
	float panelMinX = (displaySize.x - panelWidth) * 0.5f;
	float panelMinY = (displaySize.y - panelHeight) * 0.5f;

	float verticalOffset = Scale(40.0f);

	ImVec2 panelMin(panelMinX, panelMinY + verticalOffset);
	ImVec2 panelMax(panelMinX + panelWidth, panelMinY + panelHeight + verticalOffset);

	float centerX = (panelMin.x + panelMax.x) * 0.5f;
	float centerY = (panelMin.y + panelMax.y) * 0.5f;

	// Font sizes scale with the panel so they stay readable at any DPI.
	float baseFontSize = std::max(Scale(12.0f), std::floor(std::min(panelWidth, panelHeight) / 22.0f));
	fontSize = baseFontSize;
	descFontSize = baseFontSize + std::floor(baseFontSize * 0.2f);
	smallFontSize = std::floor(baseFontSize * 0.80f);

	DrawBackground();

	drawList->AddRectFilled(panelMin, panelMax, IM_COL32(30, 30, 50, 128), Scale(8.0f)); 
	drawList->AddRect(panelMin, panelMax, IM_COL32(80, 80, 120, 128), Scale(8.0f), 0, Scale(1.0f)); // border

	auto titleText = "Shadow the Hedgehog Recompiled - Setup";
	auto titleSize = font->CalcTextSizeA(fontSize + 2.f, FLT_MAX, 0, titleText);
	float titleX = centerX - titleSize.x * 0.5f;


	float contentY = panelMin.y + Scale(80.0f);

	std::string wizardText = GetWizardText(g_currentPage);
	char descriptionText[512];
	strncpy(descriptionText, wizardText.c_str(), sizeof(descriptionText) - 1);
	auto originMargin = Scale(38);
	auto originMin = ImVec2(panelMin.x + originMargin, panelMin.y + originMargin);
	auto originMax = ImVec2(panelMax.x - originMargin, panelMax.y - originMargin);
	auto textMargin = 5.0f;

	auto textMaxWidth = originMax.x - (descFontSize / 2.0f) - originMin.x;

	DrawDescription("Shadow the Hedgehog - Installer");

	if (g_currentPage != WizardPage::Installing && g_currentPage != WizardPage::InstallFailed)
	{
			DrawTextParagraph(g_fntSeurat, descFontSize, textMaxWidth, originMin, textMargin, wizardText.c_str(),
				[&](const char* str, ImVec2 pos) { DrawTextBasic(g_fntSeurat, descFontSize, pos, IM_COL32(255, 255, 255, 255), str); });	
	}

	switch (g_currentPage)
	{
	case WizardPage::SelectLanguage:
	{

		DrawLanguagePicker(panelMin, panelMax);


		DrawNextButton(panelMin, panelMax);

		float quitBtnWidth = Scale(100.0f);
		float quitBtnHeight = Scale(36.0f);
		float quitBtnX = panelMin.x + Scale(10);
		float quitBtnY = panelMax.y - quitBtnHeight - Scale(10);
		bool quitHovered = false;
		if (io.MousePos.x >= quitBtnX && io.MousePos.x <= quitBtnX + quitBtnWidth &&
			io.MousePos.y >= quitBtnY && io.MousePos.y <= quitBtnY + quitBtnHeight)
		{
			quitHovered = true;
		}

		ImU32 quitColor = quitHovered ? IM_COL32(140, 50, 50, 255) : IM_COL32(100, 30, 30, 255);
		drawList->AddRectFilled(ImVec2(quitBtnX, quitBtnY), ImVec2(quitBtnX + quitBtnWidth, quitBtnY + quitBtnHeight), quitColor, Scale(6.0f));

		auto quitText = Localise("Common_Quit").c_str();
		auto quitSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, quitText);
		drawList->AddText(font, smallFontSize, ImVec2(quitBtnX + (quitBtnWidth - quitSize.x) * 0.5f, quitBtnY + Scale(9.0f)), IM_COL32(255, 200, 200, 255), quitText);

		if (quitHovered && io.MouseClicked[0])
		{
			g_isQuitting = true;
			s_isVisible = false;
		}

	}
	break;
	case WizardPage::Introduction:
	{
		DrawPrevButton(panelMin, panelMax);
		DrawNextButton(panelMin, panelMax);
	}
	break;
	case WizardPage::SelectISO:
	{

		contentY += Scale(50.0f);

		float inputPadding = Scale(20.0f);
		float inputWidth = panelWidth - inputPadding * 2.0f;
		float inputHeight = Scale(32.0f);
		ImVec2 inputMin(panelMin.x + inputPadding, contentY);
		ImVec2 inputMax(panelMin.x + inputPadding + inputWidth, contentY + inputHeight);

		drawList->AddRectFilled(inputMin, inputMax, IM_COL32(40, 40, 60, 255), Scale(4.0f));
		drawList->AddRect(inputMin, inputMax, IM_COL32(100, 100, 140, 255), Scale(4.0f), 0, Scale(1.0f));

		const char* displayPath = g_isoPath.empty() ? Localise("Installer_Page_NoFileSelected").c_str() : g_isoPath.c_str();
		if (strlen(displayPath) > 60)
		{
			displayPath = displayPath + strlen(displayPath) - 60;
		}


		drawList->AddText(font, smallFontSize, ImVec2(inputMin.x + Scale(8.0f), inputMin.y + Scale(7.0f)), IM_COL32(180, 180, 180, 255), displayPath, nullptr, inputWidth - Scale(100.0f));

		float btnWidth = Scale(120.0f);
		float btnHeight = Scale(32.0f);
		float btnX = inputMax.x - btnWidth - Scale(4.0f);
		float btnY = inputMin.y;

		bool btnHovered = false;
		if (io.MousePos.x >= btnX && io.MousePos.x <= btnX + btnWidth &&
			io.MousePos.y >= btnY && io.MousePos.y <= btnY + btnHeight)
		{
			btnHovered = true;
		}

		ImU32 btnColor = btnHovered ? IM_COL32(80, 80, 140, 255) : IM_COL32(60, 60, 100, 255);
		drawList->AddRectFilled(ImVec2(btnX, btnY), ImVec2(btnX + btnWidth, btnY + btnHeight), btnColor, Scale(4.0f));
		auto browseText = Localise("Installer_Header_Browse").c_str();
		auto browseSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, browseText);
		drawList->AddText(font, smallFontSize, ImVec2(btnX + (btnWidth - browseSize.x) * 0.5f, btnY + Scale(7.0f)), IM_COL32(220, 220, 255, 255), browseText);

		if (btnHovered && io.MouseClicked[0])
		{
			Game_PlaySound("cursor");
			g_browseClicked = true;
		}

		contentY += inputHeight + Scale(30.0f);

		if (!g_statusMessage.empty())
		{
			drawList->AddText(font, smallFontSize, ImVec2(panelMin.x + inputPadding, contentY), IM_COL32(255, 100, 100, 255), g_statusMessage.c_str());
			contentY += Scale(30.0f);
		}

		if (!g_isoPath.empty())
		{
			float nextBtnWidth = Scale(100.0f);
			float nextBtnHeight = Scale(36.0f);
			float nextBtnX = panelMax.x - nextBtnWidth - Scale(10);
			float nextBtnY = panelMax.y - nextBtnHeight - Scale(10);

			bool nextHovered = false;
			if (io.MousePos.x >= nextBtnX && io.MousePos.x <= nextBtnX + nextBtnWidth &&
				io.MousePos.y >= nextBtnY && io.MousePos.y <= nextBtnY + nextBtnHeight)
			{
				nextHovered = true;
			}

			ImU32 nextColor = nextHovered ? IM_COL32(30, 180, 255, 255) : IM_COL32(0, 140, 200, 255);
			drawList->AddRectFilled(ImVec2(nextBtnX, nextBtnY), ImVec2(nextBtnX + nextBtnWidth, nextBtnY + nextBtnHeight), nextColor, Scale(6.0f));

			auto nextText = Localise("Common_Install").c_str();
			auto nextSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, nextText);
			drawList->AddText(font, smallFontSize, ImVec2(nextBtnX + (nextBtnWidth - nextSize.x) * 0.5f, nextBtnY + Scale(9.0f)), IM_COL32(220, 235, 255, 255), nextText);

			if (nextHovered && io.MouseClicked[0])
			{
				Game_PlaySound("decide");
				g_currentPage = WizardPage::Installing;
				g_installerFinished.store(false);
				g_installerFailed.store(false);
				g_installerProgress = 0.0f;
				g_installerProgressTarget.store(0.0f);
				g_installerThread = std::make_unique<std::thread>(InstallerThread);
			}
		}

		DrawPrevButton(panelMin, panelMax);
	}
	break;
	case WizardPage::Installing:
	{
		DrawDescription("Shadow the Hedgehog - Installer");

		auto phase = g_installerPhase.load();
		const char* headerText = (phase == XisoExtract::Phase::Hashing)
			? Localise("Installer_Page_Verifying").c_str()
			: Localise("Installer_Page_Installing").c_str();

		DrawTextParagraph(font, descFontSize, textMaxWidth, originMin, textMargin, headerText,
			[&](const char* str, ImVec2 pos) { DrawTextBasic(g_fntSeurat, descFontSize, pos, IM_COL32(255, 255, 255, 255), str); });
		//drawList->AddText(font, descFontSize, ImVec2(panelMin.x + Scale(20), contentY), IM_COL32(220, 220, 255, 255), headerText);

		contentY += Scale(50.0f);

		float barPadding = Scale(20.0f);
		float barX = panelMin.x + barPadding;
		float barWidth = panelWidth - barPadding * 2.0f;
		float barHeight = Scale(24.0f);

		if (phase != XisoExtract::Phase::Hashing)
		{
			if (!g_installerCurrentFileName.empty())
			{
				char fileText[512];
				auto fileTxt = Localise("Common_File") + ": %s";
				snprintf(fileText, sizeof(fileText), fileTxt.c_str(), g_installerCurrentFileName.c_str());
				drawList->AddText(font, smallFontSize, ImVec2(barX, contentY), IM_COL32(180, 180, 180, 255), fileText, nullptr, barWidth);
			}
		}

		contentY += Scale(30.0f);

		ImVec2 barMin(barX, centerY - barHeight * 0.5f);
		ImVec2 barMax(barX + barWidth, centerY + barHeight * 0.5f);

		float target = g_installerProgressTarget.load();
		g_installerProgress += (target - g_installerProgress) * 0.1f;
		if (std::abs(g_installerProgress - target) < 0.001f)
			g_installerProgress = target;

		drawList->AddRectFilled(barMin, barMax, IM_COL32(40, 40, 60, 255), Scale(4.0f));

		float fillWidth = barWidth * std::min(g_installerProgress, 1.0f);
		if (fillWidth > 0)
		{
			ImVec2 fillMax(barX + fillWidth, centerY + barHeight * 0.5f);
			drawList->AddRectFilled(barMin, fillMax, IM_COL32(50, 150, 50, 255), Scale(4.0f));
		}

		char percentText[32];
		snprintf(percentText, sizeof(percentText), "%.1f%%", g_installerProgress * 100.0f);
		auto percentSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, percentText);
		drawList->AddText(font, smallFontSize, ImVec2(barX + (barWidth - percentSize.x) * 0.5f, centerY - percentSize.y * 0.5f), IM_COL32(255, 255, 255, 255), percentText);

		contentY += barHeight + Scale(20.0f);

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


		auto foxDev = "FoxWorkShop";
		bool playHovered = false;


		auto foxSize = font->CalcTextSizeA(descFontSize, FLT_MAX, 0, foxDev);
		drawList->AddText(font, descFontSize, ImVec2(centerX - foxSize.x * 0.5f, centerY - foxSize.y - Scale(48.0f)), IM_COL32(220, 255, 220, 255), foxDev);


		float playBtnWidth = Scale(250.0f);
		float playBtnHeight = Scale(58.0f);
		float playBtnX = centerX - playBtnWidth * 0.5f;
		float playBtnY = centerY + Scale(24.0f);

		if (io.MousePos.x >= playBtnX && io.MousePos.x <= playBtnX + playBtnWidth &&
			io.MousePos.y >= playBtnY && io.MousePos.y <= playBtnY + playBtnHeight)
		{
			playHovered = true;
		}

		ImU32 playColor = playHovered ? IM_COL32(30, 180, 255, 255) : IM_COL32(0, 140, 200, 255);
		drawList->AddRectFilled(ImVec2(playBtnX, playBtnY), ImVec2(playBtnX + playBtnWidth, playBtnY + playBtnHeight), playColor, Scale(8.0f));

		auto playText = Localise("Installer_Button_PlayGame").c_str();

		auto playSize = font->CalcTextSizeA(descFontSize, FLT_MAX, 0, playText);
		drawList->AddText(font, descFontSize, ImVec2(playBtnX + (playBtnWidth - playSize.x) * 0.5f, playBtnY + (playBtnHeight - playSize.y) * 0.5f), IM_COL32(220, 235, 255, 255), playText);

		if (playHovered && io.MouseClicked[0])
		{
			Game_PlaySound("decide");
			s_isVisible = false;
		}

		break;
	}

	case WizardPage::InstallFailed:
	{

		contentY += Scale(50.0f);
		float errorPadding = Scale(20.0f);

		const char* errorMsg = g_installerErrorMessage.empty() ? Localise("Installer_Page_InstallFailed").c_str() : g_installerErrorMessage.c_str();
		drawList->AddText(font, descFontSize, ImVec2(panelMin.x + errorPadding, contentY), IM_COL32(255, 180, 180, 255), errorMsg, nullptr, panelWidth - errorPadding * 2.0f);

		float retryBtnWidth = Scale(120.0f);
		float retryBtnHeight = Scale(40.0f);
		float retryBtnX = centerX - retryBtnWidth - Scale(10.0f);
		float retryBtnY = panelMax.y - retryBtnHeight - Scale(30.0f);

		bool retryHovered = false;
		if (io.MousePos.x >= retryBtnX && io.MousePos.x <= retryBtnX + retryBtnWidth &&
			io.MousePos.y >= retryBtnY && io.MousePos.y <= retryBtnY + retryBtnHeight)
		{
			retryHovered = true;
		}

		ImU32 retryColor = retryHovered ? IM_COL32(80, 80, 140, 255) : IM_COL32(60, 60, 100, 255);
		drawList->AddRectFilled(ImVec2(retryBtnX, retryBtnY), ImVec2(retryBtnX + retryBtnWidth, retryBtnY + retryBtnHeight), retryColor, Scale(6.0f));

		auto retryText = Localise("Installer_Button_Retry").c_str();
		auto retrySize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, retryText);
		drawList->AddText(font, smallFontSize, ImVec2(retryBtnX + (retryBtnWidth - retrySize.x) * 0.5f, retryBtnY + (retryBtnHeight - retrySize.y) * 0.5f), IM_COL32(220, 220, 255, 255), retryText);

		if (retryHovered && io.MouseClicked[0])
		{
			if (g_installerThread && g_installerThread->joinable())
				g_installerThread->join();
			g_installerThread.reset();

			g_currentPage = WizardPage::SelectISO;
			g_statusMessage.clear();
			g_installerFinished.store(false);
			g_installerFailed.store(false);
			g_installerProgress = 0.0f;
		}

		float quitBtnWidth = Scale(120.0f);
		float quitBtnHeight = Scale(40.0f);
		float quitBtnX = centerX + Scale(10.0f);
		float quitBtnY = panelMax.y - quitBtnHeight - Scale(30.0f);

		bool quitHovered = false;
		if (io.MousePos.x >= quitBtnX && io.MousePos.x <= quitBtnX + quitBtnWidth &&
			io.MousePos.y >= quitBtnY && io.MousePos.y <= quitBtnY + quitBtnHeight)
		{
			quitHovered = true;
		}

		ImU32 quitColor = quitHovered ? IM_COL32(140, 50, 50, 255) : IM_COL32(100, 30, 30, 255);
		drawList->AddRectFilled(ImVec2(quitBtnX, quitBtnY), ImVec2(quitBtnX + quitBtnWidth, quitBtnY + quitBtnHeight), quitColor, Scale(6.0f));

		auto quitText = Localise("Common_Quit").c_str();
		auto quitSize = font->CalcTextSizeA(smallFontSize, FLT_MAX, 0, quitText);
		drawList->AddText(font, smallFontSize, ImVec2(quitBtnX + (quitBtnWidth - quitSize.x) * 0.5f, quitBtnY + (quitBtnHeight - quitSize.y) * 0.5f), IM_COL32(255, 200, 200, 255), quitText);

		if (quitHovered && io.MouseClicked[0])
		{
			g_isQuitting = true;
			s_isVisible = false;
		}

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

		EmbeddedPlayer::PlayMusic();

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
	g_upTexBGMenuList = LOAD_ZSTD_TEXTURE(g_optionsIcons);
}