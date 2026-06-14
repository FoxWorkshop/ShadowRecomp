#include <ui/options_menu.h>
#include <locale/locale.h>
#include <kernel/memory.h>
#include <gpu/video.h>
#include <res/images/common/TitleBg.png.h>
#include <res/images/common/Options.png.h>
#include <res/images/common/Options2.png.h>
#include <res/images/common/gamemenu.png.h>
#include <decompressor.h>
#include <patches/aspect_ratio_patches.h>
#include <patches/bloom_patches.h>

#include "imgui_utils.h"
#include <gpu/imgui/imgui_snapshot.h>
#include <ui/fader.h>

#include <input/padstate.h>
#include <ui/message_window.h>
#include <ui/game_window.h>
#include <user/config.h>
#include <app.h>
#include <exports.h>
#include <ui/menu_common.h>
#include <ui/button_guide.h>
#include <patches/pause_patches.h>


static constexpr int MAX_VISIBLE_ROWS = 5;
static IConfigDef* g_optionCurrent{};
static bool g_optionCanReset{};
static std::unique_ptr<GuestTexture> g_upTexOptionsBG{};
static std::unique_ptr<GuestTexture> g_upTexBGMenuList{};
static std::unique_ptr<GuestTexture> g_upTexBGOptions{}; // flame, options logo, hud etc.
static std::unique_ptr<GuestTexture> g_upTexGameMenuText{}; // select, exit text etc.
static ImFont* g_fntSeurat;

static int g_categoryIndex{ -1 };
static int g_optionIndex{};
static int g_optionCount{};

static double g_stateTime{};
static double g_categoryTime{};
static double g_cursorArrowsTime{};
static double g_flowStateTime{};
static double g_lastIncrementTime{};
static double g_lastTappedTime{};
static double g_scrollArrowsTime{};
static double g_containerAnimTime = 0.0;
static double g_containerClosedAnimTime = 0.0;

static bool g_up{};
static bool g_upWasHeld{};
static bool g_down{};
static bool g_downWasHeld{};
static bool g_left{};
static bool g_leftWasHeld{};
static bool g_right{};
static bool g_rightWasHeld{};
static bool g_isAccepted{};
static bool g_isDeclined{};
static bool g_isLeaving{};
static bool g_isReset{};
static bool g_isControlsVisible = false;

bool m_isDescScrolling{};
bool m_isDescManualScrolling{};
float m_descScrollOffset{};
float m_descScrollTimer{};
float m_descScrollDirection{ 1.0f };

static double s_optionsOpenTime = -1.0;
static double s_optionsCloseTime = -1.0;


static std::string optionDescription = "";
static std::string optionValDescription = "";

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

static bool CheckAndDiscard(bool& value)
{
	if (value)
	{
		value = false;
		return true;
	}

	return false;
}



void resetDescScroll()
{
	m_isDescScrolling = false;
	m_descScrollOffset = 0.0f;
	m_descScrollTimer = 0.0f;
	m_descScrollDirection = 1.0f;
}


void DrawDescription(std::string s)
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


//Returns truncated string with ellipsis if it exceeds maxWidth
static std::string TruncateTextWithEllipsis(ImFont* font, float fontSize, const std::string& text, float maxWidth)
{
	auto fullSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, text.c_str());
	if (fullSize.x <= maxWidth)
		return text;

	const std::string ellipsis = "...";
	auto ellipsisSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, ellipsis.c_str());
	float budget = maxWidth - ellipsisSize.x;

	std::string result;
	result.reserve(text.size());
	for (size_t i = 0; i < text.size(); )
	{
		// Advance by one UTF-8 codepoint
		unsigned char c = (unsigned char)text[i];
		size_t charLen = (c < 0x80) ? 1 : (c < 0xE0) ? 2 : (c < 0xF0) ? 3 : 4;
		std::string candidate = result + text.substr(i, charLen);
		auto candidateSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, candidate.c_str());
		if (candidateSize.x > budget)
			break;
		result = std::move(candidate);
		i += charLen;
	}
	return result + ellipsis;
}

