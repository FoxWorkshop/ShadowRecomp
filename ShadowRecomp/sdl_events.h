#pragma once

#include <SDL.h>
#include <ui/game_window.h>
#include <app.h>

enum SDL_USER_
{
    SDL_USER_CHAOSMODE = SDL_USEREVENT + 1,
};

inline void SDL_ResizeEvent(SDL_Window* pWindow, int width, int height)
{
    SDL_Event event{};
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_RESIZED;
    event.window.windowID = SDL_GetWindowID(pWindow);
    event.window.data1 = width;
    event.window.data2 = height;

    SDL_PushEvent(&event);
}

inline void SDL_MoveEvent(SDL_Window* pWindow, int x, int y)
{
    SDL_Event event{};
    event.type = SDL_WINDOWEVENT;
    event.window.event = SDL_WINDOWEVENT_MOVED;
    event.window.windowID = SDL_GetWindowID(pWindow);
    event.window.data1 = x;
    event.window.data2 = y;

    SDL_PushEvent(&event);
}

inline void SDL_User_ChaosMode(ELedAura chaosMode)
{
    SDL_Event event{};
    event.type = SDL_USER_CHAOSMODE;

    if ((int)chaosMode < 0 || (int)chaosMode > 2)
		chaosMode = ELedAura::LED_AURA_NEUTRAL;

    event.user.code = (Sint32)chaosMode;
    SDL_PushEvent(&event);
}