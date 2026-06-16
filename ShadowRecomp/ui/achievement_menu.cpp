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
#include <user/achievement_manager.h>
#include "button_guide.h"

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

static constexpr int MAX_VISIBLE_ROWS = 4;
static constexpr double CONTAINER_MOVE_DURATION = 5;
static constexpr double CONTAINER_MOVE_OFFSET = 0;
static constexpr double CONTAINER_FADE_OFFSET = CONTAINER_MOVE_OFFSET;
static constexpr double CONTAINER_FADE_DURATION = 10;
static constexpr double ROW_FADE_OFFSET = CONTAINER_MOVE_DURATION;
static constexpr double ROW_FADE_TIME = CONTAINER_MOVE_DURATION;

static constexpr double ROW_SELECT_OFFSET = CONTAINER_FADE_DURATION;
static constexpr double ROW_SELECT_TIME = CONTAINER_FADE_DURATION;

static bool g_isControlsVisible = false;

static ImFont* g_fntSeurat;

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
		Game_PlaySound("cursor");

		g_rowSelectionTime = time;
		g_scrollArrowsTime = g_rowSelectionTime;
	}
}

void AchievementMenu::Open()
{

	if (s_isVisible)
		return;

	g_achievements.clear();

	for (auto& achievement : GetAchievements(Config::Language.Value))
	{
		//achievement.LockedDesc = xdbf::FixInvalidSequences(achievement.LockedDesc);

		g_achievements.push_back(std::make_tuple(achievement, AchievementManager::GetTimestamp(achievement.ID)));
	}

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

	Button buttons =
	{
		Button("Common_Back", 115.0f, EButtonIcon::Y, EButtonAlignment::Left, &g_isControlsVisible),
	};

	ButtonGuide::Open(buttons);
	ButtonGuide::SetSideMargins(100);

}


void AchievementMenu::Close()
{
	if (s_state == AchievementMenuState::ClosingAchievements)
		return;

	g_isControlsVisible = false;
	s_state = AchievementMenuState::ClosingAchievements;
	g_time = ImGui::GetTime();
	ButtonGuide::Close();

}
static float g_bgScale = 1.0f;

static void DrawContainer()
{
	if (g_bgScale < 0.9f)
		return;

	auto& res = ImGui::GetIO().DisplaySize;
	auto drawList = ImGui::GetBackgroundDrawList();

	ImVec2 min = { g_horzCentre + g_aspectRatioNarrowMargin, g_vertCentre };
	ImVec2 max = { res.x - min.x, res.y - min.y };

	auto borderUVs = PIXELS_TO_UV_COORDS(512, 370, 505, 0, 6, 325);
	auto borderWidth = 6.f;
	auto borderHeight = res.y * 0.8f;


	// center vertically
	float borderY = (res.y - borderHeight) / 2.f;

	// left bar
	ImVec2 leftMin = { res.x * 0.1f, borderY };
	ImVec2 leftMax = { leftMin.x + borderWidth, leftMin.y + borderHeight };
	drawList->AddImage(g_TexAchievementBtn.get(), leftMin, leftMax, GET_UV_COORDS(borderUVs));

	// right bar - mirror of left
	ImVec2 rightMin = { res.x * 0.9f - borderWidth, borderY };
	ImVec2 rightMax = { rightMin.x + borderWidth, rightMin.y + borderHeight };
	drawList->AddImage(g_TexAchievementBtn.get(), rightMin, rightMax, GET_UV_COORDS(borderUVs));


	// top bar
	auto topBarUVs = PIXELS_TO_UV_COORDS(512, 370, 0, 329, 512, 10);
	auto topWidth = rightMax.x - leftMax.x;
	auto topHeight = 10.f;
	float topY = (res.y * 0.1f) - topHeight;

	ImVec2 topMin = { leftMin.x, topY };
	ImVec2 topMax = { topMin.x + topWidth, topMin.y + topHeight };
	drawList->AddImage(g_TexAchievementBtn.get(), topMin, topMax, GET_UV_COORDS(topBarUVs));


	topMin.y = (res.y * 0.1f);
	drawList->AddLine(
		{ topMin.x, topMin.y },  // left point
		{ topMax.x, topMin.y },  // right point
		IM_COL32(255, 255, 255, 100),
		2.f // thickness
	);

}