template <typename T, bool isHidden = false>
static void DrawOption
(
	int rowIndex,
	ConfigDef<T, isHidden>* config,
	bool isAccessible,
	ImVec2 min,
	ImVec2 max,
	std::string* inaccessibleReason = nullptr,
	T valueMin = T(0),
	T valueCentre = T(0.5),
	T valueMax = T(1),
	bool isSlider = true,
	bool isInterpolatedString = false
)
{
	auto drawList = ImGui::GetBackgroundDrawList();
	auto clipRectMin = drawList->GetClipRectMin();
	auto clipRectMax = drawList->GetClipRectMax();

	auto optionMotionTime = 1.0f; // ComputeLinearMotion(g_categoryTime, 0, 5, OptionsMenu::s_state != OptionsMenuState::Idle);

	auto optionHeight = Scale(56);
	float offsetScroll = 0.0f;

	bool isMiddleRow = false;
	// original offset from the container
	float optionsOffsetX = Scale(170);
	float optionsOffsetY = Scale(23);

	// removing offset will make option draw right when the container starts, so we add an extra scale to make it fit inside.
	min.x -= optionsOffsetX - Scale(10);
	max.x -= Scale(10);

	// Only scroll if page has more than 3 items.
	if (g_optionCount > MAX_VISIBLE_ROWS)
	{
		isMiddleRow = ((rowIndex - std::max(g_optionIndex - 1, 0)) % MAX_VISIBLE_ROWS) >= 1;

		if (g_optionIndex >= g_optionCount - (MAX_VISIBLE_ROWS - 1))
		{
			// Stop scrolling near bottom to use cursor instead.
			offsetScroll = -(g_optionCount - MAX_VISIBLE_ROWS) * optionHeight;
			isMiddleRow = ((rowIndex - std::max(g_optionCount - MAX_VISIBLE_ROWS, 0)) % MAX_VISIBLE_ROWS) >= 1;
		}
		else if (g_optionIndex >= 1)
		{
			// Start scrolling from the middle item.
			offsetScroll = -(g_optionIndex - 1) * optionHeight;
		}
	}
	else
	{
		isMiddleRow = rowIndex >= 1;
	}

	auto offsetY = optionHeight * rowIndex + offsetScroll;
	auto isCurrent = g_optionIndex == rowIndex;
	auto isSelected = isCurrent && OptionsMenu::s_flowState == OptionsMenuFlowState::OptionSelected;
	auto isHover = isCurrent && OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor;
	bool isHoverOrSelected = isSelected || isHover;

	constexpr bool isSliderType = std::is_same_v<T, float> || std::is_same_v<T, int>;

	auto bgEdgeUVs = PIXELS_TO_UV_COORDS(256, 256, 1, 0, 51, 45);
	auto bgStretchUVs = PIXELS_TO_UV_COORDS(256, 256, 51, 0, 51, 45);
	auto bgWidth = Scale(50);
	auto bgHeight = Scale(45);

	ImVec2 titleBgEdgeMin = { min.x, min.y + offsetY };
	ImVec2 titleBgEdgeMax = { titleBgEdgeMin.x + bgWidth, titleBgEdgeMin.y + bgHeight };
	ImVec2 titleBgStretchMin = { titleBgEdgeMax.x, titleBgEdgeMin.y };
	ImVec2 titleBgStretchMax = { max.x, max.y };
	ImVec2 titleBgFadeMin = { titleBgStretchMax.x - Scale(300), titleBgStretchMax.y };
	ImVec2 titleBgFadeMax = titleBgStretchMax;

	/**SetHorizontalGradient(titleBgFadeMin, titleBgFadeMax, bgColour, bgFadeColour);
	drawList->AddImage(g_upTexMainMenu8.get(), titleBgEdgeMin, titleBgEdgeMax, GET_UV_COORDS(bgEdgeUVs), bgColour);
	drawList->AddImage(g_upTexMainMenu8.get(), titleBgStretchMin, titleBgStretchMax, GET_UV_COORDS(bgStretchUVs), bgColour);
	ResetGradient();*/

	auto titleText = config->GetNameLocalised(Config::Language);
	auto titleTextSize = g_fntSeurat->CalcTextSizeA(g_fntSeuratSize, FLT_MAX, 0, titleText.c_str());

	ImVec2 titlePos = { titleBgEdgeMin.x, titleBgEdgeMin.y + Scale(8) };


	auto isTitleRightFade = !isMiddleRow && OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor;


	SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);

	drawList->AddText(g_fntSeurat, g_fntSeuratSize, titlePos, isHoverOrSelected ? IM_COL32(234, 234, 69, 255) : IM_COL32(255, 255, 255, 255), titleText.c_str());

	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);


	if (isCurrent)
	{
		if (OptionsMenu::s_flowState == OptionsMenuFlowState::OptionSelected)
		{

			g_optionCurrent = config;

			g_optionCanReset = isAccessible && config->GetName().find("Language") == std::string::npos && (void*)config != (void*)&Config::WindowSize;
		}
		else
		{
			g_optionCurrent = nullptr;
			g_optionCanReset = false;
		}

		if (OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor || OptionsMenu::s_flowState == OptionsMenuFlowState::OptionSelected)
		{
			optionDescription = config->GetDescription(Config::Language);
		}
	}



	if (isCurrent)
	{
		auto setValueDescription = [=]()
			{
				auto valueDescription = config->GetValueDescription(Config::Language);
				auto isLanguageOption = (ConfigDef<ELanguage>*)config == &Config::Language;
				if (valueDescription.empty())
				{
					optionValDescription = config->GetDescription(Config::Language);
				}
				else
				{
					optionValDescription = valueDescription;
				}
			};

		if (isAccessible)
		{
			static T s_oldValue;

			if (CheckAndDiscard(g_isAccepted))
			{
				Game_PlaySound("decide");

				if (OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor)
				{
					setValueDescription();

					s_oldValue = config->Value;

					if (config->LockCallback)
						config->LockCallback(config);

					OptionsMenu::SetFlowState(OptionsMenuFlowState::OptionSelected);

					isSelected = true;
				}
				else
				{
					if (config->Value != s_oldValue)
					{
						VideoConfigValueChangedCallback(config);

						if (config->ApplyCallback)
							config->ApplyCallback(config);
					}

					OptionsMenu::SetFlowState(OptionsMenuFlowState::OptionCursor);

					isSelected = false;
				}
			}

			if (CheckAndDiscard(g_isDeclined))
			{
				Game_PlaySound("cancel");

				if (config->Value != s_oldValue)
				{
					config->Value = s_oldValue;

					VideoConfigValueChangedCallback(config);

					if (config->Callback)
						config->Callback(config);

					if (config->ApplyCallback)
						config->ApplyCallback(config);
				}

				OptionsMenu::SetFlowState(OptionsMenuFlowState::OptionCursor);

				isSelected = false;
			}

			if (g_optionCanReset && CheckAndDiscard(g_isReset))
			{
				Game_PlaySound("cancel");

				if (!config->IsDefaultValue())
				{
					config->MakeDefault();

					VideoConfigValueChangedCallback(config);

					if (config->Callback)
						config->Callback(config);

					if (config->ApplyCallback)
						config->ApplyCallback(config);
				}
			}

			if (isSelected)
			{

				auto increment = g_right;
				auto decrement = g_left;

				if (increment || decrement)
					g_lastTappedTime = ImGui::GetTime();

				if constexpr (std::is_enum_v<T>)
				{
					auto it = config->EnumTemplateReverse.find(config->Value);

					if (decrement)
					{
						if (it == config->EnumTemplateReverse.begin())
							it = config->EnumTemplateReverse.end();

						--it;
					}
					else if (increment)
					{
						++it;

						if (it == config->EnumTemplateReverse.end())
							it = config->EnumTemplateReverse.begin();
					}

					config->Value = it->first;
					config->SnapToNearestAccessibleValue(increment);

					if (increment || decrement)
					{
						setValueDescription();

						Game_PlaySound("cursor");

						if (config->Callback)
							config->Callback(config);
					}
				}
				else if constexpr (isSliderType)
				{
					auto time = ImGui::GetTime();
					auto fastIncrement = isSlider && (g_leftWasHeld || g_rightWasHeld) && (time - g_lastTappedTime) > 0.5;
					auto playIncrementSound = true;

					static auto s_fastIncrementHoldTime = 0.0;
					static auto s_lastIncrementSoundTime = 0.0;

					static constexpr auto INCREMENT_TIME = 1.0 / 60.0;
					static constexpr auto INCREMENT_SOUND_TIME = 1.0 / 20.0;

					if (fastIncrement)
					{
						s_fastIncrementHoldTime += App::s_deltaTime;
					}
					else
					{
						s_fastIncrementHoldTime = 0.0;
					}

					if (fastIncrement)
					{
						playIncrementSound = (time - s_lastIncrementSoundTime) > INCREMENT_SOUND_TIME;

						if (s_fastIncrementHoldTime < INCREMENT_TIME)
						{
							fastIncrement = false;
						}
						else
						{
							g_lastIncrementTime = time;
						}
					}

					if (fastIncrement)
					{
						increment = g_rightWasHeld;
						decrement = g_leftWasHeld;
					}

					do
					{
						if constexpr (std::is_integral_v<T>)
						{
							if (decrement)
								config->Value -= 1;
							else if (increment)
								config->Value += 1;
						}
						else
						{
							if (decrement)
								config->Value -= 0.01f;
							else if (increment)
								config->Value += 0.01f;
						}

						if (fastIncrement)
							s_fastIncrementHoldTime -= INCREMENT_TIME;
					} while (fastIncrement && s_fastIncrementHoldTime >= INCREMENT_TIME);

					auto isValueInBounds = config->Value >= valueMin && config->Value <= valueMax;

					if ((increment || decrement) && isValueInBounds && playIncrementSound)
					{
						Game_PlaySound("cursor");

						s_lastIncrementSoundTime = time;
					}

					config->Value = std::clamp(config->Value, valueMin, valueMax);
				}
				else if constexpr (std::is_same_v<T, bool>)
				{
					if (increment || decrement)
					{
						Game_PlaySound("cursor");

						config->Value = !config->Value;
					}
				}

				if (config->Value != s_oldValue)
				{
					BloomConfigValueChangedCallback(config);
				}

				// Run standard callback if there's no callback on apply.
				if (!config->ApplyCallback)
				{
					if ((increment || decrement) && config->Callback)
						config->Callback(config);
				}

			}


		}
		else
		{
			if (CheckAndDiscard(g_isAccepted))
				Game_PlaySound("cannot_decide");
		}
	}


	std::string valueText;
	bool isValueCentred = false;

	if constexpr (std::is_same_v<T, float>)
	{
		if (config == &Config::ResolutionScale)
		{
			auto resScale = round(*(float*)config->GetValue() * 1000) / 1000;

			valueText = fmt::format("{}% - ({}x{})", round(resScale * 100),
				(int)((float)Video::s_viewportWidth * resScale),
				(int)((float)Video::s_viewportHeight * resScale));

		}
		else
		{
			valueText = fmt::format("{}%", int(round(config->Value * 100.0f)));
		}

	}
	else if constexpr (std::is_same_v<T, int32_t>)
	{
		if (config == &Config::WindowSize)
		{
			if (Config::Fullscreen)
			{
				int displayW, displayH;

				GameWindow::GetSizeInPixels(&displayW, &displayH);

				valueText = fmt::format("{}x{}", displayW, displayH);
			}
			else
			{
				auto displayModes = GameWindow::GetDisplayModes();

				if (config->Value >= 0 && config->Value < displayModes.size())
				{
					auto& displayMode = displayModes[config->Value];

					valueText = fmt::format("{}x{}", displayMode.w, displayMode.h);
				}
				else
				{
					valueText = fmt::format("{}x{}", GameWindow::s_width, GameWindow::s_height);
				}
			}

			isValueCentred = true;
		}
		else if (config == &Config::Monitor)
		{
			valueText = fmt::format("{}", config->Value + 1);
			isValueCentred = true;
		}
		else
		{
			valueText = fmt::format("{}", config->Value);

			if (isSlider && config->Value >= valueMax)
				valueText = Localise("Options_Value_Max");
		}
	}
	else
	{
		valueText = config->GetValueLocalised(Config::Language);
		isValueCentred = true;
	}

	// Draw value
	float valueAreaStart = titlePos.x + titleTextSize.x + Scale(30);
	float valueAreaEnd = clipRectMax.x - Scale(10);
	float valueAreaWidth = valueAreaEnd - valueAreaStart;

	SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);

	if (valueAreaWidth > 0.0f)
	{
		auto rawValueSize = g_fntSeurat->CalcTextSizeA(g_fntSeuratSize, FLT_MAX, 0.0f, valueText.data());
		bool overflows = rawValueSize.x > valueAreaWidth;

		ImVec2 valuePos = { valueAreaStart, titlePos.y };

		if (overflows && isHover)
		{
			// Marquee scroll: slide the text left over time, then snap back
			static double s_marqueeStartTime = 0.0;
			static int s_marqueeRow = -1;

			if (s_marqueeRow != rowIndex)
			{
				s_marqueeRow = rowIndex;
				s_marqueeStartTime = ImGui::GetTime();
			}

			constexpr double PAUSE_BEFORE = 1.0;   // seconds of pause before scrolling
			constexpr double PAUSE_AFTER = 0.75;  // seconds of pause once fully scrolled
			constexpr double SCROLL_SPEED = 40.0;  // pixels per second (pre-scale)
			const float  overflow = rawValueSize.x - valueAreaWidth;
			const float  scaledSpeed = (float)(Scale(1) * SCROLL_SPEED);
			const double scrollDur = overflow / scaledSpeed;
			const double cycleDur = PAUSE_BEFORE + scrollDur + PAUSE_AFTER;

			double elapsed = std::fmod(ImGui::GetTime() - s_marqueeStartTime, cycleDur);
			float  scrollX = 0.0f;

			if (elapsed > PAUSE_BEFORE && elapsed < PAUSE_BEFORE + scrollDur)
				scrollX = (float)((elapsed - PAUSE_BEFORE) * scaledSpeed);
			else if (elapsed >= PAUSE_BEFORE + scrollDur)
				scrollX = overflow;

			// we can use clip rect so the text can't bleed into the title area or off the right edge
			drawList->PushClipRect({ valueAreaStart, clipRectMin.y }, { valueAreaEnd, clipRectMax.y }, true);
			valuePos.x -= scrollX;
			drawList->AddText(g_fntSeurat, g_fntSeuratSize, valuePos,
				IM_COL32(255, 255, 255, 255), valueText.data());
			drawList->PopClipRect();
		}
		else
		{
			// selected or doesn't fit fine: truncate with ellipsis if needed
			std::string displayText = overflows
				? TruncateTextWithEllipsis(g_fntSeurat, g_fntSeuratSize, valueText, valueAreaWidth)
				: valueText;

			drawList->AddText(g_fntSeurat, g_fntSeuratSize, valuePos,
				IM_COL32(255, 255, 255, isAccessible ? 255 : 127), displayText.data());
		}
	}

}

