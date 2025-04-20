#include <stdafx.h>
#include <cpu/x86_context.h>
#include <cpu/guest_thread.h>
#include "io/file_system.h"
#include "function.h"

#include "xbox_types.h"
#include "types.h"
#include "heap.h"
#include "memory.h"
#include <memory>
#include "kernel.h"
#include <util.h>
#include <xbox.h>

const std::string XBOX_BASE_DIR = ""; //todo


constexpr uint32_t GPU_ALIGNMENT = 4096;

#define LOG_UTILITY printf //todo replace

//IRQL levels
#define PASSIVE_LEVEL 0
#define DISPATCH_LEVEL 2
#define PROFILE_LEVEL 27
#define CLOCK_LEVEL 28
#define IPI_LEVEL 29
#define POWER_LEVEL 30
#define HIGH_LEVEL 31

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
	LOG_UTILITY("!!! STUB !!!");
	return X_STATUS_SUCCESS;
}

void NtReadFile()
{
	LOG_UTILITY("!!! STUB !!!");
}

void AvGetSavedDataAddress()
{
	LOG_UTILITY("!!! STUB !!!");
}


int32_t NtFreeVirtualMemory(IN OUT uint32_t* BaseAddress, IN OUT ULONG32 FreeSize, IN uint32_t FreeType)
{
	printf("NtFreeVirtualMemory\n");
	g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
	return X_STATUS_SUCCESS;
}

int32_t NtAllocateVirtualMemory(IN OUT uint32_t* BaseAddress,
	IN ulong_xt         ZeroBits,
	IN OUT uint32_t*    AllocationSize,
	IN dword_xt         AllocationType,
	IN dword_xt         Protect)
{
	void* ptr = g_userHeap.Alloc(*AllocationSize);

	uint32_t xbox_addr = g_memory.MapVirtual(ptr);
	*BaseAddress = reinterpret_cast<uint32_t>(xbox_addr);

	printf("NtAllocateVirtualMemory: %zu bytes -> 0x%08X\n\n", *AllocationSize, xbox_addr);
	return X_STATUS_SUCCESS;
}

void* AvSavedDataAddress;
void AvSetSavedDataAddress(IN  uint32_t* Address)
{
	LOG_UTILITY("%d", Address);
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
	LOG_UTILITY("!!! STUB !!!");
}

void* ExAllocatePoolWithTag
(
	IN uint32_t NumberOfBytes,
	IN uint32_t Tag
)
{
	void* ptr = reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.Alloc(NumberOfBytes > 0 ? NumberOfBytes : 1)));
	printf("  Allocated dummy ptr: %p\n", ptr);
	return ptr;
}

uint32_t ExQueryPoolBlockSize
(
	IN PVOID PoolBlock
)
{
	printf("[STUB] ExQueryPoolBlockSize(%p) -> Dummy 4096\n", PoolBlock);
	return 4096;
}


void LockEeprom()
{
	//EnterCriticalSection(get_eeprom_crit_section());
}

