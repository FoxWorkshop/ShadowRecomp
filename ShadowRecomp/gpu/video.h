#pragma once

#define D3DCLEAR_TARGET  0x1
#define D3DCLEAR_ZBUFFER 0x10

inline bool g_needsResize;

struct Video
{
    static inline uint32_t s_viewportWidth;
    static inline uint32_t s_viewportHeight;

    static bool CreateHostDevice(const char* sdlVideoDriver, bool graphicsApiRetry);
};