static void DrawGoToLegacyOption
(
	int rowIndex,
	ImVec2 min,
	ImVec2 max
)
{
	auto drawList = ImGui::GetBackgroundDrawList();
	auto clipRectMin = drawList->GetClipRectMin();
	auto clipRectMax = drawList->GetClipRectMax();

	auto optionMotionTime = 1.0f; // ComputeLinearMotion(g_categoryTime, 0, 5, OptionsMenu::s_state != OptionsMenuState::Idle);

	auto optionHeight = Scale(56);
	float offsetScroll = 0.0f;

	bool isMiddleRow = false;
	// original offset from the container
	float optionsOffsetX = Scale(170);
	float optionsOffsetY = Scale(23);

	// removing offset will make option draw right when the container starts, so we add an extra scale to make it fit inside.
	min.x -= optionsOffsetX - Scale(10);
	max.x -= Scale(10);

	// Only scroll if page has more than 3 items.
	if (g_optionCount > MAX_VISIBLE_ROWS)
	{
		isMiddleRow = ((rowIndex - std::max(g_optionIndex - 1, 0)) % MAX_VISIBLE_ROWS) >= 1;

		if (g_optionIndex >= g_optionCount - (MAX_VISIBLE_ROWS - 1))
		{
			// Stop scrolling near bottom to use cursor instead.
			offsetScroll = -(g_optionCount - MAX_VISIBLE_ROWS) * optionHeight;
			isMiddleRow = ((rowIndex - std::max(g_optionCount - MAX_VISIBLE_ROWS, 0)) % MAX_VISIBLE_ROWS) >= 1;
		}
		else if (g_optionIndex >= 1)
		{
			// Start scrolling from the middle item.
			offsetScroll = -(g_optionIndex - 1) * optionHeight;
		}
	}
	else
	{
		isMiddleRow = rowIndex >= 1;
	}

	auto offsetY = optionHeight * rowIndex + offsetScroll;
	auto isCurrent = g_optionIndex == rowIndex;
	auto isSelected = isCurrent && OptionsMenu::s_flowState == OptionsMenuFlowState::OptionSelected;
	auto isHover = isCurrent && OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor;
	bool isHoverOrSelected = isSelected || isHover;

	auto bgEdgeUVs = PIXELS_TO_UV_COORDS(256, 256, 1, 0, 51, 45);
	auto bgStretchUVs = PIXELS_TO_UV_COORDS(256, 256, 51, 0, 51, 45);
	auto bgWidth = Scale(50);
	auto bgHeight = Scale(45);

	ImVec2 titleBgEdgeMin = { min.x, min.y + offsetY };
	ImVec2 titleBgEdgeMax = { titleBgEdgeMin.x + bgWidth, titleBgEdgeMin.y + bgHeight };
	ImVec2 titleBgStretchMin = { titleBgEdgeMax.x, titleBgEdgeMin.y };
	ImVec2 titleBgStretchMax = { max.x, max.y };
	ImVec2 titleBgFadeMin = { titleBgStretchMax.x - Scale(300), titleBgStretchMax.y };
	ImVec2 titleBgFadeMax = titleBgStretchMax;


	auto titleText = Localise("Options_Name_LegacyMenu");
	auto titleTextSize = g_fntSeurat->CalcTextSizeA(g_fntSeuratSize, FLT_MAX, 0, titleText.c_str());

	ImVec2 titlePos = { titleBgEdgeMin.x, titleBgEdgeMin.y + Scale(8) };


	auto isTitleRightFade = !isMiddleRow && OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor;


	SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);

	drawList->AddText(g_fntSeurat, g_fntSeuratSize - 2.f, titlePos, isHoverOrSelected ? IM_COL32(234, 234, 69, 255) : IM_COL32(255, 255, 255, 255), titleText.c_str());

	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);


	if (isCurrent)
	{
		if (OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor || OptionsMenu::s_flowState == OptionsMenuFlowState::OptionSelected)
		{
			optionDescription = Localise("Options_Desc_LegacyMenu");
		}

		if (CheckAndDiscard(g_isAccepted))
		{
			Game_PlaySound("decide");


			OptionsMenu::SetFlowState(OptionsMenuFlowState::CategoryCursor);
			isSelected = true;
			GameMenuCommon::g_LegacyOptions = true;
			OptionsMenu::s_GoToLegacyMenu = true;
		}
		else if (CheckAndDiscard(g_isDeclined))
		{
			Game_PlaySound("cancel");

			OptionsMenu::SetFlowState(OptionsMenuFlowState::OptionCursor);

			isSelected = false;
		}

	}
	else
	{
		if (CheckAndDiscard(g_isAccepted))
			Game_PlaySound("cannot_decide");
	}

	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);
}


