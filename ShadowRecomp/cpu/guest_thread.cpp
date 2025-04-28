#include <stdafx.h>
#include <kernel/kernel.h>
#include "guest_thread.h"
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <kernel/function.h>
#include "x86_context.h"
#include <kernel/types.h>
#include <cstring>
#ifdef _WIN32
#include <windows.h>
#endif 
#include <intrin.h>

struct KTHREAD_R
{
	/* 0x0/0 */ uint8_t header[16];
	/* 0x10/16 */ int8_t MutantListHead[8];
	/* 0x18/24 */ unsigned long KernelTime;
	/* 0x1C/28 */ uint32_t StackBase;
	/* 0x20/32 */ uint32_t StackLimit;
	/* 0x24/36 */ uint32_t KernelStack;
	/* 0x28/40 */ uint32_t TlsData;
	uint8_t padding[0xE4];
};

typedef struct _ETHREAD_R
{
	struct KTHREAD_R    Tcb;
	uint8_t padding[0x30];
}
ETHREAD_R;
static_assert(sizeof(ETHREAD_R) == 0x140);

constexpr size_t KPCR_SIZE = 0x280;
constexpr size_t ETHREAD_R_SIZE = sizeof(_ETHREAD_R);

constexpr size_t TLS_SIZE = 0x2000;//0x1000;
constexpr size_t STACK_SIZE = 0x10000;
size_t TOTAL_SIZE = KPCR_SIZE + TLS_SIZE + STACK_SIZE + ETHREAD_R_SIZE;

bool isHostThread = true;

void KiInitializeContextThread(
	IN KTHREAD_R* Thread,
	IN xbox::ulong_xt TlsDataSize,
	IN uint32_t SystemRoutine,
	IN uint32_t StartRoutine,
	IN uint32_t StartContext
)
{
	*(uint32_t*)(Thread + 0x30) = NPX_STATE_NOT_LOADED;
}

GuestThreadContext::GuestThreadContext(GuestThreadParams params)
{
	auto KernelStackSize = params.kernelStackSize;
	auto tlsSize = params.tlsSize;

	if (tlsSize <= 0)
		tlsSize = TLS_SIZE;

	if (KernelStackSize <= 0)
		KernelStackSize = STACK_SIZE;

	TOTAL_SIZE = KPCR_SIZE + tlsSize + KernelStackSize + ETHREAD_R_SIZE;

	assert(thread == nullptr);
	thread = (uint8_t*)g_userHeap.Alloc(TOTAL_SIZE);
	//ETHREAD_R* eThread = (ETHREAD_R*)g_userHeap.Alloc(sizeof(ETHREAD_R));
	//uint8_t* memory = (uint8_t*)g_userHeap.Alloc(KernelStackSize + tlsSize);	
	

	ETHREAD_R* eThread = (ETHREAD_R*)thread + KPCR_SIZE + tlsSize + KernelStackSize;

	//uint8_t* memory = thread + KernelStackSize + tlsSize;

	uint8_t* stack_limit = thread + KPCR_SIZE + tlsSize;
	uint8_t* stack_top = stack_limit + KernelStackSize;
	uint8_t* tls_region = thread + KPCR_SIZE;

	memset(thread, 0, TOTAL_SIZE);
	memset(eThread, 0, sizeof(ETHREAD_R));

	// Set up memory regions
	uint8_t* NewPcr = thread;   //FS:[0x00] points here
	memset(NewPcr, 0, TOTAL_SIZE);
	uint8_t* XbTib = (NewPcr + 0x0);

	uint8_t* Prcb = NewPcr + 0x28; //FS:[0x28] = prcbData

	*(uint32_t*)(NewPcr + 0x04) = g_memory.MapVirtual(thread + 0x04);
	*(uint32_t*)(NewPcr + 0x01C) = g_memory.MapVirtual(NewPcr); //SelfPcr
	*(uint32_t*)(NewPcr + 0x018) = g_memory.MapVirtual(XbTib); //Self
	*(uint32_t*)(NewPcr + 0x020) = g_memory.MapVirtual(Prcb);  //fs:[0x20] = Prcb

	*(uint32_t*)(Prcb + 0x30) = FALSE;
	*(uint32_t*)(NewPcr + 0x24) = PASSIVE_LEVEL;


	if (isHostThread)
	{
		/**xbox::ulong_xt KernelStackSize_ = Host2XbStackBaseReserved - reinterpret_cast<xbox::ulong_xt>(*(uint32_t*)(hTib + 12));
		auto guestAddr = g_memory.MapVirtual((const void*)(stack));
		eThread->Tcb.StackBase = static_cast<uint32_t>(guestAddr);

		eThread->Tcb.StackLimit = g_memory.MapVirtual((uint32_t*)(stack - KernelStackSize_));
		KiInitializeContextThread(&eThread->Tcb, tlsSize, 0, 0, 0);*/

	}
	else
	{
		eThread->Tcb.StackBase = g_memory.MapVirtual(stack_top); // stack;
		eThread->Tcb.StackLimit = g_memory.MapVirtual(stack_limit);
		eThread->Tcb.KernelStack = eThread->Tcb.StackBase; // g_memory.MapVirtual(stack_top);
		eThread->Tcb.TlsData = g_memory.MapVirtual(tls_region);
		KiInitializeContextThread(&eThread->Tcb, tlsSize, 0, 0, 0);

	}

	// Allocate space in the stack for TLS, todo check if needed
	if (eThread->Tcb.TlsData) 
	{
		uint32_t RawTlsDataSize = m_TLS->dwDataEndAddr - m_TLS->dwDataStartAddr;
		uint32_t TlsSizeTotal = RawTlsDataSize + m_TLS->dwSizeofZeroFill;
	}


	if (isHostThread == false)
	{
		printf("StackBase: 0x%08X, StackLimit: 0x%08X, TlsData: 0x%08X\n",
			eThread->Tcb.StackBase, eThread->Tcb.StackLimit, eThread->Tcb.TlsData);
	}

	*(uint32_t*)(Prcb + 0x04B) = GuestThread::GetCurrentThreadId(); //maybe

	memset(&x86Context, 0, sizeof(x86Context));


	if (isHostThread)
	{
		//x86Context.esp.u32 = Thread->KernelStack;
		x86Context.ebp.u32 = 0;
	}
	else
	{
		x86Context.esp.u32 = eThread->Tcb.StackBase & ~0xF;
		x86Context.ebp.u32 = x86Context.esp.u32;
	}

	// Default flags (Interrupts enabled)
	x86Context.eflags.value = 0x202; // IF=1, reserved bit 1=1

	// Floating point control
	x86Context.fpu.cw = 0x037F;
	x86Context.mxcsr = 0x1F80;
	x86Context.kpcr_ptr = g_memory.MapVirtual(thread);

	x86Context.stack_ptr = *(uint32_t*)(NewPcr + 0x04); //unused for now

	assert(GetX86Context() == nullptr);
	SetX86Context(x86Context);

	isHostThread = false;
}