LONG ExQueryNonVolatileSetting(
	IN  dword_xt  ValueIndex,
	OUT dword_xt* Type,
	OUT uint32_t*   Value,
	IN  size_xt   ValueLength,
	OUT uint32_t*  ResultLength OPTIONAL
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


	exit(0);
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

void KeQuerySystemTime(uint32_t* CurrentTime_)
{
	xpointer<PLARGE_INTEGER*> CurrentTime = xpointer<PLARGE_INTEGER*>(*CurrentTime_);
	static LONGLONG fakeTime = 0;
	fakeTime += 10000000; // Increment by 1 second

	//CurrentTime->QuadPart = fakeTime;
	//printf("[STUB] KeQuerySystemTime() -> %lld\n", CurrentTime->QuadPart);
}

LONG KeQueryBasePriorityThread(IN uint32_t* Thread)
{
	//xbox::PKTHREAD t = (xbox::PKTHREAD)Thread;
	printf("[STUB] KeQueryBasePriorityThread(%p)\n");
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

uint32_t GuestTimeoutToMilliseconds(uint32_t* timeout)
{
	return timeout ? (*timeout * -1) / 10000 : INFINITE;
}

LONG KeDelayExecutionThread(uint8_t WaitMode, BOOLEAN Alertable, uint32_t* Interval)
{

	LOG_UTILITY("KeDelayExecutionThread\n");

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

void InitializeListHead(xpointer<xbox::LIST_ENTRY> pListHead_)
{
	xbox::PLIST_ENTRY pListHead = (xbox::PLIST_ENTRY)pListHead_;
	pListHead->Flink = pListHead->Blink = pListHead;
}

xbox::_KTIMER* g_timers = nullptr;

void InitializeListHead(xbox::PLIST_ENTRY pListHead)
{
	pListHead->Flink = pListHead->Blink = pListHead;
}

VOID KeInitializeTimerEx(xbox::_KTIMER* Timer, uint32_t Type)
{
	Timer->Header.Type = Type + 8;
	Timer->Header.Inserted = FALSE;
	Timer->Header.Size = sizeof(xbox::_KTIMER) / sizeof(ULONG);
	Timer->Header.SignalState = 0;
	InitializeListHead(&(Timer->Header.WaitListHead));
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

// In a real implementation, would restore FPU/XMM registers
int32_t KeRestoreFloatingPointState(uint32_t* SaveArea_)
{
	xbox::PKFLOATING_SAVE SaveArea = (xbox::PKFLOATING_SAVE)SaveArea_;
	printf("[STUB] KeRestoreFloatingPointState(%p)\n", SaveArea);
	LONG ret = X_STATUS_SUCCESS;
	return ret;
}

LONG KeSaveFloatingPointState(uint32_t* SaveArea_)
{
	xbox::PKFLOATING_SAVE SaveArea = (xbox::PKFLOATING_SAVE)SaveArea_;
	printf("[STUB] KeSaveFloatingPointState(%p)\n", SaveArea);
	return X_STATUS_SUCCESS;
}

std::int32_t KeSetBasePriorityThread(uint32_t* Thread_, LONG Priority)
{
	xbox::PKTHREAD Thread = (xbox::PKTHREAD)Thread_;
	Thread->Priority = Priority;
	std::int32_t ret = Thread->Priority;
	printf("[STUB] KeSetBasePriorityThread(%p, %ld)\n", Thread, Priority);
	return ret;
}

std::uint8_t KeSetDisableBoostThread(uint32_t* Thread_, BOOLEAN Disable)
{
	xbox::PKTHREAD Thread = (xbox::PKTHREAD)Thread_;
	printf("[STUB] KeSetDisableBoostThread(%p, %s)\n",
		Thread, Disable ? "TRUE" : "FALSE");
	std::uint8_t prevDisableBoost = Thread->DisableBoost;
	Thread->DisableBoost = (CHAR)Disable;
	return prevDisableBoost;
}

LONG KeSetEvent(void* Event_, uint32_t Increment_, BOOLEAN Wait)
{
	xbox::PKEVENT Event = (xbox::PKEVENT)Event_;
	xbox::KPRIORITY Increment = (xbox::KPRIORITY)Increment_;
	printf("[STUB] KeSetEvent(%p, %d, %s)\n", Event, Increment, Wait ? "TRUE" : "FALSE");
	return 1;
}

BOOLEAN KeSetTimer(uint32_t* Dpc_, uint32_t* DueTime_, uint32_t* Timer_)
{
	//return FALSE;
	_LARGE_INTEGER DueTime;
	if (DueTime_)
		DueTime = *(_LARGE_INTEGER*)DueTime_;
	else
		DueTime.QuadPart = 0;

	xbox::PKTIMER Timer = (xbox::PKTIMER)Timer_;
	KDPC* Dpc = (KDPC*)Dpc_;
	printf("[STUB] KeSetTimer(%p, %lld ns, Dpc %p)\n", Timer, DueTime.QuadPart, Dpc);
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

LONG KeWaitForSingleObject(uint32_t Object, uint32_t WaitReason_, uint8_t WaitMode, BOOLEAN Alertable,
	uint32_t* Timeout
) {
	//xbox::KWAIT_REASON WaitReason = (xbox::KWAIT_REASON)WaitReason_;
	printf("[STUB] KeWaitForSingleObject(%p, Reason %d, %s, Timeout %lld)\n");

	return X_STATUS_SUCCESS; //Pretend object was signaled
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
	/**printf("[STUB] MmAllocateContiguousMemoryEx(%zu bytes, Align %u)\n",
		NumberOfBytes, Alignment);*/
	return reinterpret_cast<uint32_t*>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, Alignment)));
}

PVOID MmClaimGpuInstanceMemory(SIZE_T NumberOfBytes, ULONG Protect
) {
	printf("[STUB] MmClaimGpuInstanceMemory(%zu bytes, Prot 0x%X)\n",
		NumberOfBytes, Protect);

	return reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, GPU_ALIGNMENT)));
}

