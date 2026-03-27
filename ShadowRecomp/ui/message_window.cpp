#include "message_window.h"
#include <locale/locale.h>
#include <gpu/imgui/imgui_snapshot.h>
#include <ui/imgui_utils.h>
#include <app.h>
#include <input/input.h>
#include <sdl_listener.h>


static bool g_isAwaitingResult = false;
static ImVec2 g_joypadAxis = {};
static bool g_isAccepted;
static bool g_isDeclined;
static bool g_isClosing = false;

static double g_appearTime;
static double g_controlsAppearTime;
static bool g_isControlsVisible = false;
static int g_selectedRowIndex;
static int g_foregroundCount;

static ImFont* g_fntShadow;

std::string g_text;
int g_result;
static std::vector<std::string> g_buttons;
int g_defaultButtonIndex;
int g_cancelButtonIndex;

static bool g_upWasHeld;
static bool g_downWasHeld;

class SDLEventListenerForMessageWindow : public SDLEventListener
{
public:
    bool OnSDLEvent(SDL_Event* event) override
    {
        if (App::s_isInit || !MessageWindow::s_isVisible || !IsInputAllowed())
            return false;

        constexpr float axisValueRange = 32767.0f;
        constexpr float axisTapRange = 0.5f;

        ImVec2 tapDirection = {};

        switch (event->type)
        {
        case SDL_KEYDOWN:
        {
            switch (event->key.keysym.scancode)
            {
            case SDL_SCANCODE_UP:
                g_joypadAxis.y = -1.0f;
                break;

            case SDL_SCANCODE_DOWN:
                g_joypadAxis.y = 1.0f;
                break;

            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_KP_ENTER:
                g_isAccepted = true;
                break;

            case SDL_SCANCODE_ESCAPE:
                g_isDeclined = true;
                break;
            }

            break;
        }

        case SDL_MOUSEBUTTONDOWN:
        {
            // Only accept left mouse button.
            if (event->button.button != SDL_BUTTON_LEFT)
                break;

            // Only accept mouse buttons when an item is selected.
            if (g_isControlsVisible && g_selectedRowIndex == -1)
                break;

            g_isAccepted = true;

            break;
        }

        case SDL_CONTROLLERBUTTONDOWN:
        {
            switch (event->cbutton.button)
            {
            case SDL_CONTROLLER_BUTTON_DPAD_UP:
                g_joypadAxis = { 0.0f, -1.0f };
                break;

            case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
                g_joypadAxis = { 0.0f, 1.0f };
                break;

            case SDL_CONTROLLER_BUTTON_A:
                g_isAccepted = true;
                break;

            case SDL_CONTROLLER_BUTTON_B:
                g_isDeclined = true;
                break;
            }

            break;
        }

        case SDL_CONTROLLERAXISMOTION:
        {
            if (event->caxis.axis < 2)
            {
                float newAxisValue = event->caxis.value / axisValueRange;
                bool sameDirection = (newAxisValue * g_joypadAxis[event->caxis.axis]) > 0.0f;
                bool wasInRange = abs(g_joypadAxis[event->caxis.axis]) > axisTapRange;
                bool isInRange = abs(newAxisValue) > axisTapRange;

                if (sameDirection && !wasInRange && isInRange)
                    tapDirection[event->caxis.axis] = newAxisValue;

                g_joypadAxis[event->caxis.axis] = newAxisValue;
            }

            break;
        }
        }

        return false;
    }
}
g_sdlEventListenerForMessageWindow;