static void DrawBackground()
{
	auto& res = ImGui::GetIO().DisplaySize;
	auto drawList = ImGui::GetBackgroundDrawList();


	ImVec2 centre = { res.x / 2.f, res.y / 2.f };

	g_bgScale = 1.0f;

	if (AchievementMenu::s_state == AchievementMenuState::ClosingAchievements)
		g_bgScale = (float)ComputeEaseOut(AchievementMenu::g_BackgroundAnimClose, 0.5, true);
	else
		g_bgScale = (float)ComputeEaseOut(AchievementMenu::g_BackgroundAnimIntro, 0.5, false);

	// animate from centre outward to fullscreen
	ImVec2 bgHalfSize = { (res.x / 2.f) * g_bgScale, (res.y / 2.f) * g_bgScale };
	ImVec2 bgMin = { centre.x - bgHalfSize.x, centre.y - bgHalfSize.y };
	ImVec2 bgMax = { centre.x + bgHalfSize.x, centre.y + bgHalfSize.y };

	drawList->AddImage
	(
		g_TexAchievementBG.get(),
		bgMin,
		bgMax,
		{ 0, 0 },
		{ 1, 1 },
		IM_COL32(255, 255, 255, 255)
	);

	if (g_bgScale <= 0.1f && AchievementMenu::s_state == AchievementMenuState::ClosingAchievements)
	{
		AchievementMenu::s_isVisible = false;
	}


	DrawContainer();
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


static void DrawAchievement(int rowIndex, Achievement& achievement, bool isUnlocked)
{
	auto drawList = ImGui::GetBackgroundDrawList();
	auto clipRectMin = drawList->GetClipRectMin();
	auto clipRectMax = drawList->GetClipRectMax();

	auto itemHeight = Scale(84);
	auto itemMarginX = Scale(18);
	auto imageMarginX = Scale(34);
	auto imageMarginY = Scale(10);
	auto imageSize = Scale(64);

	auto offsetScroll = 0.0f;
	auto startIndex = 0;

	// Only scroll if page has more than four items.
	if (g_rowCount > MAX_VISIBLE_ROWS)
	{
		if (g_selectedIndex >= g_rowCount - (MAX_VISIBLE_ROWS - 1))
		{
			// Stop scrolling near bottom to use cursor instead.
			startIndex = g_rowCount - MAX_VISIBLE_ROWS;
			offsetScroll = -startIndex * itemHeight;
		}
		else if (g_selectedIndex >= 1)
		{
			// Start scrolling from the middle item.
			startIndex = g_selectedIndex - 1;
			offsetScroll = -startIndex * itemHeight;
		}
	}

	auto offsetY = itemHeight * rowIndex + offsetScroll;
	auto isCurrent = g_selectedIndex == rowIndex;
	auto isVisible = ((rowIndex - startIndex + g_rowCount) % g_rowCount) < MAX_VISIBLE_ROWS;

	if (!isVisible)
		return;

	ImVec2 min = { clipRectMin.x + itemMarginX, clipRectMin.y + offsetY };
	ImVec2 max = { clipRectMax.x - itemMarginX, min.y + itemHeight };


	float inset = Scale(4.f);

	ImVec2 borderMin = { min.x, min.y + inset };
	ImVec2 borderMax = { max.x, max.y - inset };

	drawList->AddRectFilled(borderMin, borderMax, IM_COL32(255, 255, 255, 20));

	if (isCurrent)
	{
		constexpr auto rowColourAlphaMax = 255;
		constexpr auto rowColourAlphaMin = 0;
		auto rowColourBreatheMotionTime = BREATHE_MOTION(1.0f, 0.0f, g_rowSelectionTime, 0.9f);
		auto rowColourTransitionMotionTime = ComputeLinearMotion(g_time, ROW_SELECT_OFFSET, ROW_SELECT_TIME, false);

		auto rowColourYellow = isCurrent
			? Lerp(rowColourAlphaMax, rowColourAlphaMin, rowColourBreatheMotionTime) * rowColourTransitionMotionTime
			: rowColourAlphaMin * rowColourTransitionMotionTime;

		auto rowColour = IM_COL32(255, 255, rowColourYellow, 128);
		drawList->AddRect(borderMin, borderMax, rowColour);
	}



	auto rowUVs = PIXELS_TO_UV_COORDS(1024, 1024, 605, 449, 10, 30);
	auto rowMarginX = Scale(2);
	auto rowMarginY = Scale(24);

	auto rowColourBreatheMotionTime = BREATHE_MOTION(1.0f, 0.0f, g_rowSelectionTime, 0.9f);
	auto rowColourTransitionMotionTime = ComputeLinearMotion(g_time, ROW_FADE_OFFSET, ROW_FADE_TIME, false);

	constexpr auto rowColourAlphaMax = 165;
	constexpr auto rowColourAlphaMin = 115;

	auto rowColourAlpha = isCurrent
		? Lerp(rowColourAlphaMax, rowColourAlphaMin, rowColourBreatheMotionTime) * rowColourTransitionMotionTime
		: rowColourAlphaMin * rowColourTransitionMotionTime;

	auto rowColour = IM_COL32(255, 255, 255, rowColourAlpha);

	ImVec2 rowMin = { clipRectMin.x + rowMarginX, min.y + itemHeight - rowMarginY };
	ImVec2 rowMax = { clipRectMax.x - rowMarginX, rowMin.y + Scale(30) };


	SetAdditive(true);
	SetVerticalGradient(rowMin, rowMax, IM_COL32_WHITE_TRANS, rowColour);
	//drawList->AddImage(g_upTexMainMenu1.get(), rowMin, rowMax, GET_UV_COORDS(rowUVs), rowColour);
	ResetGradient();
	ResetAdditive();

	auto image = achievement.pImageBuffer;
	ImVec2 imageMin = { min.x + imageMarginX, min.y + imageMarginY };
	ImVec2 imageMax = { min.x + imageMarginX + imageSize, min.y + imageMarginY + imageSize };

	if (!isUnlocked)
		SetShaderModifier(IMGUI_SHADER_MODIFIER_GRAYSCALE);

	// Draw achievement image.
	drawList->AddImage((GuestTexture*)image, imageMin, imageMax, {0, 0}, {1, 1}, IM_COL32_WHITE);
	drawList->AddRect(imageMin, imageMax, IM_COL32(255, 255, 255, 100), 0, 0, 2.f);

	if (!isUnlocked)
		SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);

	auto fontSize = Scale(Config::Language == ELanguage::Japanese ? 18 : 17);

	auto textX = imageMax.x + itemMarginX * 2;
	auto textColour = isUnlocked ? IM_COL32_WHITE : IM_COL32(137, 137, 137, 255);

	std::string name;
	std::string lockedDesc;
	std::string unlockedDesc;



	auto& newLocale = GetAchievementLocale(achievement.ID);

	name = newLocale.Name;
	lockedDesc = newLocale.LockedDesc;
	unlockedDesc = newLocale.UnlockedDesc;


	auto descText = isUnlocked ? unlockedDesc.c_str() : lockedDesc.c_str();
	auto descTextOffsetY = Scale(32);
	auto descSize = g_fntSeurat->CalcTextSizeA(fontSize, FLT_MAX, 0, descText);

	ImVec2 namePos = { textX, imageMin.y + Scale(3) };
	ImVec2 descPos = { textX, namePos.y + descTextOffsetY };

	// Draw achievement name.
	SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);
	drawList->AddText(g_fntSeurat, fontSize, namePos, textColour, name.c_str());
	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);

	auto timestampOffsetX = Scale(60);

	ImVec2 marqueeMin = { textX, min.y };
	ImVec2 marqueeMax = { max.x - timestampOffsetX, max.y };
	constexpr auto marqueeDelay = 0.9;
	auto shouldMarquee = isCurrent && marqueeMin.x + descSize.x >= marqueeMax.x;

	// Reduce clip rect size when marquee starts to make the
	// scrolling text align more with text above or below it.
	if (shouldMarquee && ImGui::GetTime() - (g_rowSelectionTime + marqueeDelay) > 0.0)
	{
		marqueeMin.x += Scale(2);
		marqueeMax.x -= Scale(1);
	}

	SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);
	DrawTextWithMarquee(g_fntSeurat, fontSize, descPos, marqueeMin, marqueeMax, textColour, descText, g_rowSelectionTime, marqueeDelay, shouldMarquee ? Scale(200) : 0);
	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);

	if (!isUnlocked)
		return;

	auto timestamp = AchievementManager::GetTimestamp(achievement.ID);

	if (!timestamp)
		return;

	char buffer[32];

