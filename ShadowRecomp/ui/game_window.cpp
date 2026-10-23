#include "game_window.h"
#include <gpu/video.h>

#include <app.h>
#include <sdl_listener.h>
#include <SDL_syswm.h>

#include <cmath>

#if _WIN32
#include <dwmapi.h>
#include <shellscalingapi.h>
#endif
#include <os/logger.h>
#include "imgui_utils.h"

//Original code from Unleashed Recomp

bool m_isFullscreenKeyReleased = true;
bool m_isResizing = false;

// SDL dispatches event watches on whichever thread pumps or pushes, this ends not being thread safe.
// We need to queue events and use mutex to ensure thread safety.
static std::mutex s_imguiEventMutex;
static std::vector<SDL_Event> s_imguiEvents;


static constexpr size_t IMGUI_EVENT_QUEUE_LIMIT = 4096; // arbitrary limits


// ensure that we only bounds events that can potentially crash
// like if DrawImGui stops running (loading screen, stalled swap chain) while input continue to be processed
static bool IsImGuiBackendEvent(const SDL_Event& event)
{
    switch (event.type)
    {
    case SDL_MOUSEMOTION:
    case SDL_MOUSEWHEEL:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_TEXTINPUT:
    case SDL_KEYDOWN:
    case SDL_KEYUP:
    case SDL_WINDOWEVENT:
    case SDL_CONTROLLERDEVICEADDED:
    case SDL_CONTROLLERDEVICEREMOVED:
        return true;
    default:
        return false;
    }
}

void GameWindow::DrainImGuiEvents()
{
    std::vector<SDL_Event> eventsToProcess;
    {
        std::lock_guard<std::mutex> lock(s_imguiEventMutex);
        eventsToProcess.swap(s_imguiEvents);
    }

    if (ImGui::GetCurrentContext() == nullptr || ImGui::GetIO().BackendPlatformUserData == nullptr)
        return;

    for (auto& event : eventsToProcess)
    {
        ImGui_ImplSDL2_ProcessEvent(&event);
    }
}

int Window_OnSDLEvent(void*, SDL_Event* event)
{
    // Queue ImGui backend events
    if (IsImGuiBackendEvent(*event))
    {
        std::lock_guard<std::mutex> lock(s_imguiEventMutex);

        if (s_imguiEvents.size() < IMGUI_EVENT_QUEUE_LIMIT)
            s_imguiEvents.push_back(*event);
    }

    if (ImGui::GetIO().BackendPlatformUserData != nullptr)
        ImGui_ImplSDL2_ProcessEvent(event);

    for (auto listener : GetEventListeners())
    {
        if (listener->OnSDLEvent(event))
        {
            return 0;
        }
    }

    switch (event->type)
    {
    case SDL_QUIT:
    {
        if (App::s_isSaving)
            break;

        App::Exit();
    }
    break;
    case SDL_KEYDOWN:
    {
        switch (event->key.keysym.sym)
        {
            // Toggle fullscreen on ALT+ENTER.
        case SDLK_RETURN:
        {
            if (!(event->key.keysym.mod & KMOD_ALT) || !m_isFullscreenKeyReleased)
                break;

            Config::Fullscreen = GameWindow::SetFullscreen(!GameWindow::IsFullscreen());

            if (Config::Fullscreen)
            {
                if (GameWindow::IsAllDisplays(Config::Monitor))
                    GameWindow::SetDisplay(Config::Monitor);
                else
                    Config::Monitor = GameWindow::GetDisplay();
            }
            else
            {
                Config::WindowState = GameWindow::SetMaximised(Config::WindowState == EWindowState::Maximised);
            }

            // Block holding ALT+ENTER spamming window changes.
            m_isFullscreenKeyReleased = false;
        }
        break;
        }
    }
    break;
    case SDL_KEYUP:
    {
        switch (event->key.keysym.sym)
        {
            // Allow user to input ALT+ENTER again.
        case SDLK_RETURN:
            m_isFullscreenKeyReleased = true;
            break;
        }
    }
    break;
    case SDL_WINDOWEVENT:
    {
        switch (event->window.event)
        {
        case SDL_WINDOWEVENT_FOCUS_LOST:
            GameWindow::s_isFocused = false;
            SDL_ShowCursor(SDL_ENABLE);
            break;

        case SDL_WINDOWEVENT_FOCUS_GAINED:
        {
            GameWindow::s_isFocused = true;

            if (GameWindow::IsFullscreen())
                SDL_ShowCursor(GameWindow::s_isFullscreenCursorVisible ? SDL_ENABLE : SDL_DISABLE);

            break;
        }

        case SDL_WINDOWEVENT_RESTORED:
            Config::WindowState = EWindowState::Normal;
            break;

        case SDL_WINDOWEVENT_MAXIMIZED:
            Config::WindowState = EWindowState::Maximised;
            break;

        case SDL_WINDOWEVENT_RESIZED:
            m_isResizing = true;

            if (Config::WindowSize >= 0 &&
                (event->window.data1 != GameWindow::s_width ||
                    event->window.data2 != GameWindow::s_height))
            {
                Config::WindowSize = -1;
            }

            GameWindow::s_width = event->window.data1;
            GameWindow::s_height = event->window.data2;
            GameWindow::SetTitle(fmt::format("{} - [{}x{}]", GameWindow::GetTitle(), GameWindow::s_width, GameWindow::s_height).c_str());
            break;

        case SDL_WINDOWEVENT_MOVED:
            GameWindow::s_x = event->window.data1;
            GameWindow::s_y = event->window.data2;
            break;
        }

        break;
    }

    }

    return 0;
}

