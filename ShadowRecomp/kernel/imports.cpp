#include <stdafx.h>
#include <cpu/x86_context.h>
#include <cpu/guest_thread.h>
#include "io/file_system.h"
#include "function.h"

#include "xbox_types.h"
#include "xbox.h"
#include "types.h"
#include "heap.h"
#include "memory.h"
#include <memory>
#include "kernel.h"
#include <util.h>



static std::atomic<uint32_t> g_keSetEventGeneration;
constexpr uint32_t GPU_ALIGNMENT = 4096;

//IRQL levels
#define PASSIVE_LEVEL 0
#define DISPATCH_LEVEL 2
#define PROFILE_LEVEL 27
#define CLOCK_LEVEL 28
#define IPI_LEVEL 29
#define POWER_LEVEL 30
#define HIGH_LEVEL 31

template<typename TGuest>
struct HostObject
{
	typedef TGuest guest_type;
};

struct Event final : KernelObject, HostObject<XKEVENT>
{
	bool manualReset;
	std::atomic<bool> signaled;

	Event(XKEVENT* header)
		: manualReset(!header->Type), signaled(!!header->SignalState)
	{
	}

	Event(bool manualReset, bool initialState)
		: manualReset(manualReset), signaled(initialState)
	{
	}

	uint32_t Wait(uint32_t timeout) override
	{
		if (timeout == 0)
		{
			if (manualReset)
			{
				if (!signaled)
					return STATUS_TIMEOUT;
			}
			else
			{
				bool expected = true;
				if (!signaled.compare_exchange_strong(expected, false))
					return STATUS_TIMEOUT;
			}
		}
		else if (timeout == INFINITE)
		{
			if (manualReset)
			{
				signaled.wait(false);
			}
			else
			{
				while (true)
				{
					bool expected = true;
					if (signaled.compare_exchange_weak(expected, false))
						break;

					signaled.wait(expected);
				}
			}
		}
		else
		{
			assert(false && "Unhandled timeout value.");
		}

		return X_STATUS_SUCCESS;
	}

	bool Set()
	{
		signaled = true;

		if (manualReset)
			signaled.notify_all();
		else
			signaled.notify_one();

		return TRUE;
	}

	bool Reset()
	{
		signaled = false;
		return TRUE;
	}
};


void AvSendTVEncoderOption
(
	IN  PVOID   RegisterBase,
	IN  uint32_t   Option,
	IN  uint32_t   Param,
	OUT uint32_t* Result
)
{
	if (Option == 6)
	{

	}
}

uint32_t AvSetDisplayMode
(
	IN  PVOID   RegisterBase,
	IN  uint32_t   Step,
	IN  uint32_t   Mode,
	IN  uint32_t   Format,
	IN  uint32_t   Pitch,
	IN  uint32_t   FrameBuffer
)
{
	LOG_UTIL("!!! STUB !!!");
	return X_STATUS_SUCCESS;
}

void NtReadFile()
{
	LOG_UTIL("!!! STUB !!!");
}

void AvGetSavedDataAddress()
{
	LOG_UTIL("!!! STUB !!!");
}


int32_t NtFreeVirtualMemory(IN OUT uint32_t* BaseAddress, IN OUT ULONG32 FreeSize, IN uint32_t FreeType)
{
	printf("NtFreeVirtualMemory\n");
	//g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
	return X_STATUS_SUCCESS;
}

int32_t NtAllocateVirtualMemory(IN OUT uint32_t* BaseAddress,
	IN ulong_xt         ZeroBits,
	IN OUT uint32_t* AllocationSize,
	IN dword_xt         AllocationType,
	IN dword_xt         Protect)
{
	/**void* ptr = g_userHeap.Alloc(*AllocationSize);

	uint32_t xbox_addr = g_memory.MapVirtual(ptr);
	*BaseAddress = reinterpret_cast<uint32_t>(xbox_addr);

	printf("NtAllocateVirtualMemory: %zu bytes -> 0x%08X\n\n", *AllocationSize, xbox_addr);*/
	return X_STATUS_SUCCESS;
}

void* AvSavedDataAddress;
void AvSetSavedDataAddress(IN  uint32_t* Address)
{
	printf("AvSetSavedDataAddress %d\n", Address);
	AvSavedDataAddress = Address;
}

uint32_t DbgPrintV(PCHAR Format, va_list args) {
	vprintf(Format, args);
	return X_STATUS_SUCCESS;
}

uint32_t DbgPrint(PCHAR Format, ...) {
	va_list args;
	va_start(args, Format);
	uint32_t result = DbgPrintV(Format, args);
	va_end(args);
	return result;
}

void ExFreePool()
{
	LOG_UTIL("!!! STUB !!!");
}

void* ExAllocatePoolWithTag
(
	IN uint32_t NumberOfBytes,
	IN uint32_t Tag
)
{
	void* ptr = reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.Alloc(NumberOfBytes > 0 ? NumberOfBytes : 1)));
	printf("Allocated dummy ptr: %p\n", ptr);
	return ptr;
}

uint32_t ExQueryPoolBlockSize
(
	IN PVOID PoolBlock
)
{
	printf("[STUB] ExQueryPoolBlockSize(%p) -> Dummy 4096\n", PoolBlock);
	auto size = g_userHeap.Size(PoolBlock);
	return size;
}



LONG ExQueryNonVolatileSetting(
	IN  dword_xt  ValueIndex,
	OUT dword_xt* Type,
	OUT uint32_t* Value,
	IN  size_xt   ValueLength,
	OUT uint32_t* ResultLength OPTIONAL
)
{

	// Default to REG_DWORD (common for Xbox settings)
	if (Type)
		*Type = REG_DWORD;

	// Handle known settings
	switch (ValueIndex)
	{
	case 17:
		return X_STATUS_SUCCESS;
	case 0x1000: // Language (en-US)
		if (ValueLength >= 4)
		{
			*(DWORD*)Value = 0x0409; // English
			if (ResultLength)
				*ResultLength = 4;
			return X_STATUS_SUCCESS;
		}
		break;

	case 0x2002:
		if (ValueLength >= 4)
		{
			*(DWORD*)Value = 0; // 640x480
			if (ResultLength) *ResultLength = 4;
			return X_STATUS_SUCCESS;
		}
		break;

	case xbox::XC_FACTORY_GAME_REGION:
		if (ValueLength >= 4)
		{
			*(DWORD*)Value = 0x01;
			if (ResultLength) *ResultLength = 4;
			return X_STATUS_SUCCESS;
		}
		break;
	default:
		break;
	}

	// Fallback: Log unknown indices and return "not found"
	printf("ExQueryNonVolatileSetting: Unhandled index 0x%X\n", ValueIndex);
	return X_STATUS_OBJECT_NAME_NOT_FOUND;
}

