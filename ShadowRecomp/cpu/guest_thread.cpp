#include <stdafx.h>
#include <kernel/kernel.h>
#include "guest_thread.h"
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <kernel/function.h>
#include "x86_context.h"
#include <kernel/types.h>
#ifdef _WIN32
#include <windows.h>
#endif 
#include <intrin.h>



constexpr size_t TIB_SIZE = 0x38;
constexpr size_t NTTIB_SIZE = 0x1C;
constexpr size_t SELFPCR_SIZE = 0x4;
constexpr size_t PRCB_SIZE = 0x26C;  
constexpr size_t IRQL_SIZE = 0x1;
constexpr size_t PRCBDATA_SIZE = PRCB_SIZE;

constexpr size_t TLS_SIZE = 0x2000;//0x1000;
constexpr size_t STACK_SIZE = 0x10000;
constexpr size_t TOTAL_SIZE = 0x280; //TIB_SIZE + SELFPCR_SIZE + PRCB_SIZE + IRQL_SIZE + PRCBDATA_SIZE + TLS_SIZE + STACK_SIZE;

constexpr int8_t TLS_INDEX = -37; //hardcoded by the game on startup

uint8_t KPRCB_blob[PRCB_SIZE] = {};

bool isHostThread = true;

//from CXBX's code
xbox::dword_xt EmuGenerateStackSize(xbox::addr_xt& espBaseAddress, xbox::ulong_xt TlsDataSize)
{
	using namespace xbox;
	dword_xt StackSize = espBaseAddress & 15; //Ensure 16-byte alignment
	espBaseAddress -= StackSize;

	StackSize += 528; //FX_SAVE_AREA 

	TlsDataSize = ALIGN_UP(TlsDataSize, ulong_xt); 
	StackSize += TlsDataSize;

	StackSize += 24; //KSTART_FRAME and KSWITCHFRAME are 12 bytes each

	return StackSize;
}


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

typedef struct _KSWITCHFRAME_R
{
	uint32_t ExceptionList;
	uint32_t Unknown;
	uint32_t RetAddr;
} KSWITCHFRAME_R, * PKSWITCHFRAME_R;


typedef struct _KSTART_FRAME_R
{
	uint32_t SystemRoutine;
	uint32_t StartRoutine;
	uint32_t StartContext;
} KSTART_FRAME_R, * PKSTART_FRAME_R;

void KiInitializeContextThread(
	IN KTHREAD_R* Thread,
	IN xbox::ulong_xt TlsDataSize,
	IN uint32_t SystemRoutine,
	IN uint32_t StartRoutine,
	IN uint32_t StartContext
)
{
	auto StackAddress = static_cast<uint8_t*>(g_memory.Translate(Thread->StackBase));


	*(uint32_t*)(Thread + 0x30) = NPX_STATE_NOT_LOADED;

	/* Setup the Stack for TlsData dynamic sized array */
	//TlsDataSize = ALIGN_UP(TlsDataSize, uint32_t);

	/**StackAddress -= TlsDataSize; // TlsData section (optional)
	if (TlsDataSize) 
	{
		Thread->TlsData = g_memory.MapVirtual(StackAddress);
	}
	else {
		Thread->TlsData = 0;
	}*/

	/**StackAddress -= sizeof(KSTART_FRAME_R);
	KSTART_FRAME_R* StartFrame = reinterpret_cast<KSTART_FRAME_R*>(StackAddress);
	StackAddress -= sizeof(KSWITCHFRAME_R);
	PKSWITCHFRAME_R CtxSwitchFrame = reinterpret_cast<PKSWITCHFRAME_R>(StackAddress);

	/**StartFrame->StartContext = StartContext;
	StartFrame->StartRoutine = StartRoutine;
	StartFrame->SystemRoutine = SystemRoutine;

	CtxSwitchFrame->RetAddr = 0;
	CtxSwitchFrame->Unknown = 0x200; 
	CtxSwitchFrame->ExceptionList = reinterpret_cast<uint32_t>(X_EXCEPTION_CHAIN_END);

	/* Save back the new value of the kernel stack. 
	Thread->KernelStack = g_memory.MapVirtual(CtxSwitchFrame);*/
}