VOID MmFreeContiguousMemory(uint32_t* BaseAddress)
{
	printf("[STUB] MmFreeContiguousMemory(%p)\n", BaseAddress);
	g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
}

VOID MmFreeSystemMemory(uint32_t* BaseAddress, uint32_t NumberOfBytes)
{
	printf("[STUB] MmFreeSystemMemory(%p, %zu bytes)\n",
		BaseAddress, NumberOfBytes);

	g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
}

uint32_t MmAllocateSystemMemory(uint32_t NumberOfBytes, ULONG Protect)
{
	printf("[STUB] MmAllocateSystemMemory(%zu bytes, Prot 0x%X)\n",
		NumberOfBytes, Protect);
	return g_memory.MapVirtual(g_userHeap.Alloc(NumberOfBytes));
}

void KfLowerIrql(std::uint8_t NewIrql)
{
	printf("[STUB] KfLowerIrql(%d)\n", NewIrql);
}

LONG NtClose(uint32_t Handle)
{
	auto test = GetX86Context();
	test = test;
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

LONG NtCreateEvent(uint32_t* EventHandle, uint32_t* bjectAttributes, uint32_t EventType, std::uint8_t InitialState)
{

	*EventHandle = reinterpret_cast<uint32_t>(0xDEADBEEF); // Dummy handle
	LOG_UTILITY("NtCreateEvent() -> 0x%p", *EventHandle);
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

	xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
	xbox::POBJECT_ATTRIBUTES ObjectAttributes = (xbox::POBJECT_ATTRIBUTES)ObjectAttributes_;
	LARGE_INTEGER AllocationSize = *(LARGE_INTEGER*)AllocationSize_;
	const char* xbox_path = (const char*)ObjectAttributes->ObjectName->Buffer;
	if (!xbox_path) {
		return STATUS_INVALID_PARAMETER;
	}

	// --- Step 2: Convert Xbox path to host path ---
	std::string host_path;
	if (strncmp(xbox_path, "D:\\", 3) == 0)
	{
		host_path = XBOX_BASE_DIR + (xbox_path + 3);
	}
	else {
		// Handle other prefixes (e.g., "\\Device\\Cdrom0")
		host_path = xbox_path; // Fallback (may need special handling)
	}

	// --- Step 3: Open the file ---
	FileHandleType host_handle = INVALID_HANDLE_VALUE_WIN32;

#ifdef _WIN32
	// Windows implementation
	DWORD win_access = 0;
	if (DesiredAccess & GENERIC_READ) win_access |= GENERIC_READ;
	if (DesiredAccess & GENERIC_WRITE) win_access |= GENERIC_WRITE;

	host_handle = CreateFileA(
		host_path.c_str(),
		win_access,
		ShareAccess,
		NULL, // No security attributes
		CreateDisposition, // OPEN_EXISTING, CREATE_ALWAYS, etc.
		FileAttributes,
		NULL
	);
#else
	// Linux implementation todo

#endif

	if (host_handle == INVALID_HANDLE_VALUE_WIN32)
	{
		return STATUS_INVALID_PARAMETER;
	}

	*FileHandle = reinterpret_cast<uint32_t>(host_handle);


	if (IoStatusBlock) {
		IoStatusBlock->Status = X_STATUS_SUCCESS;
		IoStatusBlock->Information = 0;
	}
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
	LOG_UTILITY("NtDeviceIoControlFile(0x%X)", IoControlCode);
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
	LOG_UTILITY("NtFsControlFile(0x%X)", FsControlCode);
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
	IN  uint32_t* ApcRoutine, // Todo: define this routine's prototype
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
	LOG_UTILITY("NtQueryInformationFile STUB\n");
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
	LOG_UTILITY("STUB NtQueryVirtualMemory\n");

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


xbox::physical_address_xt MmGetPhysicalAddress(uint32_t* BaseAddress)
{
	xbox::physical_address_xt pa;
	pa = static_cast<ULONG>(g_memory.MapVirtual(BaseAddress)); // Returns 32-bit offset
	LOG_UTILITY("MmGetPhysicalAddress(%p) -> 0x%08X", BaseAddress, pa);
	return pa;
}

LONG MmLockUnlockBufferPages(uint32_t* BaseAddress, uint32_t NumberOfBytes, BOOLEAN Lock)
{
	LOG_UTILITY("MmLockUnlockBufferPages(%p, %zu, %s)", BaseAddress, NumberOfBytes, Lock ? "Lock" : "Unlock");
	return X_STATUS_SUCCESS; // No-op in recompiler
}

LONG MmLockUnlockPhysicalPage(uint32_t PhysicalAddress_, BOOLEAN Lock)
{
	xbox::physical_address_xt PhysicalAddress = PhysicalAddress_;
	uint32_t addr = static_cast<uint32_t>(PhysicalAddress);
	LOG_UTILITY("MmLockUnlockPhysicalPage(0x%08X, %s)", addr, Lock ? "Lock" : "Unlock");
	return X_STATUS_SUCCESS;
}

VOID MmPersistContiguousMemory(uint32_t* BaseAddress, uint32_t NumberOfBytes, BOOLEAN Persist)
{
	LOG_UTILITY("MmPersistContiguousMemory(%p, %zu, %s\n)", BaseAddress, NumberOfBytes, Persist ? "Persist" : "Unpersist");
}

LONG MmQueryAddressProtect(uint32_t* BaseAddress, uint32_t* Protect)
{
	LOG_UTILITY("MmQueryAddressProtect");
	*Protect = PAGE_READWRITE;
	return X_STATUS_SUCCESS;
}

SIZE_T MmQueryAllocationSize(uint32_t* BaseAddress)
{
	return g_userHeap.Size(&BaseAddress);
}

LONG MmSetAddressProtect(uint32_t* BaseAddress, SIZE_T NumberOfBytes, ULONG NewProtect)
{
	LOG_UTILITY("MmSetAddressProtect(%p, %zu, 0x%X)", &BaseAddress, NumberOfBytes, NewProtect);
	return X_STATUS_SUCCESS;
}

LONG NtWaitForSingleObject(uint32_t* Handle, BOOLEAN Alertable, uint32_t* Timeout)
{
	LOG_UTILITY("NtWaitForSingleObject\n");
	return X_STATUS_SUCCESS; // Pretend the object is always signaled
}

LONG NtSetEvent(uint32_t* Handle, uint32_t* PreviousState)
{
	LOG_UTILITY("NtSetEvent\n");
	if (PreviousState)
		*PreviousState = 1;

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

	LOG_UTILITY("NtWriteFile\n");
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
	LOG_UTILITY("RtlLONGToDosError STUB\n");
	return X_STATUS_SUCCESS;
}

void NtYieldExecution() {
	std::this_thread::yield();
}

SIZE_T RtlCompareMemoryUlong(uint32_t* Buffer, uint32_t Length, ULONG Pattern)
{
	LOG_UTILITY("RtlCompareMemoryUlong Pretend bytes match\n");
	return Length; // Pretend all bytes match
}

BOOLEAN RtlEqualString(PCSTR String1, PCSTR String2)
{
	LOG_UTILITY("RtlEqualString\n");
	return strcmp(String1, String2) == 0;
}

LONG NtSuspendThread(GuestThreadHandle* ThreadHandle, uint32_t* PreviousSuspendCount)
{
	LOG_UTILITY("NtSuspendThread(0x%X)\n", ThreadHandle);
	assert(ThreadHandle != GetKernelObject(CURRENT_THREAD_HANDLE) && ThreadHandle->thread.get_id() == std::this_thread::get_id());

	ThreadHandle->suspended = true;
	ThreadHandle->suspended.wait(true);

	return S_OK;
}

LONG NtResumeThread(GuestThreadHandle* ThreadHandle, uint32_t* PreviousSuspendCount)
{

	LOG_UTILITY("NtResumeThread(0x%X)\n", ThreadHandle);
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


void KeInitializeThread(
	IN OUT uint32_t Thread,
	IN uint32_t KernelStack,
	IN ulong_xt KernelStackSize,
	IN ulong_xt TlsDataSize
)
{
	/* ReactOS's KeInitThread inline code begin */

	/* Initialize the Dispatcher Header */
	*(uint32_t*)(Thread + 0) = xbox::ThreadObject;
	*(uint32_t*)(Thread + 0 + 2) = 272 / sizeof(xbox::long_xt);
	// ThreadControlFlags
	// DebugActive

	/* Set the Thread Stacks */
	*(uint32_t*)(Thread + 0x1c) = KernelStack;
	*(uint32_t*)(Thread  + 0x20) = (reinterpret_cast<ulong_ptr_xt>(KernelStack) - KernelStackSize);

}

LONG PsCreateSystemThreadEx(
	OUT uint32_t*         ThreadHandle,
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

LONG ObReferenceObjectByHandle(
	uint32_t* Handle,
	uint32_t* ObjectType,
	uint32_t* Object
) {
	*Object = reinterpret_cast<uint32_t>(0xDEADBEEF); // Dummy object
	return X_STATUS_SUCCESS;
}

void ObfDereferenceObject(PVOID Object) {
	// No-op
}

void RtlInitializeCriticalSection(uint32_t* CritSec)
{
	LOG_UTILITY("RtlInitializeCriticalSection STUB\n");
}

void RtlEnterCriticalSection(uint32_t* CritSec) {
	LOG_UTILITY("RtlEnterCriticalSection STUB\n");
}

void RtlLeaveCriticalSection(uint32_t* CritSec) {
	LOG_UTILITY("RtlLeaveCriticalSection STUB\n");
}

void RtlInitAnsiString(uint32_t* Destination_, char* Source)
{
	/**xbox::PANSI_STRING Destination = (xbox::PANSI_STRING)Destination_;
	Destination->Buffer = const_cast<PCHAR>(Source);
	Destination->Length = static_cast<USHORT>(strlen(Source));
	Destination->MaximumLength = Destination->Length + 1;*/
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
	LOG_UTILITY("[STUB] HalInitiateShutdown() - Ignoring");
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
	LOG_UTILITY("[STUB] RtlRaiseException() - Crashing intentionally");
	std::terminate();
}

void RtlUnwind_()
{
	LOG_UTILITY("[STUB] RtlUnwind() - Ignoring");
}

uint32_t RtlNtStatusToDosError(xbox::ntstatus_xt status)
{
	if (!status || (status & 0x20000000))
		return TRUE;

	return TRUE;
}

LONG NtWaitForSingleObjectEx(
	uint32_t* Handle,
	BOOLEAN Alertable,
	uint32_t* Timeout,
	DWORD WaitMode
) {
	return NtWaitForSingleObject(Handle, Alertable, Timeout); // Reuse stub
}




void sub_2B126A_r()
{
	LOG_UTILITY("[STUB] sub_2B126A_r() - Ignoring");
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
GUEST_FUNCTION_HOOK(__imp__NtSetInformationFile, NtSetInformationFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__ExQueryNonVolatileSetting, ExQueryNonVolatileSetting);
GUEST_FUNCTION_HOOK(__imp__HalReturnToFirmware, HalReturnToFirmware);
GUEST_FUNCTION_HOOK(__imp__MmFreeContiguousMemory, MmFreeContiguousMemory);
GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlNtStatusToDosError, RtlNtStatusToDosError);
GUEST_FUNCTION_HOOK_STDCALL(__imp__FscSetCacheSize, FscSetCacheSize);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtQueryDirectoryFile, NtQueryDirectoryFile);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtCreateEvent, NtCreateEvent);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtSetEvent, NtSetEvent);
GUEST_FUNCTION_HOOK_STDCALL(__imp__NtWaitForSingleObjectEx, NtWaitForSingleObjectEx);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeDelayExecutionThread, KeDelayExecutionThread)
GUEST_FUNCTION_HOOK_STDCALL(__imp__ObfDereferenceObject, ObfDereferenceObject);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeSetBasePriorityThread, KeSetBasePriorityThread);
GUEST_FUNCTION_HOOK_STDCALL(__imp__ObReferenceObjectByHandle, ObReferenceObjectByHandle);
GUEST_FUNCTION_HOOK_STDCALL(__imp__PsCreateSystemThreadEx, PsCreateSystemThreadEx);
GUEST_FUNCTION_HOOK(__imp__KeQueryBasePriorityThread, KeQueryBasePriorityThread);
GUEST_FUNCTION_HOOK(__imp__KeSetDisableBoostThread, KeSetDisableBoostThread);
GUEST_FUNCTION_HOOK(__imp__NtResumeThread, NtResumeThread);
GUEST_FUNCTION_HOOK(__imp__NtSuspendThread, NtSuspendThread);
GUEST_FUNCTION_HOOK(__imp__NtYieldExecution, NtYieldExecution);
GUEST_FUNCTION_HOOK(__imp__PsTerminateSystemThread, PsTerminateSystemThread);
GUEST_FUNCTION_HOOK(__imp__NtWaitForSingleObject, NtWaitForSingleObject);
GUEST_FUNCTION_HOOK(__imp__NtReadFile, NtReadFile);
GUEST_FUNCTION_HOOK(__imp__NtWriteFile, NtWriteFile);
GUEST_FUNCTION_HOOK(__imp__NtQueryInformationFile, NtQueryInformationFile);
GUEST_FUNCTION_HOOK(__imp__NtQueryVolumeInformationFile, NtQueryVolumeInformationFile);
GUEST_FUNCTION_HOOK(__imp__NtFlushBuffersFile, NtFlushBuffersFile);
GUEST_FUNCTION_HOOK(__imp__NtFsControlFile, NtFsControlFile);
GUEST_FUNCTION_HOOK(__imp__NtDeviceIoControlFile, NtDeviceIoControlFile);
GUEST_FUNCTION_HOOK(__imp__XcSHAInit, XcSHAInit);
GUEST_FUNCTION_HOOK(__imp__XcSHAUpdate, XcSHAUpdate);
GUEST_FUNCTION_HOOK(__imp__XcSHAFinal, XcSHAFinal);
GUEST_FUNCTION_HOOK(__imp__KeQuerySystemTime, KeQuerySystemTime);