static void DrawOptions(ImVec2 minClipRect, ImVec2 maxClipRect, ImVec2 min, ImVec2 max)
{
	auto drawList = ImGui::GetBackgroundDrawList();

	drawList->PushClipRect(minClipRect, maxClipRect);

	auto cmnReason = &Localise("Options_Desc_NotAvailable");
	auto devReason = &Localise("Options_Desc_NotImplemented");
	auto rowCount = 0;

#define ENUM_VALUE(type) (type)0, (type)0, (type)0

	switch ((OptionsMenuCategory)g_categoryIndex)
	{
	case OptionsMenuCategory::System:
		DrawOption(rowCount++, &Config::Language, !OptionsMenu::s_isPause, min, max, cmnReason);
		DrawOption(rowCount++, &Config::VoiceLanguage, !OptionsMenu::s_isPause, min, max, cmnReason);
		DrawOption(rowCount++, &Config::Subtitles, true, min, max);
		DrawOption(rowCount++, &Config::EnablePartnerIntro, true, min, max);
		DrawOption(rowCount++, &Config::Hints, !OptionsMenu::s_isPause, min, max);
		DrawOption(rowCount++, &Config::AchievementNotifications, true, min, max);
		break;

	case OptionsMenuCategory::Input:
		DrawOption(rowCount++, &Config::HorizontalCamera1P, true, min, max);
		DrawOption(rowCount++, &Config::HorizontalCamera2P, true, min, max);
		DrawOption(rowCount++, &Config::HorizontalCamera3P, true, min, max);
		DrawOption(rowCount++, &Config::HorizontalCamera4P, true, min, max);
		DrawOption(rowCount++, &Config::Vibration1P, true, min, max);
		DrawOption(rowCount++, &Config::Vibration2P, true, min, max);
		DrawOption(rowCount++, &Config::Vibration3P, true, min, max);
		DrawOption(rowCount++, &Config::Vibration4P, true, min, max);
		DrawOption(rowCount++, &Config::AllowBackgroundInput, true, min, max);
		DrawOption(rowCount++, &Config::ControllerIcons, !OptionsMenu::s_isPause, min, max, cmnReason);
		break;

	case OptionsMenuCategory::Audio:
		DrawOption(rowCount++, &Config::MasterVolume, true, min, max);
		DrawOption(rowCount++, &Config::MusicVolume, true, min, max);
		DrawOption(rowCount++, &Config::VoicesVolume, true, min, max);
		DrawOption(rowCount++, &Config::EffectsVolume, true, min, max);
		break;

	case OptionsMenuCategory::Video:
	{
		bool canChangeResolution = Config::Fullscreen == false;


		DrawOption(rowCount++, &Config::WindowSize, canChangeResolution, min, max, devReason, 0, 0, (int)GameWindow::GetDisplayModes().size() - 1, false);

		// DrawOption(rowCount++, &Config::WindowSize, !Config::Fullscreen,
		//     &Localise("Options_Desc_NotAvailableFullscreen"),
		//     0, 0, (int)GameWindow::GetDisplayModes().size() - 1, false);

		auto displayCount = GameWindow::GetDisplayCount();
		auto canChangeMonitor = Config::Fullscreen && displayCount > 1;
		auto monitorReason = &Localise("Options_Desc_NotAvailableWindowed");


		if (Config::Fullscreen && displayCount <= 1)
			monitorReason = &Localise("Options_Desc_NotAvailableHardware");

		DrawOption(rowCount++, &Config::Monitor, canChangeMonitor, min, max, devReason, 0, 0, 1, false);
		DrawOption(rowCount++, &Config::AspectRatio, true, min, max, devReason);
		DrawOption(rowCount++, &Config::ResolutionScale, true, min, max, nullptr, 0.25f, 1.0f, 2.0f);
		DrawOption(rowCount++, &Config::Fullscreen, true, min, max, devReason);
		DrawOption(rowCount++, &Config::VSync, true, min, max, nullptr);
		DrawOption(rowCount++, &Config::FPS, true, min, max, nullptr, FPS_MIN, 120, FPS_MAX);
		DrawOption(rowCount++, &Config::Brightness, true, min, max, nullptr);
		DrawOption(rowCount++, &Config::AntiAliasing, false, min, max, devReason);
		DrawOption(rowCount++, &Config::TextureFiltering, true, min, max, nullptr);
		//DrawOption(rowCount++, &Config::UIAlignmentMode, true, min, max, nullptr);
		DrawOption(rowCount++, &Config::BloomIntensity, true, min, max, nullptr);



		break;
	}

	case OptionsMenuCategory::Debug:
	{
		for (auto def : g_configDefinitions)
		{
			if (def->GetSection() != "Codes")
				continue;

			//  def->SetHidden(false);

			DrawOption(rowCount++, (ConfigDef<bool, true>*)def, true, min, max);
		}

		break;
	}
	case OptionsMenuCategory::Misc:
	{
		DrawGoToLegacyOption(rowCount++, min, max);
		if (CheckAndDiscard(g_isAccepted))
		{

		}
	}

	}

#undef ENUM_VALUE

	g_optionCount = rowCount;

	drawList->PopClipRect();

	if (g_optionCount > MAX_VISIBLE_ROWS && OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor)
	{
		auto& res = ImGui::GetIO().DisplaySize;
		auto scrollArrowsOffsetX = Scale(40);
		auto scrollArrowsOffsetTop = Scale(100);
		auto scrollArrowsOffsetBottom = Scale(25);
		float containerLeft = res.x * 0.1f;
		float containerRight = res.x * 0.9f;
		float containerTop = res.y * 0.15f;
		float containerBottom = res.y * 0.9f;
		ImVec2 min = { containerRight - containerLeft, g_vertCentre + containerTop};
		ImVec2 max = { min.x + containerLeft, g_vertCentre + containerBottom  };
		ImVec2 scrollArrowsMin = { max.x + scrollArrowsOffsetX, min.y + scrollArrowsOffsetTop };
		ImVec2 scrollArrowsMax = { scrollArrowsMin.x + scrollArrowsOffsetX, max.y - scrollArrowsOffsetBottom };

		DrawScrollArrows(scrollArrowsMin, scrollArrowsMax, Scale(15), g_scrollArrowsTime, g_optionIndex > 1, g_optionIndex < rowCount - 2);
	}

}