void GameWindow::Init(const char* sdlVideoDriver)
{
#ifdef __linux__
    SDL_SetHint("SDL_APP_ID", "io.github.foxworkshop.shadowrecomp");
#endif

    if (SDL_VideoInit(sdlVideoDriver) != 0 && sdlVideoDriver)
    {
        //LOGN_ERROR("Failed to initialise the SDL video driver: \"{}\". Falling back to default.", sdlVideoDriver);
        SDL_VideoInit(nullptr);
    }

    auto videoDriverName = SDL_GetCurrentVideoDriver();

    if (videoDriverName)
        LOGFN("SDL video driver: \"{}\"", videoDriverName);

    SDL_EventState(SDL_SYSWMEVENT, SDL_ENABLE);
    SDL_AddEventWatch(Window_OnSDLEvent, s_pWindow);

#ifdef _WIN32
    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
#endif

    s_x = Config::WindowX;
    s_y = Config::WindowY;
    s_width = Config::WindowWidth.Value;
    s_height = Config::WindowHeight.Value;

    if (s_x == -1 && s_y == -1)
        s_x = s_y = SDL_WINDOWPOS_CENTERED;

    if (!IsPositionValid())
        GameWindow::ResetDimensions();

    s_pWindow = SDL_CreateWindow("Shadow Recompiled", s_x, s_y, s_width, s_height, GetWindowFlags());

    if (IsFullscreen())
        SDL_ShowCursor(SDL_DISABLE);

    SetDisplay(Config::Monitor);
    SetIcon((ELedAura)0);
    SetTitle();

    SDL_SetWindowMinimumSize(s_pWindow, MIN_WIDTH, MIN_HEIGHT);

    SDL_SysWMinfo info;
    SDL_VERSION(&info.version);
    SDL_GetWindowWMInfo(s_pWindow, &info);

#if defined(_WIN32)
    s_renderWindow = info.info.win.window;

    /**if (Config::DisableDWMRoundedCorners)
    {
        DWM_WINDOW_CORNER_PREFERENCE wcp = DWMWCP_DONOTROUND;
        DwmSetWindowAttribute(s_renderWindow, DWMWA_WINDOW_CORNER_PREFERENCE, &wcp, sizeof(wcp));
    }*/
#elif defined(PLUME_SDL_VULKAN_ENABLED)
    s_renderWindow = s_pWindow;
#elif defined(__linux__)
    s_renderWindow = { info.info.x11.display, info.info.x11.window };
#else
    static_assert(false, "Unknown platform.");
#endif

    SetTitleBarColour();

    SDL_ShowWindow(s_pWindow);
}