GUEST_FUNCTION_HOOK_STDCALL(__imp__NtCreateFile, NtCreateFile); //get value hook error
GUEST_FUNCTION_HOOK(__imp__IoCreateSymbolicLink, IoCreateSymbolicLink);
GUEST_FUNCTION_HOOK_STDCALL(__imp__IoDeleteSymbolicLink, IoDeleteSymbolicLink);
GUEST_FUNCTION_HOOK(__imp__NtQuerySymbolicLinkObject, NtQuerySymbolicLinkObject);
GUEST_FUNCTION_HOOK(__imp__NtOpenSymbolicLinkObject, NtOpenSymbolicLinkObject);
GUEST_FUNCTION_HOOK(__imp__MmFreeSystemMemory, MmFreeSystemMemory);

GUEST_FUNCTION_HOOK(__imp__MmAllocateSystemMemory, MmAllocateSystemMemory);
GUEST_FUNCTION_HOOK(__imp__KfLowerIrql, KfLowerIrql); //todo implement thiscall

GUEST_FUNCTION_HOOK(__imp__KeRaiseIrqlToDpcLevel, KeRaiseIrqlToDpcLevel);
GUEST_FUNCTION_HOOK_STDCALL(__imp__HalRegisterShutdownNotification, HalRegisterShutdownNotification);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeInitializeTimerEx, KeInitializeTimerEx);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeSetTimer, KeSetTimer);
GUEST_FUNCTION_HOOK_STDCALL(__imp__KeCancelTimer, KeCancelTimer);


