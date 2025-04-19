#pragma once

#include <kernel/kernel.h>

#define CURRENT_THREAD_HANDLE uint32_t(-2)

struct GuestThreadParams
{
    uint32_t function;
    uint32_t value;
    uint32_t flags;
    uint32_t tlsSize;
    uint32_t stack;
    uint32_t kernelStackSize;
};

struct GuestThreadContext
{
    X86Context x86Context{};
    uint8_t* thread = nullptr;

    GuestThreadContext(GuestThreadParams params);
    ~GuestThreadContext();
};



struct GuestThreadHandle : KernelObject
{
    GuestThreadParams params;
    std::atomic<bool> suspended;
    std::thread thread;

    GuestThreadHandle(const GuestThreadParams& params);
    ~GuestThreadHandle() override;

    uint32_t Wait(uint32_t timeout) override;
};

struct GuestThread
{
    static uint32_t Start(const GuestThreadParams& params);
    static GuestThreadHandle* Start(const GuestThreadParams& params, uint32_t* threadId);

    static uint32_t GetCurrentThreadId();
    //static void SetLastError(uint32_t error);

#ifdef _WIN32
    //static void SetThreadName(uint32_t threadId, const char* name);
#endif
};

#include "kernel/xbox_types.h"

extern TLS* m_TLS;