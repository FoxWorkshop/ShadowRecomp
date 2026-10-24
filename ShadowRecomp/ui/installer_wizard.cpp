#include "installer_wizard.h"

#include <nfd.h>
#include <audio/embedded_player.h>
#include <install/installer.h>
#include <gpu/video.h>
#include <gpu/frame_pacing/frame_pacer.h>
#include <locale/locale.h>
#include <user/config.h>
#include <ui/game_window.h>
#include <os/logger.h>
#include <input/input.h>
#include <thread>
#include <atomic>
#include <exports.h>
#include <gpu/imgui/imgui_snapshot.h>
#include <ui/imgui_utils.h>
#include <ui/fader.h>
#include <res/images/common/TitleBg.png.h>
#include <res/images/common/Options2.png.h>
#include <decompressor.h>
#include <kernel/io/file_system.h>


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

static ImFont* g_fntSeurat;

// TODO: Probably nuke all this since this is just unleashed stuff
// Keeping for now incase we add the blue 'advertise' animation
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

static int g_selectedBtn = -1;

static WizardPage g_currentPage = WizardPage::SelectLanguage;
static std::filesystem::path g_installPath;
static std::string g_sourcePath;
static std::string g_statusMessage;

static std::unique_ptr<std::thread> g_installerThread;
static std::atomic<bool> g_installerFinished{ false };
static std::atomic<bool> g_installerFailed{ false };
static std::string g_installerErrorMessage;
static float g_installerProgress = 0.0f;
static std::atomic<float> g_installerProgressTarget{ 0.0f };
static std::atomic<const char*> g_installerCurrentFileName{ "" };
static std::atomic<bool> g_installerCompiling{ false };

static constexpr float INSTALL_EXTRACT_RATIO_END = 0.8f;

static bool g_currentCursorAccepted = false;
static bool g_currentCursorBack = false;
static int g_currentCursorIndex = -1;
static int g_currentCursorDefault = 0;
static bool g_isQuitting = false;
static bool g_browseClicked = false;
static double g_sourceDropTime = -1.0;
static float g_openProgress = 0.0f;
static bool g_isClosing = false;

static std::vector<std::pair<ImVec2, ImVec2>> g_currentCursorRects;

static bool playedNextSound = false;
static bool playedPrevSound = false;
static bool playedLangSound[6] = {false};
static bool playedBrowseSound = false;

static void Quit()
{
	if (g_isQuitting)
	{
		return;
	}

	g_isQuitting = true;
	g_isClosing = true;

	EmbeddedPlayer::FadeOutMusic();
	Fader::FadeOut(1, []() { InstallerWizard::s_isVisible = false; });
}