LONG FscSetCacheSize(ULONG NumberOfCachePages)
{
	printf("[STUB] FscSetCacheSize, Page: %u)\n", NumberOfCachePages);
	return X_STATUS_SUCCESS;
}

uint32_t HalGetInterruptVector(uint32_t BusInterruptLevel, std::uint8_t* Irql)
{

	printf("[STUB] HalGetInterruptVector(IRQ %d, Pin %d)\n", BusInterruptLevel, Irql);
	return 0x20; //Return a dummy vector (typical for ISA IRQs)
}

//allows drivers or system components to register a callback function to be notified when the console is about to shut down.
void HalRegisterShutdownNotification(int32_t* ShutdownRegistration, uint8_t Register)
{
	printf("[STUB] HalRegisterShutdownNotification(%p, %d)\n", ShutdownRegistration, Register);
}

void HalReturnToFirmware(ULONG Action)
{
	printf("[STUB] HalReturnToFirmware(%d) - Would reboot\n", Action);
}

void HalReadWritePCISpace
(
	IN uint32_t   BusNumber,
	IN uint32_t   SlotNumber,
	IN uint32_t   RegisterNumber,
	IN uint32_t* Buffer,
	IN uint32_t   Length,
	IN std::uint8_t WritePCISpace
)
{
	printf("[STUB] %sPCISpace(Bus %d, Slot %d, Offset %X, Len %d)\n",
		WritePCISpace ? "Write" : "Read",
		BusNumber, SlotNumber, Buffer, Length);

	if (WritePCISpace)
		return;

	// For reads, zero the buffer
	memset(Buffer, 0, Length);
}

LONG IoCreateSymbolicLink(uint32_t* SymbolicLinkName_, uint32_t* DeviceName_)
{

	printf("[STUB] IoCreateSymbolicLink\n");

	return X_STATUS_SUCCESS;
}

LONG IoDeleteSymbolicLink(uint32_t* SymbolicLinkName_)
{
	xbox::PSTRING SymbolicLinkName = (xbox::PSTRING)SymbolicLinkName_;
	printf("[STUB] IoDeleteSymbolicLink(%.*S)\n",
		SymbolicLinkName->Length / 2, SymbolicLinkName->Buffer);

	return X_STATUS_SUCCESS;
}

int32_t KeBugCheckEx(IN std::uint32_t BugCheckCode, IN uint32_t* BugCheckParameter1, IN uint32_t* BugCheckParameter2, IN PVOID BugCheckParameter3, IN PVOID BugCheckParameter4)
{

	static bool KeBugCheckIgnored = false;

	if (KeBugCheckIgnored)
	{
		return S_OK;
	}

	char buffer[1024];
	sprintf(buffer, "The running software triggered KeBugCheck with the following information\n"
		"BugCheckCode: 0x%.8X\n"
		"BugCheckParameter1: 0x%p\n"
		"BugCheckParameter2: 0x%p\n"
		"BugCheckParameter3: 0x%p\n"
		"BugCheckParameter4: 0x%p\n"
		"\nThis is the Xbox equivalent to a BSOD and would cause the console to automatically reboot\n"
		"\nContinue Execution (Not Recommended)?\n",
		BugCheckCode, BugCheckParameter1, BugCheckParameter2, BugCheckParameter3, BugCheckParameter4);


	__builtin_debugtrap();
	KeBugCheckIgnored = true;
	return S_OK;
}


void KeBugCheck(IN uint32_t BugCheckMode)
{
	printf("KeBugCheckEx");
	KeBugCheckEx(BugCheckMode, 0, 0, 0, 0);
}

std::uint8_t KeRaiseIrqlToDpcLevel()
{
	printf("[STUB] KeRaiseIrqlToDpcLevel()\n");
	return DISPATCH_LEVEL;
}

void KeQuerySystemTime(uint64_t* CurrentTime_)
{
	constexpr int64_t FILETIME_EPOCH_DIFFERENCE = 116444736000000000LL;

	auto now = std::chrono::system_clock::now();
	auto timeSinceEpoch = now.time_since_epoch();

	int64_t currentTime100ns = std::chrono::duration_cast<std::chrono::duration<int64_t, std::ratio<1, 10000000>>>(timeSinceEpoch).count();
	currentTime100ns += FILETIME_EPOCH_DIFFERENCE;

	*CurrentTime_ = currentTime100ns;
}

LONG KeQueryBasePriorityThread(GuestThreadHandle* Thread)
{
	//xbox::PKTHREAD t = (xbox::PKTHREAD)Thread;
	return 8;
}

std::uint8_t KeInsertQueueDpc(uint32_t* Dpc_, uint32_t* Argument1, uint32_t* Argument2)
{
	xpointer<KDPC> Dpc = xpointer<KDPC>(*Dpc_);
	printf("[STUB] KeInsertQueueDpc(%p, %p, %p)\n", Dpc, Argument1, Argument2);
	return TRUE;
}

#define IRQ_BASE 0x30 
#define VECTOR2IRQ(vector)  ((vector)-IRQ_BASE)
void KeInitializeInterrupt(uint32_t* Interrupt_, uint32_t* ServiceRoutine_,
	uint32_t* ServiceContext, ULONG Vector, std::uint8_t Irql,
	uint32_t InterruptMode_, BOOLEAN ShareVector)
{
	/**xbox::KINTERRUPT_MODE InterruptMode = (xbox::KINTERRUPT_MODE)InterruptMode_;
	xbox::PKSERVICE_ROUTINE ServiceRoutine = (xbox::PKSERVICE_ROUTINE)ServiceRoutine_;
	xbox::PKINTERRUPT Interrupt = (xbox::PKINTERRUPT)Interrupt_;
	Interrupt->ServiceRoutine = ServiceRoutine;
	Interrupt->ServiceContext = ServiceContext;
	Interrupt->BusInterruptLevel = VECTOR2IRQ(Vector);
	Interrupt->Irql = Irql;
	Interrupt->Connected = FALSE;
	Interrupt->Mode = InterruptMode;*/

	printf("[STUB] KeInitializeInterrupt(Vector 0x%X, Irql %d)\n", Vector, Irql);
}

void KeDisconnectInterrupt(uint32_t* Interrupt_)
{
	xbox::PKINTERRUPT Interrupt = (xbox::PKINTERRUPT)Interrupt;
	printf("[STUB] KeDisconnectInterrupt(%p)\n", Interrupt);
}