void GameWindow::Update()
{
    if (!GameWindow::IsFullscreen() && !GameWindow::IsMaximised() && !s_isChangingDisplay)
    {
        Config::WindowX = GameWindow::s_x;
        Config::WindowY = GameWindow::s_y;
        Config::WindowWidth = GameWindow::s_width;
        Config::WindowHeight = GameWindow::s_height;
    }

    if (m_isResizing)
    {
        SetTitle();
        g_pendingSurfaceResizeFlag = true;
        m_isResizing = false;
    }

    if (g_needsResize)
        s_isChangingDisplay = false;
}

SDL_Surface* GameWindow::GetIconSurface(void* pIconBmp, size_t iconSize)
{
    auto rw = SDL_RWFromMem(pIconBmp, iconSize);
    auto surface = SDL_LoadBMP_RW(rw, 1);

    //if (!surface)
      //  LOGF_ERROR("Failed to load icon: {}", SDL_GetError());

    return surface;
}

void GameWindow::SetIcon(void* pIconBmp, size_t iconSize)
{
    if (auto icon = GetIconSurface(pIconBmp, iconSize))
    {
        SDL_SetWindowIcon(s_pWindow, icon);
        SDL_FreeSurface(icon);
    }
}

void GameWindow::SetIcon(ELedAura mode)
{
    switch (mode)
    {
    default:
    case LED_AURA_NEUTRAL:
        //SetIcon(g_game_icon, sizeof(g_game_icon));
        break;
    case LED_AURA_HERO:
        //SetIcon(g_game_icon_hero, sizeof(g_game_icon_hero));
        break;
    case LED_AURA_VILLAIN:
        // SetIcon(g_game_icon_vilain, sizeof(g_game_icon_vilain));
        break;
    }
}

const char* GameWindow::GetTitle()
{
    if (Config::UseOfficialTitleOnTitleBar)
    {
        return "SHADOW THE HEDGEHOG(TM)";
    }

    return "Shadow Recompiled";
}

void GameWindow::SetTitle(const char* title)
{
    SDL_SetWindowTitle(s_pWindow, title ? title : GetTitle());
}

void GameWindow::SetTitleBarColour()
{
#if _WIN32
    /**if (os::user::IsDarkTheme())
    {
        auto version = os::version::GetOSVersion();

        if (version.Major < 10 || version.Build <= 17763)
            return;

        auto flag = version.Build >= 18985
            ? DWMWA_USE_IMMERSIVE_DARK_MODE
            : 19; // DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1

        const DWORD useImmersiveDarkMode = 1;
        DwmSetWindowAttribute(s_renderWindow, flag, &useImmersiveDarkMode, sizeof(useImmersiveDarkMode));
    }*/
#endif
}

bool GameWindow::IsFullscreen()
{
    return s_isAllScreensOption || (SDL_GetWindowFlags(s_pWindow) & SDL_WINDOW_FULLSCREEN_DESKTOP);
}