static void ResetCategorySelection()
{
	g_categoryIndex = -1;
}

static void ResetOptionSelection()
{
	g_optionIndex = 0;
	g_optionCurrent = nullptr;
	g_optionCanReset = false;
}

static void ResetSelection()
{
	auto time = ImGui::GetTime();

	g_flowStateTime = time;

	ResetCategorySelection();
	ResetOptionSelection();
}


static void MoveCursor(int& cursorIndex, double& cursorTime, int min = 0, int max = INT_MAX, std::function<void()> onCursorMoved = nullptr)
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

		cursorTime = time;

		if (onCursorMoved)
			onCursorMoved();
	}
}

void DrawBackground(bool isPause = false)
{

	auto& res = ImGui::GetIO().DisplaySize;
	auto drawList = ImGui::GetBackgroundDrawList();

	ImVec2 min = { g_horzCentre + g_aspectRatioNarrowMargin, g_vertCentre };
	ImVec2 max = { res.x - min.x, res.y - min.y };

	float borderPixels = 32.f; // Options BG has an extra black border at the bottom, we don't want that.
	float uvMaxY = 1.0f - (borderPixels / 512.0f);

	ImVec2 centre = { res.x / 2.f, res.y / 2.f };
	float scale = 1.0f;

	if (isPause)
	{
		if (OptionsMenu::s_state == OptionsMenuState::Closing || s_optionsCloseTime > s_optionsOpenTime)
			scale = (float)ComputeEaseOut(s_optionsCloseTime, 0.5, true); 
		else
			scale = (float)ComputeEaseOut(s_optionsOpenTime, 0.5, false);

		// animate from centre outward to fullscreen
		ImVec2 bgHalfSize = { (res.x / 2.f) * scale, (res.y / 2.f) * scale };
		ImVec2 bgMin = { centre.x - bgHalfSize.x, centre.y - bgHalfSize.y };
		ImVec2 bgMax = { centre.x + bgHalfSize.x, centre.y + bgHalfSize.y };

		drawList->AddImage
		(
			g_upTexOptionsBG.get(),
			bgMin,
			bgMax,
			{ 0, 0 },
			{ 1, uvMaxY },
			IM_COL32(255, 255, 255, 220)
		);
	}
	else
	{

		drawList->AddImage
		(
			g_upTexOptionsBG.get(),
			{ 0.0f, 0.0f },
			{ res.x, res.y },
			{ 0, 0 },
			{ 1, uvMaxY },
			IM_COL32(255, 255, 255, 255)
		);
	}

	// draw Options Logo

	if (scale >= 0.8f || isPause == false)
	{
		auto optionsUVs = PIXELS_TO_UV_COORDS(512, 512, 0, 0, 147, 448);
		auto optionsWidth = res.x * 0.16f;
		auto optionsHeight = res.y * 0.96f;

		ImVec2 optionsMin = { 0, min.y + Scale(10) };
		ImVec2 optionsMax = { optionsMin.x + optionsWidth, optionsMin.y + optionsHeight };

		drawList->AddImage(g_upTexBGOptions.get(), optionsMin, optionsMax, GET_UV_COORDS(optionsUVs), IM_COL32(255, 255, 255, 128));
	}
}