uint32_t GuestTimeoutToMilliseconds(int64_t* timeout)
{
	return timeout ? (*timeout * -1) / 10000 : INFINITE;
}

LONG KeDelayExecutionThread(uint8_t WaitMode, BOOLEAN Alertable, int64_t* Interval)
{
	if (Alertable)
		return STATUS_USER_APC;

	uint32_t timeout = GuestTimeoutToMilliseconds(Interval);

#ifdef _WIN32
	Sleep(timeout);
#else
	if (timeout == 0)
		std::this_thread::yield();
	else
		std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
#endif

	return X_STATUS_SUCCESS;
}

std::uint8_t KeConnectInterrupt(uint32_t* Interrupt_)
{
	xbox::PKINTERRUPT Interrupt = (xbox::PKINTERRUPT)Interrupt;
	printf("[STUB] KeConnectInterrupt(%p)\n", Interrupt);
	return X_STATUS_SUCCESS;
}

std::uint8_t KeCancelTimer(uint32_t* Timer_)
{
	xbox::PKTIMER Timer = (xbox::PKTIMER)Timer_;
	printf("[STUB] KeCancelTimer(%p)\n", Timer);
	return TRUE;
}


VOID KeInitializeDpc(_KDPC* Dpc, uint32_t DeferredRoutine, uint32_t* DeferredContext)
{
	Dpc->Type = 19;
	Dpc->Inserted = FALSE;
	Dpc->DeferredRoutine = (PKDEFERRED_ROUTINE)DeferredRoutine;
	Dpc->DeferredContext = (xpointer<void>)DeferredContext;
}





typedef struct _XKTIMER
{
	XDISPATCHER_HEADER   Header;           // 0x00
	ULARGE_INTEGER      DueTime;          // 0x10
	LIST_ENTRY          TimerListEntry;   // 0x18
	struct _KDPC* Dpc;              // 0x20
	long_xt             Period;           // 0x24
}
KTIMER, * PKTIMER;


VOID KeInitializeTimerEx(_XKTIMER* Timer, uint32_t Type)
{
	Timer->Header.Type = Type + 8;
	Timer->Header.Inserted = FALSE;
	Timer->Header.Size = sizeof(xbox::_KTIMER) / sizeof(ULONG);
	Timer->Header.SignalState = 0;

	Timer->TimerListEntry.Blink = NULL;
	Timer->TimerListEntry.Flink = NULL;
	Timer->DueTime.QuadPart = 0;
	Timer->Period = 0;

}

std::uint8_t KeRemoveQueueDpc(uint32_t* Dpc_)
{
	KDPC Dpc = (KDPC)*Dpc_;
	printf("[STUB] KeRemoveQueueDpc(%p)\n", Dpc);
	return TRUE;
}

uint32_t SaveArea;
// In a real implementation, would restore FPU/XMM registers
int32_t KeRestoreFloatingPointState(uint32_t* SaveArea_)
{
	SaveArea_ = &SaveArea;
	printf("[STUB] KeRestoreFloatingPointState(%p)\n", SaveArea);
	LONG ret = X_STATUS_SUCCESS;
	return ret;
}

LONG KeSaveFloatingPointState(uint32_t* SaveArea_)
{
	SaveArea = *SaveArea_;
	printf("[STUB] KeSaveFloatingPointState(%p)\n", SaveArea);
	return X_STATUS_SUCCESS;
}

int map_xbox_priority(int xbox_priority) {
	switch (xbox_priority) {
	case -2:
		return THREAD_PRIORITY_LOWEST;
	case 1:
	case 2:
		return THREAD_PRIORITY_BELOW_NORMAL;
	case 8:
		return THREAD_PRIORITY_NORMAL;
	case 16:
		return THREAD_PRIORITY_TIME_CRITICAL;
	default:
		return THREAD_PRIORITY_NORMAL;
	}
}

std::int32_t KeSetBasePriorityThread(GuestThreadHandle* hThread, LONG priority)
{
#ifdef _WIN32
	if (priority == 16)
	{
		priority = 15;
	}
	else if (priority == -16)
	{
		priority = -15;
	}

	SetThreadPriority(hThread == GetKernelObject(CURRENT_THREAD_HANDLE) ? GetCurrentThread() : hThread->thread.native_handle(), priority);
#endif

	return priority;
}

std::uint8_t KeSetDisableBoostThread(uint32_t* Thread_, BOOLEAN Disable)
{
	printf("[STUB] KeSetDisableBoostThread(%p, %d)\n");
	return 0;
}

LONG KeSetEvent(XKEVENT* pEvent, uint32_t Increment, bool Wait)
{
	bool result = QueryKernelObject<Event>(*pEvent)->Set();

	++g_keSetEventGeneration;
	g_keSetEventGeneration.notify_all();

	return result;
}

BOOLEAN KeSetTimer(uint32_t* Dpc, uint32_t* DueTime, uint32_t* Timer)
{
	//return FALSE;


	//printf("[STUB] KeSetTimer(%p, %lld ns, Dpc %p)\n", Timer, DueTime.QuadPart, Dpc);
	return FALSE; // Timer wasn't already active
}

VOID KeStallExecutionProcessor(ULONG Microseconds)
{
	printf("[STUB] KeStallExecutionProcessor(%u)\n", Microseconds);
}

BOOLEAN KeSynchronizeExecution(uint32_t* Interrupt_, uint32_t* SynchronizeRoutine_, uint32_t SynchronizeContext)
{
	xbox::PKSYNCHRONIZE_ROUTINE SynchronizeRoutine = (xbox::PKSYNCHRONIZE_ROUTINE)SynchronizeRoutine_;
	xbox::PKINTERRUPT Interrupt = (xbox::PKINTERRUPT)Interrupt_;
	printf("[STUB] KeSynchronizeExecution(Int %p, Routine %p, Ctx %p)\n", Interrupt, SynchronizeRoutine, SynchronizeContext);
	return SynchronizeRoutine(&SynchronizeContext);
}

LONG KeWaitForSingleObject(XDISPATCHER_HEADER* Object, uint32_t WaitReason, uint32_t WaitMode, bool Alertable, int64_t* Timeout)
{
	const uint32_t timeout = GuestTimeoutToMilliseconds(Timeout);
	assert(timeout == INFINITE);

	switch (Object->Type)
	{
	case 0:
	case 1:
		QueryKernelObject<Event>(*Object)->Wait(timeout);
		break;
	default:
		assert(false && "Unrecognized kernel object type.");
		return STATUS_TIMEOUT;
	}

	return X_STATUS_SUCCESS;
}

