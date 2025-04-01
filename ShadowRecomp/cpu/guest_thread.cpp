#include <stdafx.h>
#include "guest_thread.h"
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <kernel/function.h>
#include "x86_context.h"

// Updated sizes for original Xbox
constexpr size_t PCR_SIZE = 0x1CC0;  // KPCR size
constexpr size_t TLS_SIZE = 0x1000;  // TLS slots
constexpr size_t TEB_SIZE = 0x1000;  // Full TEB
constexpr size_t STACK_SIZE = 0x10000; // 64KB stack
constexpr size_t TOTAL_SIZE = PCR_SIZE + TLS_SIZE + TEB_SIZE + STACK_SIZE;

GuestThreadContext::GuestThreadContext(uint32_t cpuNumber)
{
    thread = (uint8_t*)g_userHeap.Alloc(PCR_SIZE + TLS_SIZE + TEB_SIZE + STACK_SIZE);

    // Set FS segment (x86-specific)
    x86Context.fs = 0x30;  // TEB selector in OG Xbox kernel
    *(uint32_t*)thread = g_memory.MapVirtual(thread + PCR_SIZE); // tls pointer

    // Stack setup (EBP/ESP)
    x86Context.esp.u32 = g_memory.MapVirtual(thread + PCR_SIZE + TLS_SIZE + TEB_SIZE + STACK_SIZE);
    x86Context.ebp.u32 = x86Context.esp.u32;

    // TLS pointer at FS:[0x2C]
    *(uint32_t*)(thread + PCR_SIZE + TLS_SIZE + 0x2C) = g_memory.MapVirtual(thread + PCR_SIZE);

    assert(GetX86Context() == nullptr);
    SetX86Context(x86Context);
}


GuestThreadContext::~GuestThreadContext()
{
    g_userHeap.Free(thread);
}

static void GuestThreadFunc(GuestThreadHandle* hThread)
{
    hThread->suspended.wait(true);
    GuestThread::Start(hThread->params);
}

GuestThreadHandle::GuestThreadHandle(const GuestThreadParams& params)
    : params(params), suspended((params.flags & 0x1) != 0), thread(GuestThreadFunc, this)
{
}


GuestThreadHandle* GuestThread::Start(const GuestThreadParams& params, uint32_t* threadId)
{
    return nullptr;
}


uint32_t GuestThread::Start(const GuestThreadParams& params)
{
    GuestThreadContext ctx(0); 
    ctx.x86Context.ecx.u32 = params.value;
    ctx.x86Context.esp.u32 -= 4;

    X86Func* targetFunc = g_memory.FindFunction(params.function);
    targetFunc(ctx.x86Context, g_memory.base);

    return ctx.x86Context.eax.u32;
}