void DrawGameStyleMessageBox(const char* text, ImVec2 center, ImVec2 size)
{


    ImDrawList* dl = ImGui::GetForegroundDrawList();

    ImVec2 min = { center.x - size.x * 0.5f, center.y - size.y * 0.5f };
    ImVec2 max = { center.x + size.x * 0.5f, center.y + size.y * 0.5f };

    // Background fill - dark blue
    dl->AddRectFilled(min, max, IM_COL32(1, 1, 70, 220));

    // Outer border
    dl->AddRect(min, max, IM_COL32(14, 14, 110, 255), 0.0f, 0, 2.0f);

    // Inner border (inset by ~4px)
    float inset = 4.0f;
    ImVec2 innerMin = { min.x + inset, min.y + inset };
    ImVec2 innerMax = { max.x - inset, max.y - inset };
    dl->AddRect(innerMin, innerMax, IM_COL32(30, 60, 180, 255), 0.0f, 0, 1.0f);

    // Corner accents - short lines in each corner
    float cornerLen = 10.0f;
    ImU32 cornerCol = IM_COL32(120, 160, 255, 255);

    // Top-left
    dl->AddLine(min, { min.x + cornerLen, min.y }, cornerCol, 2.0f);
    dl->AddLine(min, { min.x, min.y + cornerLen }, cornerCol, 2.0f);
    // Top-right
    dl->AddLine({ max.x, min.y }, { max.x - cornerLen, min.y }, cornerCol, 2.0f);
    dl->AddLine({ max.x, min.y }, { max.x, min.y + cornerLen }, cornerCol, 2.0f);
    // Bottom-left
    dl->AddLine({ min.x, max.y }, { min.x + cornerLen, max.y }, cornerCol, 2.0f);
    dl->AddLine({ min.x, max.y }, { min.x, max.y - cornerLen }, cornerCol, 2.0f);
    // Bottom-right
    dl->AddLine(max, { max.x - cornerLen, max.y }, cornerCol, 2.0f);
    dl->AddLine(max, { max.x, max.y - cornerLen }, cornerCol, 2.0f);

    auto maxWidth = Scale(820);
    auto fontSize = Scale(28);

    const auto input = RemoveRubyAnnotations(g_text.c_str());
    auto lines = Split(input.first.c_str(), g_fntShadow, fontSize, maxWidth);

    for (auto& line : lines)
    {
        line = ReAddRubyAnnotations(line, input.second);
    }

    // Text centered inside

    auto lineMargin = Config::Language != ELanguage::Japanese ? 5.0f : 5.5f;
    auto textSize = MeasureCentredParagraph(g_fntShadow, fontSize, lineMargin, lines);
    ImVec2 textPos = { center.x - textSize.x * 0.5f, center.y - textSize.y * 0.5f };
    auto textMarginX = Scale(37);
    auto textMarginY = Scale(45);

    auto textX = center.x;
    auto textY = center.y + Scale(3);

    if (Config::Language == ELanguage::Japanese)
    {
        textMarginX -= Scale(2.5f);
        textMarginY -= Scale(2.0f);

        textY += Scale(lines.size() % 2 == 0 ? 1.5f : 8.0f);
    }

    if (App::s_isInit)
        SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);

    max.x += 1.f;
    max.y += 1.f;
    dl->PushClipRect(min, max);

    DrawRubyAnnotatedText
    (
        g_fntShadow,
        fontSize,
        maxWidth,
        { textX, textY },
        lineMargin,
        g_text.c_str(),

        [=](const char* str, ImVec2 pos)
        {
            DrawTextWithShadow(g_fntShadow, fontSize, pos, IM_COL32(255, 255, 255, 255), str);
        },
        [=](const char* str, float size, ImVec2 pos)
        {
            DrawTextWithShadow(g_fntShadow, size, pos, IM_COL32(255, 255, 255, 255), str, 1.5f, 1.5f);
        },

        true
    );

    if (App::s_isInit)
        SetShaderModifier(IMGUI_SHADER_MODIFIER_LOW_QUALITY_TEXT);

    dl->PopClipRect();
}

static void ResetSelection()
{
    /* Always use -1 for mouse input to prevent the selection
       cursor from erroneously appearing where it shouldn't. */
    g_selectedRowIndex = input::g_inputDevice == input::EInputDevice::Mouse
        ? -1
        : g_defaultButtonIndex;

    g_upWasHeld = false;
    g_downWasHeld = false;
    g_joypadAxis = {};
    g_isAccepted = false;
    g_isDeclined = false;
}

bool MessageWindow::Open(std::string text, int* result, std::span<std::string> buttons, int defaultButtonIndex, int cancelButtonIndex)
{
    if (!g_isAwaitingResult && *result == -1)
    {
        s_isVisible = true;
        g_isClosing = false;
        g_isControlsVisible = false;
        g_foregroundCount = 0;
        g_appearTime = ImGui::GetTime();
        g_controlsAppearTime = ImGui::GetTime();

        g_text = text;
        g_buttons = std::vector(buttons.begin(), buttons.end());
        g_defaultButtonIndex = defaultButtonIndex;
        g_cancelButtonIndex = cancelButtonIndex;

        ResetSelection();

        //Game_PlaySound("sys_actstg_pausewinopen");

        g_isAwaitingResult = true;
    }

    *result = g_result;

    // Returns true when the message window is closed.
    return !g_isAwaitingResult;
}

void MessageWindow::Init()
{
    auto& io = ImGui::GetIO();

    g_fntShadow = io.Fonts->Fonts[0];
}

void MessageWindow::Draw()
{
    if (!s_isVisible)
        return;

    auto drawList = ImGui::GetBackgroundDrawList();
    auto& res = ImGui::GetIO().DisplaySize;

    ImVec2 centre = { res.x / 2, res.y / 2 };
    ImVec2 max = { res.x * 0.9f, res.y * 0.7f };
    DrawGameStyleMessageBox(g_text.c_str(), centre, max);
}


void MessageWindow::Close()
{
    if (!g_isClosing)
    {
        g_appearTime = ImGui::GetTime();
        g_controlsAppearTime = ImGui::GetTime();
        g_isClosing = true;
        g_isControlsVisible = false;
        g_foregroundCount = 0;
        g_isAwaitingResult = false;

        //ButtonGuide::Close();
    }

   // Game_PlaySound("sys_actstg_pausewinclose");
}