std::uint8_t KfRaiseIrql(std::uint8_t NewIrql)
{
	std::uint8_t old = PASSIVE_LEVEL;
	printf("[STUB] KfRaiseIrql(%d) -> %d\n", NewIrql, old);
	return old; // Return previous IRQL
}

uint32_t* MmAllocateContiguousMemory(uint32_t NumberOfBytes)
{
	printf("[STUB] MmAllocateContiguousMemory(%zu bytes)\n", NumberOfBytes);
	return reinterpret_cast<uint32_t*>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, GPU_ALIGNMENT)));
}

uint32_t* MmAllocateContiguousMemoryEx(uint32_t NumberOfBytes, uint32_t LowestAcceptableAddress,
	uint32_t HighestAcceptableAddress, uint32_t Alignment,
	uint32_t ProtectionType
)
{
	printf("MmAllocateContiguousMemoryEx(%zu bytes, Align %u)\n\n", NumberOfBytes, Alignment);
	return reinterpret_cast<uint32_t*>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, Alignment)));
}

PVOID MmClaimGpuInstanceMemory(SIZE_T NumberOfBytes, ULONG Protect
) {
	printf("[STUB] MmClaimGpuInstanceMemory(%zu bytes, Prot 0x%X)\n",
		NumberOfBytes, Protect);

	return reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, GPU_ALIGNMENT)));
}

void MmFreeContiguousMemory(uint32_t guestAddress)
{
	if (guestAddress != NULL)
		g_userHeap.Free(g_memory.Translate(guestAddress));
}

VOID MmFreeSystemMemory(uint32_t* BaseAddress, uint32_t NumberOfBytes)
{
	printf("[STUB] MmFreeSystemMemory(%p, %zu bytes)\n", BaseAddress, NumberOfBytes);

	//g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
}

uint32_t MmAllocateSystemMemory(uint32_t NumberOfBytes, ULONG Protect)
{
	printf("MmAllocateSystemMemory(%zu bytes, Prot 0x%X)", NumberOfBytes, Protect);
	return g_memory.MapVirtual(g_userHeap.Alloc(NumberOfBytes));
}

void KfLowerIrql(std::uint8_t NewIrql)
{
	//printf("[STUB] KfLowerIrql(%d)\n", NewIrql);
}

LONG NtClose(uint32_t Handle)
{
	printf("NtClose(%p) called\n", Handle);

	if (Handle == 0)
	{
		printf("NtClose: No handle detected for this thread, aborting deleting handle.\n");
		return X_STATUS_SUCCESS;
	}

	if (Handle == GUEST_INVALID_HANDLE_VALUE)
		return 0xFFFFFFFF;

	if (IsKernelObject(Handle))
	{
		DestroyKernelObject(Handle);
		return 0;
	}
	else
	{
		assert(false && "Unrecognized kernel object.");
		return 0xFFFFFFFF;
	}

	return 1;
}

LONG NtCreateEvent(uint32_t* EventHandle, void* bjectAttributes, uint32_t EventType, std::uint8_t InitialState)
{
	*EventHandle = GetKernelHandle(CreateKernelObject<Event>(!EventType, !!InitialState));
	return X_STATUS_SUCCESS;
}

#ifdef _WIN32
#include <windows.h>

using FileHandleType = HANDLE;
#define INVALID_HANDLE_VALUE_WIN32 INVALID_HANDLE_VALUE
#else
using FileHandleType = int;
#define INVALID_HANDLE_VALUE_WIN32 (-1)
#endif

LONG NtCreateFile(
	OUT uint32_t* FileHandle,
	IN  uint32_t         DesiredAccess,
	IN  uint32_t* ObjectAttributes_,
	OUT uint32_t* IoStatusBlock_,
	IN  uint32_t* AllocationSize_ OPTIONAL,
	IN  uint32_t               FileAttributes,
	IN  uint32_t               ShareAccess,
	IN  uint32_t               CreateDisposition,
	IN  uint32_t               CreateOptions
)
{

	printf("Stub ntCreatefile\n");
	return 0;
}

LONG NtDeviceIoControlFile(
	HANDLE FileHandle,
	HANDLE Event,
	uint32_t* ApcRoutine,
	PVOID ApcContext,
	uint32_t* IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength
) {
	LOG_UTIL("!!! STUB !!!");
	return X_STATUS_SUCCESS;
}

LONG NtFlushBuffersFile(HANDLE FileHandle, uint32_t* IoStatusBlock)
{
#ifdef _WIN32
	FlushFileBuffers(FileHandle);
#else
	fsync(reinterpret_cast<int>(FileHandle));
#endif
	return X_STATUS_SUCCESS;
}

LONG NtFsControlFile(
	uint32_t* FileHandle,
	uint32_t* Event,
	uint32_t* ApcRoutine,
	uint32_t* ApcContext,
	uint32_t* IoStatusBlock,
	ULONG FsControlCode,
	uint32_t* InputBuffer,
	ULONG InputBufferLength,
	uint32_t* OutputBuffer,
	ULONG OutputBufferLength
) {
	LOG_UTIL("!!! STUB !!!");
	return X_STATUS_SUCCESS;
}

LONG NtOpenFile(
	uint32_t* FileHandle,
	ACCESS_MASK DesiredAccess,
	uint32_t* ObjectAttributes_,
	uint32_t* IoStatusBlock_,
	ULONG ShareAccess,
	ULONG OpenOptions
) {

	//xbox::POBJECT_ATTRIBUTES ObjectAttributes = (xbox::POBJECT_ATTRIBUTES)ObjectAttributes_;
	//xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
	// Reuse NtCreateFile logic
	//_LARGE_INTEGER t;
	//memset(&t, 0, sizeof(_LARGE_INTEGER));
	return NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes_, IoStatusBlock_, 0, ShareAccess, FILE_OPEN, OpenOptions, NULL);
}

LONG NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle, IN void* ObjectAttributes
) {
	*LinkHandle = reinterpret_cast<HANDLE>(0xCAFEBABE);
	return X_STATUS_SUCCESS;
}