GUEST_FUNCTION_HOOK_STDCALL(__imp__RtlLONGToDosError, RtlLONGToDosError);
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
GUEST_FUNCTION_HOOK_STDCALL(__imp__KfRaiseIrql, KfRaiseIrql);
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



X86_FUNC_IMPL(__imp__sub_11AD0);
X86_FUNC(sub_11AD0)
{
	LOG_UTILITY("PreMainLauncher\n");
	__imp__sub_11AD0(ctx, base);
}


X86_FUNC_IMPL(__imp__sub_2AD4E4);
X86_FUNC(sub_2AD4E4)
{
	LOG_UTILITY("XLaunchNewImageA\n");
	__imp__sub_2AD4E4(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_11430);
X86_FUNC(sub_11430)
{
	LOG_UTILITY("RsRwInitialize\n");
	__imp__sub_11430(ctx, base);
}

//XAutoPowerDownResetTimer
GUEST_FUNCTION_STUB(sub_2B17E8);

int CreateThread_r(int ignoreHandle, int ThreadHandle, int startRoutine, int startContext, unsigned int a5, int threadID)
{
	return 1;

}


void LogStackValues(const X86Context& ctx, uint8_t* base, size_t count) 
{
	/**printf("Stack values at ESP=%08X:\n", ctx.esp.u32);
	for (size_t i = 0; i < count; i++) {
		uint32_t addr = ctx.esp.u32 + 4 + (i * 4);
		uint32_t value = *reinterpret_cast<uint32_t*>(base + addr);
		printf("  [ESP+%02X] = %08X\n", 4 + (i * 4), value);
	}*/
}

int XAPILIBCloseHandle(uint32_t Handle)
{

	auto v1 = NtClose((uint32_t)Handle);
	if (v1 >= 0)
		return 1;
	//XapiSetLastNTError(v1);
	return 0;
}

//GUEST_FUNCTION_HOOK(sub_2AD8B2, XAPILIBCloseHandle);
//GUEST_FUNCTION_HOOK(sub_2ADF0B, CreateThread_r);