void DrawAnimatedFire(ImVec2 min, ImVec2 max)
{
	auto* drawList = ImGui::GetBackgroundDrawList();
	auto& res = ImGui::GetIO().DisplaySize;

	constexpr int frameCount = 4;  // columns 
	constexpr int rowCount = 4;   // rows 
	constexpr float frameWidth = 128.f / frameCount;
	constexpr float frameHeight = 128.f / rowCount;


	constexpr double fps = 24.f;
	int frame = (int)(ImGui::GetTime() * fps) % (frameCount * rowCount);

	// Convert flat frame index to row/col
	int col = frame % frameCount;
	int row = frame / frameCount;

	float x = 385 + col * frameWidth;
	float y = row * frameHeight;

	auto fireUVs = PIXELS_TO_UV_COORDS(512, 512, x, y, frameWidth, frameHeight);

	SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);


	auto containerWidth = res.x * 0.2f;
	float containerFinalX = res.x - frameWidth;
	auto containerHeight = res.y * 0.2f;

	// First we calculate the position of the flame of the menu BG
	float barTexX = 440.f;
	float barTexY = 340.f;
	float barTexH = 30.f;
	float texHeight = 512.0f;

	// Then we map to screen space proportionally
	float barScreenX = (barTexX / texHeight) * res.x;
	float barScreenY = (barTexY / texHeight) * res.y;
	float barScreenH = (barTexH / texHeight) * res.y;


	float barWidth = res.x - barScreenX;


	ImVec2 containerMin = {
		barScreenX + barWidth * 0.5f,  // centered within bar bounds
		barScreenY + barScreenH * 0.6f
	};


	ImVec2 containerMax = { barScreenX, barScreenH };


	drawList->AddImage(g_upTexBGOptions.get(), containerMin, containerMax, GET_UV_COORDS(fireUVs), IM_COL32(255, 255, 255, 255));

	SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);
}

void DrawMenuListContainer(ImVec2 min, ImVec2 max)
{
	auto* drawList = ImGui::GetBackgroundDrawList();
	auto& res = ImGui::GetIO().DisplaySize;

	auto containerUVs = PIXELS_TO_UV_COORDS(512, 512, 0, 247, 274, 264);
	auto containerWidth = res.x * 0.5f;
	auto containerHeight = res.y;
	auto containerOffsetX = Scale(1);
	auto containerOffsetY = Scale(136);

	// anim

	// 1.0 = fully on screen, 0.0 = fully off screen
	float t = OptionsMenu::s_state == OptionsMenuState::Closing ? (float)ComputeLinearMotion(g_containerClosedAnimTime, 0, 15, true) : (float)ComputeLinearMotion(g_containerAnimTime, 0, 15, false);


	float containerFinalX = res.x - containerWidth;

	// Start position is fully off screen to the right
	float containerStartX = res.x;

	// Interpolate between start and final
	float containerCurrentX = containerStartX + (containerFinalX - containerStartX) * t;

	ImVec2 containerMin = { containerCurrentX, min.y + containerOffsetY };
	ImVec2 containerMax = { containerCurrentX + containerWidth, containerMin.y + containerHeight };

	drawList->AddImage(g_upTexBGMenuList.get(), containerMin, containerMax, GET_UV_COORDS(containerUVs));

	if (t < 1.f)
		return;

	float optionsOffsetX = Scale(170);
	float optionsOffsetY = Scale(23);

	ImVec2 minOptions = { containerMin.x + optionsOffsetX, containerMin.y + optionsOffsetY };
	ImVec2 maxOptions = { res.x, containerMax.y - optionsOffsetY };

	optionsOffsetX -= Scale(170);
	//optionsOffsetY -= Scale(23);
	ImVec2 minClipRect = { containerMin.x + optionsOffsetX, containerMin.y + optionsOffsetY };
	ImVec2 maxClipRect = { res.x, containerMax.y - optionsOffsetY };
	DrawOptions(minClipRect, maxClipRect, minOptions, maxOptions);
	if (OptionsMenu::s_flowState == OptionsMenuFlowState::OptionCursor)
		DrawDescription(optionDescription); // we draw option description separately so they don't get discard by the clip rect.
	else if (OptionsMenu::s_flowState == OptionsMenuFlowState::OptionSelected)
	{
		DrawDescription(optionValDescription);
	}
}


