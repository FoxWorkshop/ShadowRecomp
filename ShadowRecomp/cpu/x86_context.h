#pragma once

inline thread_local X86Context* g_x86Context;

inline X86Context* GetX86Context()
{
    return g_x86Context;
}

inline void SetX86Context(X86Context& ctx)
{
    g_x86Context = &ctx;
}