LONG NtQueryDirectoryFile(
	IN  HANDLE FileHandle,
	IN  HANDLE Event OPTIONAL,
	IN  uint32_t* ApcRoutine,
	IN  uint32_t* ApcContext,
	OUT uint32_t* IoStatusBlock,
	OUT uint32_t* FileInformation_,
	IN  uint32_t Length,
	IN  uint32_t FileInformationClass,
	IN  uint32_t* FileMask,
	IN  std::uint8_t RestartScan
)
{
	printf("QUERY DIRECTION FILE STUB\n");
	//xbox::FILE_INFORMATION_CLASS fileInfo = (xbox::FILE_INFORMATION_CLASS)FileInformationClass;
	//xbox::_FILE_DIRECTORY_INFORMATION FileInformation = (xbox::_FILE_DIRECTORY_INFORMATION)*FileInformation_;
	//memset(FileInformation, 0, Length);
	return X_STATUS_SUCCESS;
}

LONG NtQueryFullAttributesFile(uint32_t* ObjectAttributes_, uint32_t* FileInformation_
)
{
	printf("NtQueryFullAttributesFile STUB\n");
	//xbox::POBJECT_ATTRIBUTES ObjectAttributes = (xbox::POBJECT_ATTRIBUTES)ObjectAttributes_;
	//xbox::PFILE_NETWORK_OPEN_INFORMATION FileInformation = (xbox::PFILE_NETWORK_OPEN_INFORMATION)ObjectAttributes_;
	//FileInformation->EndOfFile.QuadPart = 0x1000; // Dummy size
	return X_STATUS_SUCCESS;
}

LONG NtQueryInformationFile(
	uint32_t* FileHandle,
	uint32_t* IoStatusBlock_,
	uint32_t* FileInformation,
	ULONG FileInformationLength,
	uint32_t FileInformationClass_
)
{
	LOG_UTIL("!!! STUB !!!");
	/**xbox::FILE_INFORMATION_CLASS FileInformationClass = (xbox::FILE_INFORMATION_CLASS)FileInformationClass_;
	xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;

	if (FileInformationClass == xbox::FileStandardInformation)
	{
		auto* info = reinterpret_cast<xbox::FILE_STANDARD_INFORMATION*>(FileInformation);
		info->EndOfFile.QuadPart = 0x1000; // Dummy 4KB file
	}*/

	return X_STATUS_SUCCESS;
}

LONG NtQuerySymbolicLinkObject(HANDLE LinkHandle, void* TargetName_, PULONG ReturnedLength
)
{

	xbox::PUNICODE_STRING TargetName = (xbox::PUNICODE_STRING)TargetName_;
	// Xbox uses UTF-16 strings with 16-bit wchar_xt
	static const xbox::wchar_xt dummy[] = {
		'\\', 'D', 'e', 'v', 'i', 'c', 'e', '\\',
		'H', 'a', 'r', 'd', 'd', 'i', 's', 'k', '0', '\\',
		'P', 'a', 'r', 't', 'i', 't', 'i', 'o', 'n', '1', 0
	};

	// Calculate required bytes (including null terminator)
	size_t required_bytes = (sizeof(dummy) / sizeof(xbox::wchar_xt)) * sizeof(xbox::wchar_xt);

	// Check if buffer is large enough
	if (TargetName->MaximumLength < required_bytes) {
		if (ReturnedLength) *ReturnedLength = static_cast<ULONG>(required_bytes);
		return X_STATUS_BUFFER_TOO_SMALL;
	}

	// Copy the string
	memcpy(TargetName->Buffer, dummy, required_bytes);
	TargetName->Length = static_cast<USHORT>(required_bytes - sizeof(xbox::wchar_xt)); // Length without null terminator

	if (ReturnedLength) *ReturnedLength = static_cast<ULONG>(required_bytes);
	return X_STATUS_SUCCESS;
}

LONG NtQueryVirtualMemory(uint32_t* BaseAddress, uint32_t* Buffer)
{
	LOG_UTIL("STUB NtQueryVirtualMemory\n");

	if (!Buffer)
		return X_STATUS_UNSUCCESSFUL;

	//memset(Buffer, 0, sizeof(MEMORY_BASIC_INFORMATION));

	/**if (!g_memory.IsInMemoryRange(BaseAddress))
	{
		Buffer->BaseAddress = BaseAddress;
		Buffer->State = MEM_FREE;
		Buffer->RegionSize = 0x1000;
		return X_STATUS_SUCCESS;
	}

	Buffer->BaseAddress = BaseAddress;
	Buffer->AllocationBase = g_memory.base;
	Buffer->RegionSize = X86_MEMORY_SIZE;
	Buffer->State = MEM_COMMIT;
	Buffer->Protect = PAGE_READWRITE;
	Buffer->Type = MEM_PRIVATE;*/

	return X_STATUS_SUCCESS;
}


uint32_t MmGetPhysicalAddress(uint32_t BaseAddress)
{
	printf("MmGetPhysicalAddress: 0x%08X\n", BaseAddress);
	return BaseAddress;
}

LONG MmLockUnlockBufferPages(uint32_t* BaseAddress, uint32_t NumberOfBytes, BOOLEAN Lock)
{
	LOG_UTIL("!!! STUB !!!");
	printf("\n");
	return X_STATUS_SUCCESS; // No-op in recompiler
}

LONG MmLockUnlockPhysicalPage(uint32_t PhysicalAddress_, BOOLEAN Lock)
{
	xbox::physical_address_xt PhysicalAddress = PhysicalAddress_;
	uint32_t addr = static_cast<uint32_t>(PhysicalAddress);
	printf("MmLockUnlockPhysicalPage(0x%08X, %s)\n", addr, Lock ? "Lock" : "Unlock");
	printf("\n");
	return X_STATUS_SUCCESS;
}

VOID MmPersistContiguousMemory(uint32_t* BaseAddress, uint32_t NumberOfBytes, BOOLEAN Persist)
{
	LOG_UTIL("!!! STUB !!!");
}

LONG MmQueryAddressProtect(uint32_t* BaseAddress, uint32_t* Protect)
{
	printf("MmQueryAddressProtect\n");
	*Protect = PAGE_READWRITE;
	return X_STATUS_SUCCESS;
}

SIZE_T MmQueryAllocationSize(uint32_t* BaseAddress)
{
	return g_userHeap.Size(&BaseAddress);
}

LONG MmSetAddressProtect(uint32_t* BaseAddress, SIZE_T NumberOfBytes, ULONG NewProtect)
{
	LOG_UTIL("!!! STUB !!!");
	return X_STATUS_SUCCESS;
}

LONG NtWaitForSingleObject(uint32_t Handle, BOOLEAN Alertable, int64_t* Timeout)
{
	uint32_t timeout = GuestTimeoutToMilliseconds(Timeout);
	assert(timeout == 0 || timeout == INFINITE);

	if (IsKernelObject(Handle))
	{
		return GetKernelObject(Handle)->Wait(timeout);
	}
	else
	{
		assert(false && "Unrecognized handle value.");
	}

	return STATUS_TIMEOUT;
}