static void DrawCategories(ImVec2 min, ImVec2 max)
{
	auto drawList = ImGui::GetBackgroundDrawList();


	auto categoryWidth = Scale(560);
	auto categoryHeight = Scale(47);

	if (g_categoryIndex == -1)
	{
		g_categoryIndex = 0;

	}

	for (size_t i = 0; i < (size_t)OptionsMenuCategory::Count; i++)
	{
		// Don't show debug category if locked.
		if (!OptionsMenu::s_isDebugUnlocked && (OptionsMenuCategory)i == OptionsMenuCategory::Debug)
			continue;

		if (OptionsMenu::s_isPause && (OptionsMenuCategory)i == OptionsMenuCategory::Misc) // do not allow legacy options during gameplay
			continue;

		auto isCurrent = i == g_categoryIndex;
		auto categoryOffsetY = Scale(48) * i;

		ImVec2 categoryMin = { min.x + Scale(42), min.y + Scale(147) + categoryOffsetY };
		ImVec2 categoryMax = { categoryMin.x + categoryWidth, categoryMin.y + categoryHeight };


		// drawList->AddImage(g_upTexMainMenu7.get(), categoryMin, categoryMax, GET_UV_COORDS(isCurrent ? selectedUVs : unselectedUVs));


		auto text = GetCategoryName((OptionsMenuCategory)i);
		auto textSize = g_fntSeurat->CalcTextSizeA(g_fntSeuratSize, FLT_MAX, 0.0f, text.c_str());

		/**SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);
		drawList->AddText(g_fntSeurat, g_fntSeuratSize, { categoryMin.x + Scale(129), categoryMin.y + Scale(6) }, isCurrent ? IM_COL32(234, 234, 69, 255) : IM_COL32(255, 255, 255, 255), text.c_str());
		SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);*/

		SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);


		DrawRubyAnnotatedText
		(
			g_fntSeurat,
			g_fntSeuratSize,
			max.x / 2,
			{ categoryMin.x + Scale(129), categoryMin.y + Scale(6) },
			5.f,
			text.c_str(),

			[=](const char* str, ImVec2 pos)
			{
				DrawTextWithShadow(g_fntSeurat, g_fntSeuratSize, pos, isCurrent ? IM_COL32(234, 234, 69, 255) : IM_COL32(255, 255, 255, 255), str, 1.f, 0.5f);
			},
			[=](const char* str, float size, ImVec2 pos)
			{
				// DrawTextWithShadow(g_fntSeurat, size, pos, IM_COL32(255, 255, 255, 255), str, 3.f, 0.5f);
			},

			true
			);

		SetShaderModifier(IMGUI_SHADER_MODIFIER_NONE);
	}

	if (OptionsMenu::s_flowState == OptionsMenuFlowState::CategoryCursor)
		DrawDescription(GetCategoryDescription((OptionsMenuCategory)g_categoryIndex));
}

void OptionsMenu::Draw()
{
	if (!s_isVisible)
		return;


	bool upIsHeld = false;
	bool downIsHeld = false;
	bool leftIsHeld = false;
	bool rightIsHeld = false;

	float t = (float)ComputeLinearMotion(g_containerAnimTime, 0, 15.f, false);
	float tClosing = (float)ComputeLinearMotion(g_containerClosedAnimTime, 0, 30.f, true);

	switch (s_state)
	{
	case OptionsMenuState::Transition:
	{
		GameMenuCommon::g_LegacyOptions = false;
		if (OptionsMenu::s_nextState == OptionsMenuState::Opening)
			s_optionsOpenTime = ImGui::GetTime();

		if (OptionsMenu::s_nextState == OptionsMenuState::Closing)
			s_optionsCloseTime = ImGui::GetTime();

		s_state = OptionsMenu::s_nextState;
	}
		break;

	case OptionsMenuState::Opening:
	{

		GameMenuCommon::g_LegacyOptions = false;

		if (t > 0.8f)
		{
			s_state = OptionsMenuState::Idle;
		}
	}
	break;

	case OptionsMenuState::Idle:
	{
		//if (s_commonMenu.IsOpen())
		{
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

				if (PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_B) || PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_Y))
					g_isDeclined = true;

				if (PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_Y))
					g_isLeaving = true;

				if (PJSPeripheral::IsJustPressed(0, PJSPeripheral::BTN_X))
					g_isReset = true;
			}
		}


		break;
	}

	case OptionsMenuState::Closing:
	{

		if (tClosing <= 0.2f)
		{
			if (GameMenuCommon::g_LegacyOptions == false)
			{
				if (s_isPause == false)
					GameMenuCommon::ResetNextMode();
			}
			else
			{
				GameMenuCommon::g_OptionsOpen = false;
			}

			s_isVisible = false;
		}
		break;
	}
	case OptionsMenuState::Restarting:
	{
		static bool s_restartPromptOpened = false;
		static bool s_restartFaderBegun = false;
		static int s_restartMessageResult = -1;

		auto message = Localise("Options_Message_Restart");
		std::array<std::string, 1> options = { Localise("Common_Ok") };

		if (!s_restartPromptOpened)
		{
			// Fade out description.
		   // s_commonMenu.SetDescription(" ");

			/**for (auto& def : g_configDefinitions)
			{
				if (def->RequiresRestart() && def->IsValueChanged())
					message += "\n- " + def->GetNameLocalised(Config::Language);
			}*/

			s_restartPromptOpened = true;
		}

		if (!s_restartFaderBegun && MessageWindow::Open(message, &s_restartMessageResult, options) == MSG_CLOSED)
		{
			if (s_restartMessageResult == 0)
			{
				Fader::FadeOut(1, []() { App::Restart({ "--quickboot" }); });
				s_restartFaderBegun = true;
			}
			else
			{
				//s_commonMenu.SetDescription(GetCategoryDescription((OptionsMenuCategory)g_categoryIndex));
				s_state = OptionsMenuState::Opening;
			}

			s_restartPromptOpened = false;
			s_restartMessageResult = -1;
		}

		break;
	}
	}

	switch (s_flowState)
	{
	case OptionsMenuFlowState::CategoryCursor:
	{
		auto categoryCount = (int)OptionsMenuCategory::Count;

		// Remove debug category from cursor select.
		if (!s_isDebugUnlocked)
			categoryCount -= 1;

		if (s_isPause)
			categoryCount -= 1;

		MoveCursor(g_categoryIndex, g_flowStateTime, 0, categoryCount, []()
			{
				ResetOptionSelection();

				g_categoryTime = ImGui::GetTime();
				g_cursorArrowsTime = g_categoryTime;
				// s_commonMenu.SetDescription(GetCategoryDescription((OptionsMenuCategory)g_categoryIndex));
			});

		if (CheckAndDiscard(g_isAccepted))
		{
			Game_PlaySound("decide");

			SetFlowState(OptionsMenuFlowState::OptionCursor);

			g_cursorArrowsTime = ImGui::GetTime();
			g_scrollArrowsTime = g_cursorArrowsTime;
		}

		if (CheckAndDiscard(g_isDeclined) || CheckAndDiscard(g_isLeaving) || OptionsMenu::s_GoToLegacyMenu == true)
		{
			if (OptionsMenu::s_GoToLegacyMenu == false)
				Game_PlaySound("cancel");

			g_stateTime = ImGui::GetTime();
			g_categoryTime = g_stateTime;

			s_isRestartRequired = IsRestartRequired();

			if (OptionsMenu::s_isRestartRequired)
			{
				s_state = OptionsMenuState::Restarting;
			}
			else
			{
		
				Close();
			}
		}

		OptionsMenu::s_GoToLegacyMenu = false;
		break;
	}

	case OptionsMenuFlowState::OptionCursor:
	{
		MoveCursor(g_optionIndex, g_flowStateTime, 0, g_optionCount, []()
			{
				g_cursorArrowsTime = ImGui::GetTime();
				g_scrollArrowsTime = g_cursorArrowsTime;
			});

		if (CheckAndDiscard(g_isDeclined))
		{
			Game_PlaySound("cancel");

			SetFlowState(OptionsMenuFlowState::CategoryCursor);

			g_cursorArrowsTime = ImGui::GetTime();

			// s_commonMenu.SetDescription(GetCategoryDescription((OptionsMenuCategory)g_categoryIndex));
		}

		break;
	}
	}

	DrawBackground(OptionsMenu::s_isPause);

	auto& res = ImGui::GetIO().DisplaySize;
	ImVec2 min = { g_horzCentre + g_aspectRatioNarrowMargin, g_vertCentre };
	ImVec2 max = { res.x - min.x, res.y - min.y };

	//	DrawAnimatedFire(min, max);

	if (t >= 0.8f)
	{
		DrawCategories(min, max);
		g_isControlsVisible = true;
	}

	DrawMenuListContainer(min, max);

	g_up = false;
	g_upWasHeld = upIsHeld;
	g_down = false;
	g_downWasHeld = downIsHeld;
	g_left = false;
	g_leftWasHeld = leftIsHeld;
	g_right = false;
	g_rightWasHeld = rightIsHeld;
	g_isAccepted = false;
	g_isDeclined = false;
	g_isReset = false;
	g_isLeaving = false;
}