static std::string& GetWizardText(WizardPage page)
{
	switch (page)
	{
	case WizardPage::SelectLanguage: return Localise("Installer_Page_SelectLanguage");
	case WizardPage::Introduction: return Localise("Installer_Page_Introduction");
		// case WizardPage::CheckSpace: return Localise("Installer_Page_CheckSpace"); // TODO: Should probably implement this esp for cloud drive?
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
	else
	{
		playedLangSound[i] = false;
	}

	if (btnHovered && io.MouseClicked[0] && !buttonEnabled)
	{
		Game_PlaySound("decide");
		buttonPressed = true;
		g_selectedBtn = i;
	}

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
	auto progressCallback = [](const Installer::Progress& progress)
		{
			g_installerCurrentFileName.store(progress.currentFileName);
			g_installerProgressTarget.store(progress.ratio * INSTALL_EXTRACT_RATIO_END);
		};

	std::filesystem::path sourcePath(std::u8string_view(reinterpret_cast<const char8_t*>(g_sourcePath.c_str())));
	std::string errorMessage;
	bool success = Installer::Install(sourcePath, g_installPath, progressCallback, errorMessage);
	if (success)
	{
		g_installerCompiling.store(true);

		Video::PrecompileShadersAndPipelinesForInstall([](float progress)
			{
				g_installerProgressTarget.store(INSTALL_EXTRACT_RATIO_END + (1.0f - INSTALL_EXTRACT_RATIO_END) * progress);
			});

		g_installerCompiling.store(false);
		g_installerProgressTarget.store(1.0f);
	}
	else
	{
		LOGF_ERROR("Installing from {} failed: {}", g_sourcePath, errorMessage);

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
	else
	{
		playedNextSound = false;
	}

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
		Game_PlaySound("cancel");
		Quit();
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
	else
	{
		playedPrevSound = false;
	}

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
		Game_PlaySound("cancel");
		g_currentPage = (WizardPage)((int)g_currentPage - 1);
	}
}

// TODO: Maybe re-use this for the credits scroll on final page
// Otherwise scrap it, its unused since we copied to Options
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

	// Bevel at edges
	ImVec2 min = { winPos.x + offsetX + 21.0f * scale, winPos.y + offsetY + 54.0f * scale };
	ImVec2 max = { min.x + 598.0f * scale, min.y + 362.0f * scale };

	DrawShadowWindow(drawList, min, max, scale, g_openProgress);
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

	// prevent the anim stutter
	if (ImGui::GetTime() < 1.0)
	{
		return;
	}

	// recreate the message box opening and closing anim
	g_openProgress = StepOpenProgress(g_openProgress, g_isClosing);
	if (g_isClosing && g_openProgress <= 0.0f)
	{
		if (!g_isQuitting && !EmbeddedPlayer::IsPlayingSound())
		{
			s_isVisible = false;
		}
		return;
	}

	DrawBackground(viewport, winPos, winSize, scale, offsetX, offsetY);
	if (g_openProgress < 1.0f)
	{
		return;
	}

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

		const std::string displayPathStorage = g_sourcePath.empty() ? Localise("Installer_Page_NoFileSelected") : std::string(g_sourcePath);
		const char* displayPath = displayPathStorage.c_str();

		auto displayTextSize = font->CalcTextSizeA(fileFontSize, FLT_MAX, 0.0f, displayPath);
		bool pathOverflows = displayTextSize.x > pathClipWidth;
		std::string overflowRender;
		const char* pathToShow = displayPath;

		if (pathOverflows)
		{
			// TODO: Maybe can this crap if we do drag/drop and typing? just scrollable with end shown
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

		// Create a short yellow flash on the field to confirm the drop worked
		if (g_sourceDropTime >= 0.0)
		{
			float flash = 1.0f - float(ImGui::GetTime() - g_sourceDropTime) / 0.5f;
			if (flash > 0.0f)
				drawList->AddRect(inputMin, inputMax, IM_COL32(255, 255, 0, int(255 * flash)), 0.0f, 0, 2.0f * scale);
			else
				g_sourceDropTime = -1.0;
		}

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
		else
		{
			playedBrowseSound = false;
		}

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
			float statusPadding = 6.0f * scale;
			float statusMaxWidth = panelWidth - (statusPadding * 2.0f);
			float statusFontSize = smallFontSize;
			auto statusSize = font->CalcTextSizeA(statusFontSize, FLT_MAX, 0.0f, g_statusMessage.c_str());
			if (statusSize.x > statusMaxWidth)
			{
				statusFontSize = std::floor(statusFontSize * (statusMaxWidth / statusSize.x));
			}
			drawList->AddText(font, statusFontSize, ImVec2(panelMin.x + statusPadding, panelMin.y + statusPadding), IM_COL32(255, 100, 100, 255), g_statusMessage.c_str(), nullptr, statusMaxWidth);
		}

		if (!g_sourcePath.empty())
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
		const char* headerText;
		if (g_installerCompiling.load())
		{
			headerText = Localise("Installer_Page_Compiling").c_str();
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

		if (!g_installerCompiling.load())
		{
			const char* currentFileName = g_installerCurrentFileName.load();
			if (*currentFileName != '\0')
			{
				char fileText[512];
				auto fileTxt = Localise("Common_File") + ": %s";
				snprintf(fileText, sizeof(fileText), fileTxt.c_str(), currentFileName);
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

		if (playHovered && io.MouseClicked[0])
		{
			Game_PlaySound("decide");
			g_isClosing = true;
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
		else
		{
			playedNextSound = false;
		}

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
	g_sourcePath.clear();
	g_statusMessage.clear();
	g_sourceDropTime = -1.0;
	g_installerFinished.store(false);
	g_installerFailed.store(false);
	g_installerProgress = 0.0f;
}

// Dropping a file on the window works the same as Browse and a dropped folder
// is taken as the extracted game.
static void OnSourceDropped(const char* file)
{
	if (g_currentPage != WizardPage::SelectGame || g_isQuitting)
		return;

	std::error_code ec;
	std::filesystem::path path(std::u8string_view(reinterpret_cast<const char8_t*>(file)));

	if (std::filesystem::is_directory(path, ec))
		path /= GameExecutableFile;

	std::u8string extensionStr = path.extension().u8string();
	std::string lowerExtension(extensionStr.begin(), extensionStr.end());
	std::transform(lowerExtension.begin(), lowerExtension.end(), lowerExtension.begin(), [](unsigned char c) { return std::tolower(c); });

	if ((lowerExtension != ".iso" && lowerExtension != ".xbe") || !std::filesystem::is_regular_file(path, ec))
	{
		Game_PlaySound("cannot_decide");
		g_statusMessage = Localise("Installer_Message_InvalidFileSelected");
		return;
	}

	std::u8string pathString = path.u8string();
	g_sourcePath.assign(pathString.begin(), pathString.end());
	g_statusMessage.clear();
	g_sourceDropTime = ImGui::GetTime();
	Game_PlaySound("decide");
}

bool InstallerWizard::Run(std::filesystem::path installPath)
{
	g_installPath = installPath / "game";
	g_isQuitting = false;
	g_isClosing = false;
	g_openProgress = 0.0f;
	g_currentPage = WizardPage::SelectLanguage;

	EmbeddedPlayer::Init();
	NFD_Init();

	GameWindow::SetFullscreenCursorVisibility(true);
	s_isVisible = true;

	FramePacer framePacer;
	framePacer.SetFpsLimit(60.0);

	while (s_isVisible)
	{
		if (!g_isQuitting)
		{
			EmbeddedPlayer::PlayMusic();
		}

		SDL_PumpEvents();

		if (g_browseClicked)
		{
			g_browseClicked = false;

			nfdu8filteritem_t filters[] = { {"Xbox ISO or default.xbe", "iso,xbe"} };
			nfdu8char_t* outPath = nullptr;
			nfdresult_t result = NFD_OpenDialogU8(&outPath, filters, 1, nullptr);

			if (result == NFD_OKAY && outPath != nullptr)
			{
				g_sourcePath = outPath;
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
			if (event.type == SDL_EVENT_QUIT)
			{
				if (g_currentPage == WizardPage::Installing)
					continue;

				if (g_isQuitting)
				{
					s_isVisible = false;
				}
				else
				{
					Quit();
				}
			}
			else if (event.type == SDL_EVENT_DROP_FILE)
			{
				OnSourceDropped(event.drop.data);
			}
		}

		GameWindow::Update();
		input::Update();
		Video::Present();

		framePacer.EndFrame(FramePacer::WaitMode::SleepOnly);
	}

	SDL_CaptureMouse(false);

	SDL_PumpEvents();
	SDL_FlushEvents(SDL_EVENT_FIRST, SDL_EVENT_LAST);

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
