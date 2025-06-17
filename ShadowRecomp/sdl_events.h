#pragma once

#include <SDL.h>
#include <ui/game_window.h>

enum SDL_USER_
{
    SDL_USER_HERO = SDL_USEREVENT + 1,
    SDL_USER_VILAIN
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

inline void SDL_User_Swap(SDL_USER_ mode)
{
    SDL_Event event{};

    switch (mode)
    {
    case SDL_USER_HERO:
        event.type = SDL_USER_HERO;
        break;
    case SDL_USER_VILAIN:
        event.type = SDL_USER_VILAIN;
        break;
    default:
        break;
    }

    event.user.code = mode;
    SDL_PushEvent(&event);
}