void OptionsMenu::Open(bool isPause)
{

	OptionsMenu::s_isPause = isPause;
	OptionsMenu::s_nextState = OptionsMenuState::Opening;

	UpdateFontSize();
	g_stateTime = ImGui::GetTime();
	s_state = OptionsMenu::s_isPause ? OptionsMenuState::Transition : OptionsMenuState::Opening;
	g_containerAnimTime = ImGui::GetTime();

	s_isVisible = true;
	s_GoToLegacyMenu = false;
	// Update stored values for config definitions
	// to check if their values have changed later.
	for (auto& def : g_configDefinitions)
		def->UpdateStore();

	ResetSelection();

	std::array<Button, 3> buttons =
	{
		//Button("Common_Switch", 115.0f, EButtonIcon::LBRB, EButtonAlignment::Left, &g_isControlsVisible),
		Button("Common_Back", 115.0f, EButtonIcon::Y, EButtonAlignment::Left, &g_isControlsVisible),
		Button("Common_Reset", 100.0f, EButtonIcon::X, EButtonAlignment::Left, &g_optionCanReset),
		Button("Common_Select", 125.0f, EButtonIcon::A, EButtonAlignment::Right, &g_isControlsVisible),


	};

	ButtonGuide::Open(buttons);
	ButtonGuide::SetSideMargins(100);
	if (s_isPause)
	{
		GameMenuCommon::g_nextModeAddr = 0;
		GameMenuCommon::g_currentModeAddr = 0;
	}
}

bool OptionsMenu::IsRestartRequired()
{
	if (!s_isVisible)
		return false;

	for (auto& def : g_configDefinitions)
	{
		if (def->RequiresRestart() && def->IsValueChanged())
			return true;
	}

	return false;
}

void OptionsMenu::SetFlowState(OptionsMenuFlowState flowState)
{
	s_flowState = flowState;
	g_flowStateTime = ImGui::GetTime();
}


void OptionsMenu::Close()
{
	if (s_state == OptionsMenuState::Closing || s_state == OptionsMenuState::Transition)
		return;

	g_isControlsVisible = false;

	if (s_isPause)
	{

		GameMenuCommon::g_nextModeAddr = 0;
		GameMenuCommon::g_currentModeAddr = 0;
		RequestQuitOptionForPause();
	}

	if (GameMenuCommon::g_LegacyOptions == false)
	{
		GameMenuCommon::ResetCurrentMode();
	}
	else
	{
		GameMenuCommon::g_nextModeAddr = 0;
	}


	s_state = s_isPause ?  OptionsMenuState::Transition : OptionsMenuState::Closing;
	s_nextState = OptionsMenuState::Closing;
	g_stateTime = ImGui::GetTime();
	g_containerClosedAnimTime = ImGui::GetTime();
	g_cursorArrowsTime = g_stateTime;
	ButtonGuide::Close();

	Config::Save();
}

bool OptionsMenu::CanClose()
{
	return true; //to do
}

void OptionsMenu::Init()
{
	optionDescription = "";
	optionValDescription = "";
	g_upTexOptionsBG = LOAD_ZSTD_TEXTURE(g_options_bg);
	g_upTexBGMenuList = LOAD_ZSTD_TEXTURE(g_optionsIcons);
	g_upTexBGOptions = LOAD_ZSTD_TEXTURE(g_optionsText);

	g_fntSeurat = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-DB.otf");
}
