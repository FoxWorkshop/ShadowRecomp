#include <stdafx.h>
#include <kernel/kernel.h>
#include "guest_thread.h"
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <kernel/function.h>
#include "x86_context.h"
#include <kernel/types.h>


constexpr size_t PCR_SIZE = 0x1CC0;  // KPCR size
constexpr size_t TLS_SIZE = 0x1000;  // TLS slots
constexpr size_t TEB_SIZE = 0x1000;  // Full TEB
constexpr size_t STACK_SIZE = 0x10000; // 64KB stack
constexpr size_t TOTAL_SIZE = PCR_SIZE + TLS_SIZE + TEB_SIZE + STACK_SIZE;

static std::atomic<uint32_t> next_thread_id;

GuestThreadContext::GuestThreadContext(uint32_t cpuNumber)
{
	assert(thread == nullptr);
	thread = (uint8_t*)g_userHeap.Alloc(TOTAL_SIZE);
	memset(thread, 0, TOTAL_SIZE);


	// Set up memory regions
	uint8_t* pcr = thread;
	uint8_t* tls = thread + PCR_SIZE;
	uint8_t* teb = thread + PCR_SIZE + TLS_SIZE;
	uint8_t* stack = thread + PCR_SIZE + TLS_SIZE + TEB_SIZE;


	// Initialize TEB (Thread Environment Block)
	* (uint32_t*)(teb + 0x000) = 0;              // ExceptionList
	*(uint32_t*)(teb + 0x004) = 0;              // StackBase
	*(uint32_t*)(teb + 0x008) = g_memory.MapVirtual(stack + STACK_SIZE);
	*(uint32_t*)(teb + 0x00C) = 0;              // SubSystemTib
	*(uint32_t*)(teb + 0x018) = g_memory.MapVirtual(tls);
	*(uint32_t*)(teb + 0x020) = ++next_thread_id; // Thread ID (Xbox-specific offset)
	*(uint32_t*)(teb + 0x10C) = cpuNumber;       // CPU Affinity (Xbox-specific)


	memset(&x86Context, 0, sizeof(x86Context));

	// Segment registers (Xbox-specific values)
	x86Context.fs = 0x30;       // FS selector (Xbox kernel uses this)
	x86Context.fs_base = g_memory.MapVirtual(teb); // TEB linear address
	x86Context.teb_ptr = teb;    // Direct host pointer to TEB

	// Stack setup (Xbox uses downward-growing stack)
	x86Context.esp.u32 = g_memory.MapVirtual(stack + STACK_SIZE);
	x86Context.ebp.u32 = x86Context.esp.u32;

	// Default flags (Interrupts enabled)
	x86Context.eflags.value = 0x202; // IF=1, reserved bit 1=1

	// Floating point control
	x86Context.fpu.cw = 0x037F;
	x86Context.mxcsr = 0x1F80;


	assert(GetX86Context() == nullptr);
	SetX86Context(x86Context);
}

GuestThreadContext::~GuestThreadContext()
{
	g_userHeap.Free(thread);
}

static void GuestThreadFunc(GuestThreadHandle* hThread)
{
	printf("GuestThreadFunc(GuestThreadHandle* hThread): Function %08X\n", hThread->params.function);	
	hThread->suspended.wait(true);
	GuestThread::Start(hThread->params);
}


GuestThreadHandle::GuestThreadHandle(const GuestThreadParams& params)
	: params(params), suspended((params.flags & 0x1) != 0), thread(GuestThreadFunc, this)
{
	printf("GuestThreadHandle(const GuestThreadParams& params): Function %08X\n", params.function);
}

GuestThreadHandle::~GuestThreadHandle()
{
	printf("Destructor: Thread %08X\n", params.function);

	if (thread.joinable())
		thread.join();
}

uint32_t GuestThreadHandle::Wait(uint32_t timeout)
{
	printf("Wait: Thread %08X\n", params.function);
	assert(timeout == INFINITE);

	if (thread.joinable())
		thread.join();

	return STATUS_WAIT_0;
}

static uint32_t GetThreadId(const std::thread::id& id)
{
	return *reinterpret_cast<const uint32_t*>(&id);
}

GuestThreadHandle* GuestThread::Start(const GuestThreadParams& params, uint32_t* threadId)
{
	printf("GuestThread::Start(const GuestThreadParams& params, uint32_t* threadId), function: %08X\n", params.function);
	auto hThread = CreateKernelObject<GuestThreadHandle>(params);

	if (threadId != nullptr)
		*threadId = GetThreadId(hThread->thread.get_id());

	return hThread;
}


uint32_t GuestThread::GetCurrentThreadId()
{
	auto ctx = GetX86Context();
	if (ctx)
	{
		return *(uint32_t*)(ctx->teb_ptr + 0x20);
	}

	return 0;
}

uint32_t GuestThread::Start(const GuestThreadParams& params)
{
	printf("GuestThread::Start(const GuestThreadParams& params), function: %08X\n", params.function);
	const auto procMask = (uint8_t)(params.flags >> 24);
	const auto cpuNumber = procMask == 0 ? 0 : 7 - std::countl_zero(procMask);
	GuestThreadContext ctx(cpuNumber);

	ctx.x86Context.esp.u32 = (ctx.x86Context.esp.u32 - 16) & ~0xF;
	ctx.x86Context.esp.u32 -= 4;

	ctx.x86Context.ecx.u32 = params.value;

	//call function, if it crashes at 0x00000 then the function was most likely not found in the map array
	g_memory.FindFunction(params.function)(ctx.x86Context, g_memory.base);

	return ctx.x86Context.eax.u32;
}

uint32_t GetCurrentThreadId(X86Context& ctx, uint8_t* base)
{
	return GetThreadId(std::this_thread::get_id());
}

GUEST_FUNCTION_STUB(sub_2B126A); // Some kernel hardware check, not needed on PC.
GUEST_FUNCTION_STUB(sub_37F6FC); //SEH prolog shit
GUEST_FUNCTION_STUB(sub_37F737); //SEH ending shit