bool GameWindow::SetFullscreen(bool isEnabled)
{
    // undo borderless window before any fullscreen change
    if (s_isAllScreensOption)
    {
        s_isAllScreensOption = false;
        SDL_SetWindowBordered(s_pWindow, SDL_TRUE);
        SDL_SetWindowResizable(s_pWindow, SDL_TRUE);
    }

    if (isEnabled)
    {
        SDL_SetWindowFullscreen(s_pWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
        SDL_ShowCursor(s_isFullscreenCursorVisible ? SDL_ENABLE : SDL_DISABLE);
    }
    else
    {
        SDL_SetWindowFullscreen(s_pWindow, 0);
        SDL_ShowCursor(SDL_ENABLE);

        SetIcon((ELedAura)GameWindow::s_Icon);
        SetDimensions(Config::WindowWidth, Config::WindowHeight, Config::WindowX, Config::WindowY);
    }


    return isEnabled;
}

void GameWindow::SetFullscreenCursorVisibility(bool isVisible)
{
    s_isFullscreenCursorVisible = isVisible;

    if (IsFullscreen())
    {
        SDL_ShowCursor(s_isFullscreenCursorVisible ? SDL_ENABLE : SDL_DISABLE);
    }
    else
    {
        SDL_ShowCursor(SDL_ENABLE);
    }
}

bool GameWindow::IsMaximised()
{
    return SDL_GetWindowFlags(s_pWindow) & SDL_WINDOW_MAXIMIZED;
}

EWindowState GameWindow::SetMaximised(bool isEnabled)
{
    if (isEnabled)
    {
        SDL_MaximizeWindow(s_pWindow);
    }
    else
    {
        SDL_RestoreWindow(s_pWindow);
    }

    return isEnabled
        ? EWindowState::Maximised
        : EWindowState::Normal;
}

SDL_Rect GameWindow::GetDimensions()
{
    SDL_Rect rect{};

    SDL_GetWindowPosition(s_pWindow, &rect.x, &rect.y);
    SDL_GetWindowSize(s_pWindow, &rect.w, &rect.h);

    return rect;
}

void GameWindow::GetSizeInPixels(int* w, int* h)
{
    SDL_GetWindowSizeInPixels(s_pWindow, w, h);
}

void GameWindow::SetDimensions(int w, int h, int x, int y)
{
    s_width = w;
    s_height = h;
    s_x = x;
    s_y = y;

    SDL_SetWindowSize(s_pWindow, w, h);
    SDL_ResizeEvent(s_pWindow, w, h);

    SDL_SetWindowPosition(s_pWindow, x, y);
    SDL_MoveEvent(s_pWindow, x, y);
}

void GameWindow::ResetDimensions()
{
    s_x = SDL_WINDOWPOS_CENTERED;
    s_y = SDL_WINDOWPOS_CENTERED;
    s_width = DEFAULT_WIDTH;
    s_height = DEFAULT_HEIGHT;

    Config::WindowX = s_x;
    Config::WindowY = s_y;
    Config::WindowWidth = s_width;
    Config::WindowHeight = s_height;
}

uint32_t GameWindow::GetWindowFlags()
{
    uint32_t flags = SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI;

    if (Config::Fullscreen)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

#if defined(SDL_VULKAN_ENABLED) || defined(PLUME_SDL_VULKAN_ENABLED)
    flags |= SDL_WINDOW_VULKAN;
#endif

    return flags;
}

int GameWindow::GetDisplayCount()
{
    auto result = SDL_GetNumVideoDisplays();

    if (result < 0)
    {
        LOGF_ERROR("Failed to get display count: {}", SDL_GetError());
        return 1;
    }

    return result;
}

int GameWindow::GetDisplay()
{
    return SDL_GetWindowDisplayIndex(s_pWindow);
}

void GameWindow::SetDisplay(int displayIndex)
{
    if (!IsFullscreen())
        return;

    // SDL fullscreen is originally locked to one display, so use a borderless window
    // that cover every monitor instead for all screens options.
    if (IsAllDisplays(displayIndex))
    {
        if (s_isAllScreensOption)
            return;

        s_isChangingDisplay = true;

        auto bounds = GetAllDisplaysBounds();

        SDL_SetWindowFullscreen(s_pWindow, 0);
        SDL_SetWindowResizable(s_pWindow, SDL_FALSE);
        SDL_SetWindowBordered(s_pWindow, SDL_FALSE);
        SetDimensions(bounds.w, bounds.h, bounds.x, bounds.y);

        s_isAllScreensOption = true;
        SDL_ShowCursor(s_isFullscreenCursorVisible ? SDL_ENABLE : SDL_DISABLE);
        return;
    }

    if (displayIndex >= GetDisplayCount())
        displayIndex = 0;

    if (!s_isAllScreensOption && GetDisplay() == displayIndex)
        return;

    s_isChangingDisplay = true;

    SDL_Rect bounds;

    if (SDL_GetDisplayBounds(displayIndex, &bounds) == 0)
    {
        SetFullscreen(false);
        SetDimensions(bounds.w, bounds.h, bounds.x, bounds.y);
        SetFullscreen(true);
    }
    else
    {
        ResetDimensions();
    }
}

bool GameWindow::CanSpanDisplays()
{
    if (GetDisplayCount() < 2)
        return false;

    // likely won't work with wayland, so prevent it
    auto videoDriverName = SDL_GetCurrentVideoDriver();

    return !videoDriverName || strcmp(videoDriverName, "wayland") != 0;
}

// any index higher than the last monitor means all screens, so unplugging one still works
bool GameWindow::IsAllDisplays(int displayIndex)
{
    return displayIndex >= GetDisplayCount() && CanSpanDisplays();
}

SDL_Rect GameWindow::GetAllDisplaysBounds()
{
    SDL_Rect result{};
    auto displayCount = GetDisplayCount();

    for (int i = 0; i < displayCount; i++)
    {
        SDL_Rect bounds;

        if (SDL_GetDisplayBounds(i, &bounds) != 0)
            continue;

        if (result.w == 0 || result.h == 0)
            result = bounds;
        else
            SDL_UnionRect(&result, &bounds, &result);
    }

    return result;
}

float GameWindow::GetDisplayScale()
{
    if (s_pWindow == nullptr)
        return 1.0f;

    int w, h, wInPixels, hInPixels;
    SDL_GetWindowSize(s_pWindow, &w, &h);
    SDL_GetWindowSizeInPixels(s_pWindow, &wInPixels, &hInPixels);

    if (w <= 0 || h <= 0)
        return 1.0f;

    return (float(wInPixels) / float(w) + float(hInPixels) / float(h)) * 0.5f;
}

std::vector<SDL_DisplayMode> GameWindow::GetDisplayModes(bool ignoreInvalidModes, bool ignoreRefreshRates)
{
    auto result = std::vector<SDL_DisplayMode>();
    auto uniqueResolutions = std::set<std::pair<int, int>>();
    auto displayIndex = GetDisplay();
    auto modeCount = SDL_GetNumDisplayModes(displayIndex);
    auto scale = GetDisplayScale();

    if (modeCount <= 0)
        return result;

    for (int i = modeCount - 1; i >= 0; i--)
    {
        SDL_DisplayMode mode;

        if (SDL_GetDisplayMode(displayIndex, i, &mode) == 0)
        {
            if (ignoreInvalidModes)
            {
                if (mode.w < MIN_WIDTH || mode.h < MIN_HEIGHT)
                    continue;

                SDL_DisplayMode desktopMode;

                if (SDL_GetDesktopDisplayMode(displayIndex, &desktopMode) == 0)
                {
                    auto desktopW = (int)lround(desktopMode.w * scale);
                    auto desktopH = (int)lround(desktopMode.h * scale);

                    if (mode.w >= desktopW && mode.h >= desktopH)
                        continue;
                }
            }

            if (ignoreRefreshRates)
            {
                auto res = std::make_pair(mode.w, mode.h);

                if (uniqueResolutions.find(res) == uniqueResolutions.end())
                {
                    uniqueResolutions.insert(res);
                    result.push_back(mode);
                }
            }
            else
            {
                result.push_back(mode);
            }
        }
    }

    return result;
}

int GameWindow::FindNearestDisplayMode()
{
    auto result = -1;
    auto displayModes = GetDisplayModes();
    auto scale = GetDisplayScale();
    auto refWidth = (int)lround(s_width * scale);
    auto refHeight = (int)lround(s_height * scale);
    auto currentDiff = std::numeric_limits<int>::max();

    for (int i = 0; i < displayModes.size(); i++)
    {
        auto& mode = displayModes[i];

        auto widthDiff = abs(mode.w - refWidth);
        auto heightDiff = abs(mode.h - refHeight);
        auto totalDiff = widthDiff + heightDiff;

        if (totalDiff < currentDiff)
        {
            currentDiff = totalDiff;
            result = i;
        }
    }

    return result;
}

bool GameWindow::IsPositionValid()
{
    auto displayCount = GetDisplayCount();

    for (int i = 0; i < displayCount; i++)
    {
        SDL_Rect bounds;

        if (SDL_GetDisplayBounds(i, &bounds) == 0)
        {
            auto x = s_x;
            auto y = s_y;

            // Window spans across the entire display in windowed mode, which is invalid.
            if (!Config::Fullscreen && s_width == bounds.w && s_height == bounds.h)
                return false;

            if (x == SDL_WINDOWPOS_CENTERED_DISPLAY(i))
                x = bounds.w / 2 - s_width / 2;

            if (y == SDL_WINDOWPOS_CENTERED_DISPLAY(i))
                y = bounds.h / 2 - s_height / 2;

            if (x >= bounds.x && x < bounds.x + bounds.w &&
                y >= bounds.y && y < bounds.y + bounds.h)
            {
                return true;
            }
        }
    }

    return false;
}