#ifdef _WIN32
	tm time{};
	tm* pTime = &time;
	localtime_s(pTime, &timestamp);
#else
	tm* pTime = localtime(&timestamp);
#endif

	snprintf(buffer, sizeof(buffer), "%d/%d/%d %02d:%02d",
		pTime->tm_year + 1900, pTime->tm_mon + 1, pTime->tm_mday, pTime->tm_hour, pTime->tm_min);

	auto timestampSize = g_fntSeurat->CalcTextSizeA(fontSize, FLT_MAX, 0, buffer);

	// Draw timestamp text.
	SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);
	drawList->AddText(g_fntSeurat, fontSize, { max.x - timestampSize.x - timestampOffsetX, namePos.y }, IM_COL32_WHITE, buffer);
	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);
}

void AchievementMenu::Draw()
{

	if (!s_isVisible)
		return;


	auto& res = ImGui::GetIO().DisplaySize;
	float containerLeft = res.x * 0.1f;
	float containerRight = res.x * 0.9f;
	float containerTop = res.y * 0.15f;
	float containerBottom = res.y * 0.9f; 
	auto drawList = ImGui::GetBackgroundDrawList();



	switch (s_state)
	{
	case AchievementMenuState::Transition:
	{

		if (AchievementMenu::s_nextState == AchievementMenuState::Achievements)
		{
			AchievementMenu::g_BackgroundAnimIntro = ImGui::GetTime();
		}

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

		DrawBackground();



		// DrawContainer(min, max);

		if (g_bgScale >= 0.9)
		{
			g_isControlsVisible = true;

			auto rowIndex = 0;

			drawList->PushClipRect(
				{ containerLeft, containerTop },
				{ containerRight, containerBottom },
				true // intersect with current clip rect
			);

			for (auto& tpl : g_achievements)
			{
				auto& achievement = std::get<0>(tpl);
					DrawAchievement(rowIndex++, achievement, AchievementManager::IsUnlocked(achievement.ID));
			}


			drawList->PopClipRect();
			auto scrollArrowsOffsetX = Scale(40);
			auto scrollArrowsOffsetTop = Scale(40);
			auto scrollArrowsOffsetBottom = Scale(70);
			ImVec2 min = { containerRight - containerLeft, g_vertCentre + containerTop };
			ImVec2 max = { min.x + containerLeft, g_vertCentre + containerBottom };
			ImVec2 scrollArrowsMin = { max.x + scrollArrowsOffsetX, min.y + scrollArrowsOffsetTop };
			ImVec2 scrollArrowsMax = { scrollArrowsMin.x + scrollArrowsOffsetX, max.y - scrollArrowsOffsetBottom };

			DrawScrollArrows(scrollArrowsMin, scrollArrowsMax, Scale(15), g_scrollArrowsTime, g_selectedIndex > MAX_VISIBLE_ROWS / 2, g_selectedIndex < g_rowCount - MAX_VISIBLE_ROWS / 2);

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
				Game_PlaySound("cancel");
			}

			MoveCursor(g_selectedIndex, 0, g_rowCount);

			g_up = false;
			g_upWasHeld = upIsHeld;
			g_down = false;
			g_downWasHeld = downIsHeld;
		}


		break;
	}

	case AchievementMenuState::ClosingAchievements:

		DrawBackground();


		g_up = false;
		g_upWasHeld = false;
		g_down = false;
		g_downWasHeld = false;
		break;
	}



}

void AchievementMenu::Init()
{
	g_TexAchievementBG = LOAD_ZSTD_TEXTURE(g_achievementBG);
	g_TexAchievementBtn = LOAD_ZSTD_TEXTURE(g_achievementBtn);
	g_fntSeurat = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-DB.otf");
}