GuestThreadContext::GuestThreadContext(GuestThreadParams params)
{
	auto KernelStackSize = params.kernelStackSize;
	auto tlsSize = params.tlsSize;
	auto stack = params.stack; //already mapped

	assert(thread == nullptr);
	thread = (uint8_t*)g_userHeap.Alloc(TOTAL_SIZE);
	ETHREAD_R* eThread = (ETHREAD_R*)g_userHeap.Alloc(sizeof(ETHREAD_R));
	uint8_t* memory = (uint8_t*)g_userHeap.Alloc(STACK_SIZE + TLS_SIZE);
	uint8_t* stack_limit = memory + TLS_SIZE;
	uint8_t* stack_top = stack_limit + STACK_SIZE;
	uint8_t* tls_region = memory;

	memset(thread, 0, TOTAL_SIZE);
	memset(eThread, 0, sizeof(ETHREAD_R));

	// Set up memory regions
	uint8_t* NewPcr = thread;   //FS:[0x00] points here
	memset(NewPcr, 0, TOTAL_SIZE);
	uint8_t* XbTib = (NewPcr + 0x0);
	uint8_t* Prcb = NewPcr + 0x28; //FS:[0x28] = prcbData
	//uint8_t* unkData = prcb + 0x250;  //[prcb + 0x250] used by the game


	//uint8_t* hTib = XbTib;

	*(uint32_t*)(NewPcr + 0x01C) = g_memory.MapVirtual(NewPcr); //SelfPcr
	*(uint32_t*)(NewPcr + 0x018) = g_memory.MapVirtual(XbTib); //Self
	*(uint32_t*)(NewPcr + 0x020) = g_memory.MapVirtual(Prcb);  //fs:[0x20] = Prcb



	*(uint32_t*)(Prcb + 0x30) = FALSE;
	*(uint32_t*)(NewPcr + 0x24) = PASSIVE_LEVEL;

	uint32_t Host2XbStackBaseReserved = (uint32_t)g_memory.MapVirtual(g_userHeap.Alloc(KernelStackSize));
	uint32_t Host2XbStackSizeReserved = EmuGenerateStackSize(Host2XbStackBaseReserved, tlsSize);

	void* kenrelStackhost = nullptr;

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
		//auto host_stack = static_cast<uint8_t*>(g_memory.Translate(stack));
	//	auto host_stack_limit = host_stack - KernelStackSize;
		auto host_stack_limit = memory + TLS_SIZE;
		eThread->Tcb.StackLimit = g_memory.MapVirtual((const void*)(host_stack_limit));
		eThread->Tcb.KernelStack = g_memory.MapVirtual(stack_top);
		eThread->Tcb.TlsData = g_memory.MapVirtual(tls_region);
		KiInitializeContextThread(&eThread->Tcb, tlsSize, 0, 0, 0);

	}

	// Allocate space in the stack for TLS (+ pointer to data array)
	if (eThread->Tcb.TlsData) 
	{
		/**uint32_t RawTlsDataSize = m_TLS->dwDataEndAddr - m_TLS->dwDataStartAddr;
		uint32_t TlsSizeTotal = RawTlsDataSize + m_TLS->dwSizeofZeroFill;
		xbox::addr_xt* TlsData = reinterpret_cast<xbox::addr_xt*>(g_memory.Translate(eThread->Tcb.TlsData));
		*TlsData = reinterpret_cast<xbox::addr_xt>(eThread->Tcb.TlsData) + sizeof(xbox::addr_xt);

		TlsData += 1;
		std::memcpy(TlsData, reinterpret_cast<xbox::PVOID>(m_TLS->dwDataStartAddr), RawTlsDataSize);

		if (m_TLS->dwSizeofZeroFill) 
			std::memset(reinterpret_cast<xbox::PBYTE>(TlsData) + RawTlsDataSize, 0, m_TLS->dwSizeofZeroFill);*/

		uint32_t RawTlsDataSize = m_TLS->dwDataEndAddr - m_TLS->dwDataStartAddr;
		uint32_t TlsSizeTotal = RawTlsDataSize + m_TLS->dwSizeofZeroFill;


		/**std::memcpy(
			g_memory.Translate(eThread->Tcb.TlsData),
			reinterpret_cast<xbox::PVOID>(m_TLS->dwDataStartAddr),
			RawTlsDataSize
		);

		// Zero-fill remaining space if needed
		if (m_TLS->dwSizeofZeroFill) {
			std::memset(
				reinterpret_cast<xbox::PBYTE>(g_memory.Translate(eThread->Tcb.TlsData)) + RawTlsDataSize,
				0,
				m_TLS->dwSizeofZeroFill
			);
		}*/
	}



	if (isHostThread == false)
	{
		printf("StackBase: 0x%08X, StackLimit: 0x%08X, TlsData: 0x%08X\n",
			eThread->Tcb.StackBase, eThread->Tcb.StackLimit, eThread->Tcb.TlsData);
	}

	*(uint32_t*)(Prcb + 0x04B) = GuestThread::GetCurrentThreadId(); //maybe

	memset(&x86Context, 0, sizeof(x86Context));

	// Segment registers (Xbox-specific values)
 
	// Stack setup (Xbox uses downward-growing stack)

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
	x86Context.stack_ptr = g_memory.MapVirtual(thread + 0x4); //unsued for now

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
	auto id_ = *reinterpret_cast<const uint32_t*>(&id);
	return id_;
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
	/**auto ctx = GetX86Context();
	if (ctx)
	{
		return *(uint32_t*)(ctx->teb_ptr + 0x20);
	}

	return 0;*/

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
	//call function, if it crashes there the function was most likely not found in the map array
	g_memory.FindFunction(params.function)(ctx.x86Context, g_memory.base);
	return ctx.x86Context.eax.u32;
}

uint32_t GetCurrentThreadId(X86Context& ctx, uint8_t* base)
{
	return GetThreadId(std::this_thread::get_id());
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
//GUEST_FUNCTION_STUB(sub_37F6FC); //SEH prolog shit
//GUEST_FUNCTION_STUB(sub_37F737); //SEH ending s

GUEST_FUNCTION_HOOK(sub_37FDA0, memcpy);
GUEST_FUNCTION_HOOK(sub_380F60, memset);
GUEST_FUNCTION_HOOK(sub_380720, strncat);
GUEST_FUNCTION_HOOK(sub_37E820, memmove);