LONG NtSetEvent(Event* handle, uint32_t* PreviousState)
{
	LOG_UTIL(" Handle Set");
	handle->Set();

	return X_STATUS_SUCCESS;
}

LONG NtWriteFile(
	uint32_t* FileHandle,
	uint32_t* Event,
	uint32_t** ApcRoutine_,
	uint32_t* ApcContext,
	uint32_t** IoStatusBlock_,
	uint32_t* Buffer,
	ULONG Length,
	uint32_t* ByteOffset
) {

	LOG_UTIL("!!! STUB !!!");
	/**xbox::PIO_APC_ROUTINE Apcroutine = (xbox::PIO_APC_ROUTINE)ApcRoutine_;
	xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
	if (IoStatusBlock)
	{
		IoStatusBlock->Status = X_STATUS_SUCCESS;
		IoStatusBlock->Information = Length; // Pretend all bytes were written
	}*/

	return X_STATUS_SUCCESS;
}

ULONG RtlLONGToDosError(LONG Status)
{
	LOG_UTIL("!!! STUB !!!");
	return X_STATUS_SUCCESS;
}

void NtYieldExecution() {
	std::this_thread::yield();
}

SIZE_T RtlCompareMemoryUlong(uint32_t* Buffer, uint32_t Length, ULONG Pattern)
{
	printf("RtlCompareMemoryUlong Pretend bytes match\n");
	return Length; // Pretend all bytes match
}

BOOLEAN RtlEqualString(PCSTR String1, PCSTR String2)
{
	printf("RtlEqualString\n");
	return strcmp(String1, String2) == 0;
}

LONG NtSuspendThread(GuestThreadHandle* ThreadHandle, uint32_t* PreviousSuspendCount)
{
	//printf("NtSuspendThread(0x%X) id: (0x%X)\n", ThreadHandle, ThreadHandle->thread.get_id());
	assert(ThreadHandle != GetKernelObject(CURRENT_THREAD_HANDLE) && ThreadHandle->thread.get_id() == std::this_thread::get_id());

	ThreadHandle->suspended = true;
	ThreadHandle->suspended.wait(true);

	return S_OK;
}

LONG NtResumeThread(GuestThreadHandle* ThreadHandle, uint32_t* PreviousSuspendCount)
{
	//printf("NtResumeThread(0x%X), id: (0x%X)\n", ThreadHandle, ThreadHandle->thread.get_id());
	assert(ThreadHandle != GetKernelObject(CURRENT_THREAD_HANDLE));

	ThreadHandle->suspended = false;
	ThreadHandle->suspended.notify_all();

	return X_STATUS_SUCCESS;
}

typedef void_xt(NTAPI* PKSTART_ROUTINE)
(
	IN PVOID StartContext
	);

typedef void_xt(NTAPI* PKSYSTEM_ROUTINE)
(
	IN PKSTART_ROUTINE StartRoutine OPTIONAL,
	IN PVOID StartContext OPTIONAL
	);



LONG PsCreateSystemThreadEx(
	OUT uint32_t* ThreadHandle,
	IN  ulong_xt           ThreadExtensionSize,
	IN  ulong_xt           KernelStackSize,
	IN  ulong_xt           TlsDataSize,
	OUT uint32_t          ThreadId OPTIONAL,
	IN  uint32_t StartRoutine,
	IN  uint32_t           StartContext,
	IN  boolean_xt         CreateSuspended,
	IN  boolean_xt         DebuggerThread,
	IN  uint32_t SystemRoutine OPTIONAL)
{

	if (KernelStackSize < KERNEL_STACK_SIZE)
		KernelStackSize = KERNEL_STACK_SIZE;

	// Double the stack size, this is to account for the overhead HLE patching adds to the stack
	uint32_t hKernelStackSize = KernelStackSize * 2;

	// round up to the next page boundary if un-aligned
	hKernelStackSize = RoundUp(hKernelStackSize, PAGE_SIZE);

	uint32_t hostThreadId;
	auto handle = GetKernelHandle(GuestThread::Start({ (uint32_t)StartRoutine, 0, 0, TlsDataSize, hKernelStackSize }, &hostThreadId));
	*ThreadHandle = handle;

	return X_STATUS_SUCCESS;
}

void PsTerminateSystemThread(LONG ExitStatus)
{
	// No-op (thread cleanup is handled by the recompiler)
}

LONG ObReferenceObjectByHandle(uint32_t Handle, uint32_t ObjectType, uint32_t* Object)
{
	*Object = Handle;
	return X_STATUS_SUCCESS;
}

void ObfDereferenceObject(PVOID Object) {
	// No-op
}

typedef struct _XRTL_CRITICAL_SECTION
{
	XDISPATCHER_HEADER   Event;
	long_xt             LockCount;
	long_xt             RecursionCount;
	uint32_t           OwningThread;
} XRTL_CRITICAL_SECTION;

typedef struct _XANSI_STRING {
	uint16_t Length;
	uint16_t MaximumLength;
	xpointer<char> Buffer;
} XANSI_STRING;

void RtlInitializeCriticalSection(XRTL_CRITICAL_SECTION* CritSec)
{
	CritSec->Event.Absolute = 0;
	CritSec->LockCount = -1;
	CritSec->RecursionCount = 0;
	CritSec->OwningThread = 0;
}

void RtlEnterCriticalSection(XRTL_CRITICAL_SECTION* CritSec)
{
	uint32_t thisThread = g_x86Context->kpcr_ptr;
	assert(thisThread != NULL);

	std::atomic_ref owningThread(CritSec->OwningThread);

	while (true)
	{
		uint32_t previousOwner = 0;

		if (owningThread.compare_exchange_weak(previousOwner, thisThread) || previousOwner == thisThread)
		{
			CritSec->RecursionCount++;
			return;
		}

		owningThread.wait(previousOwner);
	}
}

void RtlLeaveCriticalSection(XRTL_CRITICAL_SECTION* CritSec)
{
	CritSec->RecursionCount--;

	if (CritSec->RecursionCount != 0)
		return;

	std::atomic_ref owningThread(CritSec->OwningThread);
	owningThread.store(0);
	owningThread.notify_one();
}

void RtlInitAnsiString(_XANSI_STRING* Destination, char* Source)
{
	const uint16_t length = Source ? (uint16_t)strlen(Source) : 0;
	Destination->Length = length;
	Destination->MaximumLength = length + 1;
	Destination->Buffer = (xpointer<char>)(Source);
}