GuestThreadContext::~GuestThreadContext()
{
	g_userHeap.Free(thread);
}

static void GuestThreadFunc(GuestThreadHandle* hThread)
{
	//printf("GuestThreadFunc(GuestThreadHandle* hThread): Function %08X\n", hThread->params.function);
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
	//printf("GuestThread::Start(const GuestThreadParams& params, uint32_t* threadId), function: %08X\n", params.function);
	auto hThread = CreateKernelObject<GuestThreadHandle>(params);

	if (threadId != nullptr)
		*threadId = GetThreadId(hThread->thread.get_id());

	return hThread;
}


uint32_t GuestThread::GetCurrentThreadId()
{
	return GetThreadId(std::this_thread::get_id());
}

uint32_t GuestThread::Start(const GuestThreadParams& params)
{
	printf("GuestThread::Start(const GuestThreadParams& params), function: %08X\n", params.function);
	const auto procMask = (uint8_t)(params.flags >> 24);
	GuestThreadContext ctx(params);

	//ctx.x86Context.esp.u32 = (ctx.x86Context.esp.u32 - 16) & ~0xF;
		//ctx.x86Context.ecx.u32 = params.value;
	ctx.x86Context.esp.u32 -= 4; //in x86, the CPU always adjust the stack pointer by 4 bytes before calling a function
	g_memory.FindFunction(params.function)(ctx.x86Context, g_memory.base); 	//call function, if it crashes there the function was most likely not found in the map array
	return ctx.x86Context.eax.u32;
}


// Function to clean the stack after a function call
X86_FUNC(StubWithCleanStack)
{
	ctx.esp.u32 += 0x4; // Adjust ESP for the return address 
}

int XAPILIBGetCurrentThreadId()
{
	return GuestThread::GetCurrentThreadId();
}

GUEST_FUNCTION_HOOK(sub_2ADF79, XAPILIBGetCurrentThreadId);

GUEST_FUNCTION_STUB(sub_2B126A); // Some kernel hardware check, not needed on PC.
//GUEST_FUNCTION_STUB(sub_381F6C);

//GUEST_FUNCTION_STUB(sub_2B1815); 