LONG NtQueryVolumeInformationFile(
	uint32_t* FileHandle,
	uint32_t* IoStatusBlock_,
	uint32_t* FsInformation,
	ULONG Length,
	uint32_t FsInformationClass_
)
{
	/**xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
	xbox::FS_INFORMATION_CLASS FsInformationClass = (xbox::FS_INFORMATION_CLASS)FsInformationClass_;
	if (IoStatusBlock)
	{
		IoStatusBlock->Status = X_STATUS_SUCCESS;
		IoStatusBlock->Information = 0;
	}

	// Return dummy volume info
	if (FsInformationClass == xbox::FileFsVolumeInformation)
	{
		auto* info = (xbox::FILE_FS_VOLUME_INFORMATION*)FsInformation;
		info->VolumeSerialNumber = 0x1234ABCD;
		info->VolumeLabelLength = 0;
	}*/

	return X_STATUS_SUCCESS;
}

LONG NtSetInformationFile(
	uint32_t* FileHandle,
	uint32_t* IoStatusBlock_,
	uint32_t* FileInformation,
	ULONG Length,
	uint32_t* FileInformationClass
)
{

	/**xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
	//xbox::FILE_INFORMATION_CLASS fileInfo = (xbox::FILE_INFORMATION_CLASS*)FileInformationClass;
	if (IoStatusBlock)
	{
		IoStatusBlock->Status = X_STATUS_SUCCESS;
		IoStatusBlock->Information = 0;
	}*/
	return X_STATUS_SUCCESS;
}

void HalInitiateShutdown() {
	LOG_UTIL("!!! STUB !!!");
}

LONG XeLoadSection(const char* Name)
{
	return X_STATUS_SUCCESS;
}

LONG XeUnloadSection(const char* Name)
{
	return X_STATUS_SUCCESS;
}

void XcSHAInit(uint32_t* Context)
{
	memset(Context, 0, 20); //SHA-1 context size
}

void XcSHAUpdate(uint32_t* Context, const uint32_t* Data, ULONG Length)
{
	// No-op
}

void XcSHAFinal(uint32_t* Context, uint32_t* Digest)
{
	memset(Digest, 0xAA, 20); //Dummy SHA-1 hash
}

void XcHMAC(uint32_t* Key, ULONG KeySize, uint32_t* Data, ULONG DataSize, uint32_t* Digest)
{
	memset(Digest, 0xBB, 20);
}

void RtlRaiseException_(uint32_t* ExceptionRecord)
{
	printf("RtlRaiseException\n");
	std::terminate();
}

void RtlUnwind_()
{
	LOG_UTIL("!!! STUB !!!");
}

uint32_t RtlNtStatusToDosError(xbox::ntstatus_xt status)
{
	if (!status || (status & 0x20000000))
		return TRUE;

	return TRUE;
}

uint32_t NtWaitForSingleObjectEx(uint32_t Handle, uint32_t WaitMode, uint32_t Alertable, int64_t* Timeout)
{
	uint32_t timeout = GuestTimeoutToMilliseconds(Timeout);
	assert(timeout == 0 || timeout == INFINITE);

	if (IsKernelObject(Handle))
	{
		return GetKernelObject(Handle)->Wait(timeout);
	}
	else
	{
		assert(false && "Unrecognized handle value.");
	}

	return STATUS_TIMEOUT;
}


GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlInitializeCriticalSection, RtlInitializeCriticalSection);
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlEnterCriticalSection, RtlEnterCriticalSection);
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlLeaveCriticalSection, RtlLeaveCriticalSection);
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlCompareMemoryUlong, RtlCompareMemoryUlong);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtFreeVirtualMemory, NtFreeVirtualMemory);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtAllocateVirtualMemory, NtAllocateVirtualMemory);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtQueryVirtualMemory, NtQueryVirtualMemory);
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlRaiseException, RtlRaiseException_);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmPersistContiguousMemory, MmPersistContiguousMemory);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmAllocateContiguousMemory, MmAllocateContiguousMemory);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtQueryFullAttributesFile, NtQueryFullAttributesFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlInitAnsiString, RtlInitAnsiString);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtOpenFile, NtOpenFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtClose, NtClose);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtSetInformationFile, NtSetInformationFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__ExQueryNonVolatileSetting, ExQueryNonVolatileSetting);
GUEST_FUNCTION_HOOK_STDCALL(__imp__HalReturnToFirmware, HalReturnToFirmware);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmFreeContiguousMemory, MmFreeContiguousMemory);
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlNtStatusToDosError, RtlNtStatusToDosError);
GUEST_FUNCTION_HOOK_STDCALL(__imp__FscSetCacheSize, FscSetCacheSize);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtQueryDirectoryFile, NtQueryDirectoryFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtCreateEvent, NtCreateEvent);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtSetEvent, NtSetEvent);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtWaitForSingleObjectEx, NtWaitForSingleObjectEx);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeDelayExecutionThread, KeDelayExecutionThread)
GUEST_FUNCTION_HOOK_FASTCALL(__imp__ObfDereferenceObject, ObfDereferenceObject);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeSetBasePriorityThread, KeSetBasePriorityThread);
GUEST_FUNCTION_HOOK_STDCALL(__imp__ObReferenceObjectByHandle, ObReferenceObjectByHandle);
GUEST_FUNCTION_HOOK_STDCALL(__imp__PsCreateSystemThreadEx, PsCreateSystemThreadEx);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeQueryBasePriorityThread, KeQueryBasePriorityThread);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeSetDisableBoostThread, KeSetDisableBoostThread);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtResumeThread, NtResumeThread);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtSuspendThread, NtSuspendThread);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtYieldExecution, NtYieldExecution);
GUEST_FUNCTION_HOOK_STDCALL(__imp__PsTerminateSystemThread, PsTerminateSystemThread);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtWaitForSingleObject, NtWaitForSingleObject);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtReadFile, NtReadFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtWriteFile, NtWriteFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtQueryInformationFile, NtQueryInformationFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtQueryVolumeInformationFile, NtQueryVolumeInformationFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtFlushBuffersFile, NtFlushBuffersFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtFsControlFile, NtFsControlFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtDeviceIoControlFile, NtDeviceIoControlFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__XcSHAInit, XcSHAInit);
GUEST_FUNCTION_HOOK_STDCALL(__imp__XcSHAUpdate, XcSHAUpdate);
GUEST_FUNCTION_HOOK_STDCALL(__imp__XcSHAFinal, XcSHAFinal);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeQuerySystemTime, KeQuerySystemTime);

GUEST_FUNCTION_HOOK_STDCALL(__imp__NtCreateFile, NtCreateFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__IoCreateSymbolicLink, IoCreateSymbolicLink);
GUEST_FUNCTION_HOOK_STDCALL(__imp__IoDeleteSymbolicLink, IoDeleteSymbolicLink);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtQuerySymbolicLinkObject, NtQuerySymbolicLinkObject);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtOpenSymbolicLinkObject, NtOpenSymbolicLinkObject);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmFreeSystemMemory, MmFreeSystemMemory);

GUEST_FUNCTION_HOOK_STDCALL(__imp__MmAllocateSystemMemory, MmAllocateSystemMemory);
GUEST_FUNCTION_HOOK_FASTCALL(__imp__KfLowerIrql, KfLowerIrql);

GUEST_FUNCTION_HOOK_STDCALL(__imp__KeRaiseIrqlToDpcLevel, KeRaiseIrqlToDpcLevel);
GUEST_FUNCTION_HOOK_STDCALL(__imp__HalRegisterShutdownNotification, HalRegisterShutdownNotification);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeInitializeTimerEx, KeInitializeTimerEx);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeSetTimer, KeSetTimer);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeCancelTimer, KeCancelTimer);


GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlLONGToDosError, RtlLONGToDosError); //not found???? 
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlUnwind, RtlUnwind_);
GUEST_FUNCTION_HOOK_STDCALL(__imp__XcHMAC, XcHMAC);
GUEST_FUNCTION_HOOK_STDCALL(__imp__XeLoadSection, XeLoadSection);
GUEST_FUNCTION_HOOK_STDCALL(__imp__XeUnloadSection, XeUnloadSection);
GUEST_FUNCTION_HOOK_STDCALL(__imp__HalInitiateShutdown, HalInitiateShutdown);
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlEqualString, RtlEqualString);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmSetAddressProtect, MmSetAddressProtect);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmQueryAllocationSize, MmQueryAllocationSize);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmQueryAddressProtect, MmQueryAddressProtect);


GUEST_FUNCTION_HOOK_STDCALL(__imp__MmLockUnlockPhysicalPage, MmLockUnlockPhysicalPage);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmLockUnlockBufferPages, MmLockUnlockBufferPages);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmGetPhysicalAddress, MmGetPhysicalAddress);
GUEST_FUNCTION_HOOK_FASTCALL(__imp__KfRaiseIrql, KfRaiseIrql);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeWaitForSingleObject, KeWaitForSingleObject);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeSynchronizeExecution, KeSynchronizeExecution);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmClaimGpuInstanceMemory, MmClaimGpuInstanceMemory);
GUEST_FUNCTION_HOOK_STDCALL(__imp__MmAllocateContiguousMemoryEx, MmAllocateContiguousMemoryEx);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeStallExecutionProcessor, KeStallExecutionProcessor);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeSetEvent, KeSetEvent);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeSaveFloatingPointState, KeSaveFloatingPointState);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeRestoreFloatingPointState, KeRestoreFloatingPointState);

GUEST_FUNCTION_HOOK_STDCALL(__imp__KeRemoveQueueDpc, KeRemoveQueueDpc);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeInitializeDpc, KeInitializeDpc);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeConnectInterrupt, KeConnectInterrupt);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeDisconnectInterrupt, KeDisconnectInterrupt);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeInitializeInterrupt, KeInitializeInterrupt);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeInsertQueueDpc, KeInsertQueueDpc);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeBugCheck, KeBugCheck);
GUEST_FUNCTION_HOOK_STDCALL(__imp__ExQueryPoolBlockSize, ExQueryPoolBlockSize);
GUEST_FUNCTION_HOOK_STDCALL(__imp__HalReadWritePCISpace, HalReadWritePCISpace);
GUEST_FUNCTION_HOOK_STDCALL(__imp__HalGetInterruptVector, HalGetInterruptVector);
GUEST_FUNCTION_HOOK_STDCALL(__imp__ExAllocatePoolWithTag, ExAllocatePoolWithTag);


GUEST_FUNCTION_HOOK_STDCALL(__imp__ExFreePool, ExFreePool);
GUEST_FUNCTION_HOOK_STDCALL(__imp__AvGetSavedDataAddress, AvGetSavedDataAddress);
GUEST_FUNCTION_HOOK_STDCALL(__imp__AvSetSavedDataAddress, AvSetSavedDataAddress);
GUEST_FUNCTION_HOOK_STDCALL(__imp__AvSetDisplayMode, AvSetDisplayMode);
GUEST_FUNCTION_HOOK_STDCALL(__imp__AvSendTVEncoderOption, AvSendTVEncoderOption);
GUEST_FUNCTION_HOOK(__imp__DbgPrint, DbgPrintV);


//XAutoPowerDownResetTimer
GUEST_FUNCTION_STUB(sub_2B17E8);

extern uint8_t* D3D_g_pDevice;
//TODO DELETE WHEN RENDERING IS IMPLEMENTED
int D3DDevice_BlockUntilVerticalBlank()
{
	auto ctx = *GetX86Context();
	auto base = g_memory.base;
	// mov eax, [0x003E0A18]
	ctx.eax.u32 = X86_LOAD_U32(4065816);
	// push 0x00
	X86_PUSH(ctx, base, 0x0);
	// push 0x00
	X86_PUSH(ctx, base, 0x0);
	// push 0x01
	X86_PUSH(ctx, base, 0x1);
	// mov dword ptr [eax+0x1DC0], 0x00
	X86_STORE_U32(ctx.eax.u32 + 7616, 0x0u);
	// push 0x06
	X86_PUSH(ctx, base, 0x6);
	// add eax, 0x1DBC
	auto temp_3CFE87_U32_OLD = ctx.eax.u32;
	auto temp_3CFE87_U32_SRC = 7612;
	ctx.eax.u32 = temp_3CFE87_U32_OLD + temp_3CFE87_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3CFE87_U32_OLD, temp_3CFE87_U32_SRC, ctx.eax.u32);
	// push eax
	X86_PUSH(ctx, base, ctx.eax.u32);
	ctx.esp.u32 += 0x4;  //Adjust stack pointer

	// Signal the VBlank event to unblock game logic
	//KeWaitForSingleObject((XDISPATCHER_HEADER*)vblankEvent, 0, 0, 0, 0);  // 0, 0: default priority, do not wait

	// Wait ~16.67ms to simulate 60Hz refresh rate
	Sleep(15);


	return 0;
}

GUEST_FUNCTION_HOOK(sub_3CFE70, D3DDevice_BlockUntilVerticalBlank);