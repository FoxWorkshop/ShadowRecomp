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
	LOG_UTILITY("!!! STUB !!!");
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


int32_t NtFreeVirtualMemory(IN OUT PVOID* BaseAddress, IN OUT PULONG FreeSize, IN uint32_t FreeType)
{
	g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
	return X_STATUS_SUCCESS;
}

int32_t NtAllocateVirtualMemory(PHANDLE ProcessHandle, PVOID* BaseAddress,
	ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType, ULONG Protect)
{
	void* ptr = g_userHeap.Alloc(*RegionSize);

	uint32_t xbox_addr = g_memory.MapVirtual(ptr);
	*BaseAddress = reinterpret_cast<PVOID>(xbox_addr);

	printf("NtAllocateVirtualMemory: %zu bytes -> 0x%08X", *RegionSize, xbox_addr);
	return X_STATUS_SUCCESS;
}

void* AvSavedDataAddress;
void AvSetSavedDataAddress(IN  PVOID   Address)
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


typedef enum {
	NV_SETTING_VIDEO_MODE = 0,
	NV_SETTING_LANGUAGE,
} NV_SETTING_TYPE;

LONG ExQueryNonVolatileSetting(
	uint32_t SettingType,
	void* OutputBuffer,
	size_t OutputSize,
	size_t* BytesWritten
) {
	printf("[STUB] ExQueryNonVolatileSetting(%d, %p, %zu, %p)\n",
		SettingType, OutputBuffer, OutputSize, BytesWritten);

	// Default values for common settings
	switch (SettingType) {
	case NV_SETTING_VIDEO_MODE:
		*(DWORD*)OutputBuffer = 0x00010000; // 480p default
		break;
	case NV_SETTING_LANGUAGE:
		*(DWORD*)OutputBuffer = 1; // English
		break;
	default:
		printf("  Unknown setting type, returning 0\n");
		memset(OutputBuffer, 0, OutputSize);
	}

	if (BytesWritten) *BytesWritten = OutputSize;
	return X_STATUS_SUCCESS;
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

void HalRegisterShutdownNotification
(
	IN int ShutdownRegistration,
	IN std::uint8_t Register
)
{
	printf("[STUB] HalRegisterShutdownNotification(%p, %d)\n",
		ShutdownRegistration, Register);
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
	IN PVOID   Buffer,
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

LONG IoCreateSymbolicLink(void* SymbolicLinkName_, void* DeviceName_)
{
	xbox::PUNICODE_STRING DeviceName = (xbox::PUNICODE_STRING)DeviceName_;	
	xbox::PUNICODE_STRING SymbolicLinkName = (xbox::PUNICODE_STRING)SymbolicLinkName_;

	printf("[STUB] IoCreateSymbolicLink(%.*S -> %.*S)\n",
		SymbolicLinkName->Length / 2, SymbolicLinkName->Buffer,
		DeviceName->Length / 2, DeviceName->Buffer);

	return X_STATUS_SUCCESS;
}

LONG IoDeleteSymbolicLink(void* SymbolicLinkName_)
{
	xbox::PSTRING SymbolicLinkName = (xbox::PSTRING)SymbolicLinkName_;
	printf("[STUB] IoDeleteSymbolicLink(%.*S)\n",
		SymbolicLinkName->Length / 2, SymbolicLinkName->Buffer);

	return X_STATUS_SUCCESS;
}

int32_t KeBugCheckEx(IN std::uint32_t BugCheckCode, IN PVOID BugCheckParameter1, IN PVOID BugCheckParameter2, IN PVOID BugCheckParameter3, IN PVOID BugCheckParameter4)
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

void KeQuerySystemTime(void* CurrentTime_)
{
	PLARGE_INTEGER CurrentTime = (PLARGE_INTEGER)CurrentTime_;
	static LONGLONG fakeTime = 0;
	fakeTime += 10000000; // Increment by 1 second

	CurrentTime->QuadPart = fakeTime;
	printf("[STUB] KeQuerySystemTime() -> %lld\n", CurrentTime->QuadPart);
}

LONG KeQueryBasePriorityThread(IN void* Thread)
{
	xbox::PKTHREAD t = (xbox::PKTHREAD)Thread;
	printf("[STUB] KeQueryBasePriorityThread(%p)\n", t);
	return 8;
}

std::uint8_t KeInsertQueueDpc(void* Dpc_, PVOID Argument1, PVOID Argument2)
{
	xbox::PKDPC Dpc = (xbox::PKDPC)Dpc_;
	printf("[STUB] KeInsertQueueDpc(%p, %p, %p)\n", Dpc, Argument1, Argument2);
	return TRUE;
}

#define IRQ_BASE 0x30 
#define VECTOR2IRQ(vector)  ((vector)-IRQ_BASE)
void KeInitializeInterrupt(void* Interrupt_, void* ServiceRoutine_,
	PVOID ServiceContext, ULONG Vector, std::uint8_t Irql,
	uint32_t InterruptMode_, BOOLEAN ShareVector)
{
	xbox::KINTERRUPT_MODE InterruptMode = (xbox::KINTERRUPT_MODE)InterruptMode_;
	xbox::PKSERVICE_ROUTINE ServiceRoutine = (xbox::PKSERVICE_ROUTINE)ServiceRoutine_;
	xbox::PKINTERRUPT Interrupt = (xbox::PKINTERRUPT)Interrupt_;
	Interrupt->ServiceRoutine = ServiceRoutine;
	Interrupt->ServiceContext = ServiceContext;
	Interrupt->BusInterruptLevel = VECTOR2IRQ(Vector);
	Interrupt->Irql = Irql;
	Interrupt->Connected = FALSE;
	Interrupt->Mode = InterruptMode;

	printf("[STUB] KeInitializeInterrupt(Vector 0x%X, Irql %d)\n", Vector, Irql);
}

void KeDisconnectInterrupt(void* Interrupt_)
{
	xbox::PKINTERRUPT Interrupt = (xbox::PKINTERRUPT)Interrupt;
	printf("[STUB] KeDisconnectInterrupt(%p)\n", Interrupt);
}

LONG KeDelayExecutionThread(uint8_t WaitMode, BOOLEAN Alertable, PLARGE_INTEGER Interval)
{

	DWORD ms = Interval->QuadPart / -10000;
	printf("[STUB] KeDelayExecutionThread(%d ms)\n", ms);

	//Actual sleep would go here
	// Sleep(ms);
	return X_STATUS_SUCCESS;
}

std::uint8_t KeConnectInterrupt(void* Interrupt_)
{
	xbox::PKINTERRUPT Interrupt = (xbox::PKINTERRUPT)Interrupt;
	printf("[STUB] KeConnectInterrupt(%p)\n", Interrupt);
	return X_STATUS_SUCCESS;
}

std::uint8_t KeCancelTimer(void* Timer_)
{
	xbox::PKTIMER Timer = (xbox::PKTIMER)Timer_;
	printf("[STUB] KeCancelTimer(%p)\n", Timer);
	return TRUE;
}

VOID KeInitializeDpc(void* Dpc_,
	void* DeferredRoutine_,
	PVOID DeferredContext
) {
	xbox::PKDPC Dpc = (xbox::PKDPC)Dpc_;
	xbox::PKDEFERRED_ROUTINE DeferredRoutine = (xbox::PKDEFERRED_ROUTINE)DeferredRoutine_;
	printf("[STUB] KeInitializeDpc(%p, %p, %p)\n",
		Dpc, DeferredRoutine, DeferredContext);
	Dpc->Type = 19;
	Dpc->Inserted = FALSE;
	Dpc->DeferredRoutine = DeferredRoutine;
	Dpc->DeferredContext = DeferredContext;
}

void InitializeListHead(void* pListHead_)
{
	xbox::PLIST_ENTRY pListHead = (xbox::PLIST_ENTRY)pListHead_;
	pListHead->Flink = pListHead->Blink = pListHead;
}

VOID KeInitializeTimerEx(void* Timer_, uint32_t Type_)
{
	xbox::PKTIMER Timer = (xbox::PKTIMER)Timer_;
	xbox::TIMER_TYPE Type = (xbox::TIMER_TYPE)Type_;
	printf("[STUB] KeInitializeTimerEx(%p, Type %d)\n", Timer, Type);
	assert(Timer);

	// Initialize header :
	Timer->Header.Type = Type + xbox::TimerNotificationObject;
	Timer->Header.Inserted = FALSE;
	Timer->Header.Size = sizeof(xbox::KTIMER) / sizeof(ULONG);
	Timer->Header.SignalState = 0;
	InitializeListHead(&(Timer->Header.WaitListHead));
	// Initialize specific fields :
	Timer->TimerListEntry.Blink = NULL;
	Timer->TimerListEntry.Flink = NULL;
	Timer->DueTime.QuadPart = 0;
	Timer->Period = 0;
}

std::uint8_t KeRemoveQueueDpc(void* Dpc_)
{
	xbox::PKDPC Dpc = (xbox::PKDPC)Dpc_;
	printf("[STUB] KeRemoveQueueDpc(%p)\n", Dpc);
	return TRUE;
}

// In a real implementation, would restore FPU/XMM registers
int32_t KeRestoreFloatingPointState(void* SaveArea_)
{
	xbox::PKFLOATING_SAVE SaveArea = (xbox::PKFLOATING_SAVE)SaveArea_;
	printf("[STUB] KeRestoreFloatingPointState(%p)\n", SaveArea);
	LONG ret = X_STATUS_SUCCESS;
	return ret;
}

LONG KeSaveFloatingPointState(void* SaveArea_)
{
	xbox::PKFLOATING_SAVE SaveArea = (xbox::PKFLOATING_SAVE)SaveArea_;
	printf("[STUB] KeSaveFloatingPointState(%p)\n", SaveArea);
	return X_STATUS_SUCCESS;
}

std::int32_t KeSetBasePriorityThread(void* Thread_, LONG Priority)
{
	xbox::PKTHREAD Thread = (xbox::PKTHREAD)Thread_;
	Thread->Priority = Priority;
	std::int32_t ret = Thread->Priority;
	printf("[STUB] KeSetBasePriorityThread(%p, %ld)\n", Thread, Priority);
	return ret;
}

std::uint8_t KeSetDisableBoostThread(void* Thread_, BOOLEAN Disable)
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

BOOLEAN KeSetTimer(void* Timer_, void* DueTime_, void* Dpc_)
{
	_LARGE_INTEGER DueTime = *(_LARGE_INTEGER*)DueTime_;
	if (DueTime_)
		DueTime = *(_LARGE_INTEGER*)DueTime_;
	else
		DueTime.QuadPart = 0;

	xbox::PKTIMER Timer = (xbox::PKTIMER)Timer_;
	xbox::PKDPC Dpc = (xbox::PKDPC)Dpc_;
	printf("[STUB] KeSetTimer(%p, %lld ns, Dpc %p)\n", Timer, DueTime.QuadPart, Dpc);
	return FALSE; // Timer wasn't already active
}

VOID KeStallExecutionProcessor(ULONG Microseconds)
{
	printf("[STUB] KeStallExecutionProcessor(%u)\n", Microseconds);
}

BOOLEAN KeSynchronizeExecution(void* Interrupt_, void* SynchronizeRoutine_, PVOID SynchronizeContext)
{
	xbox::PKSYNCHRONIZE_ROUTINE SynchronizeRoutine = (xbox::PKSYNCHRONIZE_ROUTINE)SynchronizeRoutine_;
	xbox::PKINTERRUPT Interrupt = (xbox::PKINTERRUPT)Interrupt_;
	printf("[STUB] KeSynchronizeExecution(Int %p, Routine %p, Ctx %p)\n", Interrupt, SynchronizeRoutine, SynchronizeContext);
	return SynchronizeRoutine(SynchronizeContext);
}

LONG KeWaitForSingleObject(PVOID Object, uint32_t WaitReason_, uint8_t WaitMode, BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
) {
	xbox::KWAIT_REASON WaitReason = (xbox::KWAIT_REASON)WaitReason_;
	printf("[STUB] KeWaitForSingleObject(%p, Reason %d, %s, Timeout %lld)\n",
		Object, WaitReason, Alertable ? "Alertable" : "Non-Alertable",
		Timeout ? Timeout->QuadPart : -1);

	return X_STATUS_SUCCESS; //Pretend object was signaled
}

std::uint8_t KfRaiseIrql(std::uint8_t NewIrql)
{
	std::uint8_t old = PASSIVE_LEVEL;
	printf("[STUB] KfRaiseIrql(%d) -> %d\n", NewIrql, old);
	return old; // Return previous IRQL
}

PVOID MmAllocateContiguousMemory(SIZE_T NumberOfBytes)
{
	printf("[STUB] MmAllocateContiguousMemory(%zu bytes)\n", NumberOfBytes);
	return reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, GPU_ALIGNMENT)));
}

PVOID MmAllocateContiguousMemoryEx(uint32_t NumberOfBytes, uint32_t LowestAcceptableAddress,
	uint32_t HighestAcceptableAddress, uint32_t Alignment,
	uint32_t ProtectionType
)
{
	printf("[STUB] MmAllocateContiguousMemoryEx(%zu bytes, Align %u)\n",
		NumberOfBytes, Alignment);
	return reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, Alignment)));
}

PVOID MmClaimGpuInstanceMemory(SIZE_T NumberOfBytes, ULONG Protect
) {
	printf("[STUB] MmClaimGpuInstanceMemory(%zu bytes, Prot 0x%X)\n",
		NumberOfBytes, Protect);

	return reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, GPU_ALIGNMENT)));
}

VOID MmFreeContiguousMemory(PVOID BaseAddress)
{
	printf("[STUB] MmFreeContiguousMemory(%p)\n", BaseAddress);
	g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
}

VOID MmFreeSystemMemory(PVOID BaseAddress, SIZE_T NumberOfBytes)
{
	printf("[STUB] MmFreeSystemMemory(%p, %zu bytes)\n",
		BaseAddress, NumberOfBytes);

	g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
}

PVOID MmAllocateSystemMemory(SIZE_T NumberOfBytes, ULONG Protect)
{
	printf("[STUB] MmAllocateSystemMemory(%zu bytes, Prot 0x%X)\n",
		NumberOfBytes, Protect);
	return (void*)g_memory.MapVirtual(g_userHeap.Alloc(NumberOfBytes));
}

void KfLowerIrql(std::uint8_t NewIrql)
{
	printf("[STUB] KfLowerIrql(%d)\n", NewIrql);
}


LONG NtClose(HANDLE Handle)
{
	printf("NtClose(%p)", Handle);
	return X_STATUS_SUCCESS;
}

LONG NtCreateEvent(PHANDLE EventHandle, void*  bjectAttributes, uint32_t EventType, std::uint8_t InitialState) 
{
	*EventHandle = reinterpret_cast<HANDLE>(0xDEADBEEF); // Dummy handle
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
	OUT PHANDLE             FileHandle,
	IN  uint32_t         DesiredAccess,
	IN  void*  ObjectAttributes_,
	OUT void*  IoStatusBlock_,
	IN  void*      AllocationSize_ OPTIONAL,
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

	*FileHandle = reinterpret_cast<HANDLE>(host_handle);


	if (IoStatusBlock) {
		IoStatusBlock->Status = X_STATUS_SUCCESS;
		IoStatusBlock->Information = 0;
	}
}

LONG NtDeviceIoControlFile(
	HANDLE FileHandle,
	HANDLE Event,
	void* ApcRoutine,
	PVOID ApcContext,
	void* IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength
) {
	LOG_UTILITY("NtDeviceIoControlFile(0x%X)", IoControlCode);
	return X_STATUS_SUCCESS;
}

LONG NtFlushBuffersFile(HANDLE FileHandle, void* IoStatusBlock)
{
#ifdef _WIN32
	FlushFileBuffers(FileHandle);
#else
	fsync(reinterpret_cast<int>(FileHandle));
#endif
	return X_STATUS_SUCCESS;
}

LONG NtFsControlFile(
	HANDLE FileHandle,
	HANDLE Event,
	void* ApcRoutine,
	PVOID ApcContext,
	void* IoStatusBlock,
	ULONG FsControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength
) {
	LOG_UTILITY("NtFsControlFile(0x%X)", FsControlCode);
	return X_STATUS_SUCCESS;
}

LONG NtOpenFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	void* ObjectAttributes_,
	void* IoStatusBlock_,
	ULONG ShareAccess,
	ULONG OpenOptions
) {

	xbox::POBJECT_ATTRIBUTES ObjectAttributes = (xbox::POBJECT_ATTRIBUTES)ObjectAttributes_;
	xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
	// Reuse NtCreateFile logic
	_LARGE_INTEGER t;
	memset(&t, 0, sizeof(_LARGE_INTEGER));
	return NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock, (void*)&t, ShareAccess, FILE_OPEN, OpenOptions, NULL);
}

LONG NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle, IN void* ObjectAttributes
) {
	*LinkHandle = reinterpret_cast<HANDLE>(0xCAFEBABE);
	return X_STATUS_SUCCESS;
}

LONG NtQueryDirectoryFile(
	IN  HANDLE FileHandle,
	IN  HANDLE Event OPTIONAL,
	IN  PVOID ApcRoutine, // Todo: define this routine's prototype
	IN  PVOID ApcContext,
	OUT void* IoStatusBlock,
	OUT uint32_t* FileInformation_,
	IN  uint32_t Length,
	IN  uint32_t FileInformationClass,
	IN  xbox::PSTRING FileMask,
	IN  std::uint8_t RestartScan
)
{
	xbox::FILE_INFORMATION_CLASS fileInfo = (xbox::FILE_INFORMATION_CLASS)FileInformationClass;
	xbox::_FILE_DIRECTORY_INFORMATION FileInformation = (xbox::_FILE_DIRECTORY_INFORMATION)*FileInformation_;
	//memset(FileInformation, 0, Length);
	return X_STATUS_SUCCESS;
}

LONG NtQueryFullAttributesFile(void* ObjectAttributes_, void* FileInformation_
) 
{
	xbox::POBJECT_ATTRIBUTES ObjectAttributes = (xbox::POBJECT_ATTRIBUTES)ObjectAttributes_;
	xbox::PFILE_NETWORK_OPEN_INFORMATION FileInformation = (xbox::PFILE_NETWORK_OPEN_INFORMATION)ObjectAttributes_;
	FileInformation->EndOfFile.QuadPart = 0x1000; // Dummy size
	return X_STATUS_SUCCESS;
}

LONG NtQueryInformationFile(
	HANDLE FileHandle,
	void* IoStatusBlock_,
	PVOID FileInformation,
	ULONG FileInformationLength,
	uint32_t FileInformationClass_
) 
{
	
	xbox::FILE_INFORMATION_CLASS FileInformationClass = (xbox::FILE_INFORMATION_CLASS)FileInformationClass_;
	xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;

	if (FileInformationClass == xbox::FileStandardInformation)
	{
		auto* info = reinterpret_cast<xbox::FILE_STANDARD_INFORMATION*>(FileInformation);
		info->EndOfFile.QuadPart = 0x1000; // Dummy 4KB file
	}
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

LONG NtQueryVirtualMemory(PVOID BaseAddress, PMEMORY_BASIC_INFORMATION Buffer) 
{

	if (!Buffer)
		return X_STATUS_UNSUCCESSFUL;

	memset(Buffer, 0, sizeof(MEMORY_BASIC_INFORMATION));

	if (!g_memory.IsInMemoryRange(BaseAddress))
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
	Buffer->Type = MEM_PRIVATE;

	return X_STATUS_SUCCESS;
}


xbox::physical_address_xt MmGetPhysicalAddress(PVOID BaseAddress) {
	xbox::physical_address_xt pa;
	pa = static_cast<ULONG>(g_memory.MapVirtual(BaseAddress)); // Returns 32-bit offset
	LOG_UTILITY("MmGetPhysicalAddress(%p) -> 0x%08X", BaseAddress, pa);
	return pa;
}

LONG MmLockUnlockBufferPages(PVOID BaseAddress, SIZE_T NumberOfBytes, BOOLEAN Lock)
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

VOID MmPersistContiguousMemory(PVOID BaseAddress, SIZE_T NumberOfBytes, BOOLEAN Persist) 
{
	LOG_UTILITY("MmPersistContiguousMemory(%p, %zu, %s)", BaseAddress, NumberOfBytes, Persist ? "Persist" : "Unpersist");
}

LONG MmQueryAddressProtect(PVOID BaseAddress, PULONG Protect)
{
	*Protect = PAGE_READWRITE;
	return X_STATUS_SUCCESS;
}

SIZE_T MmQueryAllocationSize(PVOID BaseAddress)
{
	return g_userHeap.Size(BaseAddress);
}

LONG MmSetAddressProtect(PVOID BaseAddress, SIZE_T NumberOfBytes, ULONG NewProtect)
{
	LOG_UTILITY("MmSetAddressProtect(%p, %zu, 0x%X)", BaseAddress, NumberOfBytes, NewProtect);
	return X_STATUS_SUCCESS;
}

LONG NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
	return X_STATUS_SUCCESS; // Pretend the object is always signaled
}

LONG NtSetEvent(HANDLE Handle, PULONG PreviousState)
{
	if (PreviousState)
		*PreviousState = 1;

	return X_STATUS_SUCCESS;
}

LONG NtWriteFile(
	HANDLE FileHandle,
	HANDLE Event,
	void* ApcRoutine_,
	PVOID ApcContext,
	void* IoStatusBlock_,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset
) {

	xbox::PIO_APC_ROUTINE Apcroutine = (xbox::PIO_APC_ROUTINE)ApcRoutine_;
	xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
	if (IoStatusBlock) 
	{
		IoStatusBlock->Status = X_STATUS_SUCCESS;
		IoStatusBlock->Information = Length; // Pretend all bytes were written
	}
	return X_STATUS_SUCCESS;
}

ULONG RtlLONGToDosError(LONG Status)
{
	return X_STATUS_SUCCESS;
}

void NtYieldExecution() {
	std::this_thread::yield();
}

SIZE_T RtlCompareMemoryUlong(PVOID Buffer, SIZE_T Length, ULONG Pattern)
{
	return Length; // Pretend all bytes match
}

BOOLEAN RtlEqualString(PCSTR String1, PCSTR String2)
{
	return strcmp(String1, String2) == 0;
}

LONG NtSuspendThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount)
{
	if (PreviousSuspendCount)
		*PreviousSuspendCount = 0;

	return X_STATUS_SUCCESS;
}

LONG NtResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount)
{
	if (PreviousSuspendCount)
		*PreviousSuspendCount = 0;

	return X_STATUS_SUCCESS;
}

LONG PsCreateSystemThreadEx(
	PHANDLE ThreadHandle,
	ULONG Flags,
	SIZE_T StackSize,
	SIZE_T Reserved,
	PVOID StartContext
) {
	//*ThreadHandle = reinterpret_cast<HANDLE>(0x12345678); // Dummy handle
	return X_STATUS_SUCCESS;
}

void PsTerminateSystemThread(LONG ExitStatus)
{
	// No-op (thread cleanup is handled by the recompiler)
}

LONG ObReferenceObjectByHandle(
	HANDLE Handle,
	PVOID ObjectType,
	PVOID* Object
) {
	*Object = reinterpret_cast<PVOID>(0xDEADBEEF); // Dummy object
	return X_STATUS_SUCCESS;
}

void ObfDereferenceObject(PVOID Object) {
	// No-op
}

void RtlInitializeCriticalSection(void* CritSec) {

}

void RtlEnterCriticalSection(void* CritSec) {

}

void RtlLeaveCriticalSection(void* CritSec) {

}

void RtlInitAnsiString(void* Destination_, char* Source)
{
	xbox::PANSI_STRING Destination = (xbox::PANSI_STRING)Destination_;
	Destination->Buffer = const_cast<PCHAR>(Source);
	Destination->Length = static_cast<USHORT>(strlen(Source));
	Destination->MaximumLength = Destination->Length + 1;
}

LONG NtQueryVolumeInformationFile(
	HANDLE FileHandle,
	void* IoStatusBlock_,
	PVOID FsInformation,
	ULONG Length,
	uint32_t FsInformationClass_
) 
{
	xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
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
	}

	return X_STATUS_SUCCESS;
}

LONG NtSetInformationFile(
	HANDLE FileHandle,
	void* IoStatusBlock_,
	PVOID FileInformation,
	ULONG Length,
	void* FileInformationClass
) 
{

	xbox::PIO_STATUS_BLOCK IoStatusBlock = (xbox::PIO_STATUS_BLOCK)IoStatusBlock_;
	//xbox::FILE_INFORMATION_CLASS fileInfo = (xbox::FILE_INFORMATION_CLASS*)FileInformationClass;
	if (IoStatusBlock)
	{
		IoStatusBlock->Status = X_STATUS_SUCCESS;
		IoStatusBlock->Information = 0;
	}
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

void XcSHAInit(void* Context)
{
	memset(Context, 0, 20); //SHA-1 context size
}

void XcSHAUpdate(void* Context, const void* Data, ULONG Length)
{
	// No-op
}

void XcSHAFinal(void* Context, void* Digest)
{
	memset(Digest, 0xAA, 20); //Dummy SHA-1 hash
}

void XcHMAC(void* Key, ULONG KeySize, void* Data, ULONG DataSize, void* Digest)
{
	memset(Digest, 0xBB, 20);
}

void RtlRaiseException_(void* ExceptionRecord) 
{
	LOG_UTILITY("[STUB] RtlRaiseException() - Crashing intentionally");
	std::terminate();
}

void RtlUnwind_()
{
	LOG_UTILITY("[STUB] RtlUnwind() - Ignoring");
}

void RtlNtStatusToDosError()
{
	LOG_UTILITY("[STUB]");
}

LONG NtWaitForSingleObjectEx(
	HANDLE Handle,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout,
	DWORD WaitMode
) {
	return NtWaitForSingleObject(Handle, Alertable, Timeout); // Reuse stub
}


GUEST_FUNCTION_HOOK(__imp__RtlInitializeCriticalSection, RtlInitializeCriticalSection);
GUEST_FUNCTION_HOOK(__imp__RtlEnterCriticalSection, RtlEnterCriticalSection);
GUEST_FUNCTION_HOOK(__imp__RtlLeaveCriticalSection, RtlLeaveCriticalSection);
GUEST_FUNCTION_HOOK(__imp__RtlCompareMemoryUlong, RtlCompareMemoryUlong);
GUEST_FUNCTION_HOOK(__imp__NtFreeVirtualMemory, NtFreeVirtualMemory);
GUEST_FUNCTION_HOOK(__imp__NtAllocateVirtualMemory, NtAllocateVirtualMemory);
GUEST_FUNCTION_HOOK(__imp__NtQueryVirtualMemory, NtQueryVirtualMemory);
GUEST_FUNCTION_HOOK(__imp__RtlRaiseException, RtlRaiseException_);
GUEST_FUNCTION_HOOK(__imp__MmPersistContiguousMemory, MmPersistContiguousMemory);
GUEST_FUNCTION_HOOK(__imp__MmAllocateContiguousMemory, MmAllocateContiguousMemory);
GUEST_FUNCTION_HOOK(__imp__NtQueryFullAttributesFile, NtQueryFullAttributesFile);
GUEST_FUNCTION_HOOK(__imp__RtlInitAnsiString, RtlInitAnsiString);
GUEST_FUNCTION_HOOK(__imp__NtOpenFile, NtOpenFile);
GUEST_FUNCTION_HOOK(__imp__NtClose, NtClose);
GUEST_FUNCTION_HOOK(__imp__NtSetInformationFile, NtSetInformationFile);
GUEST_FUNCTION_HOOK(__imp__ExQueryNonVolatileSetting, ExQueryNonVolatileSetting);
GUEST_FUNCTION_HOOK(__imp__HalReturnToFirmware, HalReturnToFirmware);
GUEST_FUNCTION_HOOK(__imp__MmFreeContiguousMemory, MmFreeContiguousMemory);
GUEST_FUNCTION_HOOK(__imp__RtlNtStatusToDosError, RtlNtStatusToDosError);
GUEST_FUNCTION_HOOK(__imp__FscSetCacheSize, FscSetCacheSize);
GUEST_FUNCTION_HOOK(__imp__NtQueryDirectoryFile, NtQueryDirectoryFile);
GUEST_FUNCTION_HOOK(__imp__NtCreateEvent, NtCreateEvent);
GUEST_FUNCTION_HOOK(__imp__NtSetEvent, NtSetEvent);
GUEST_FUNCTION_HOOK(__imp__NtWaitForSingleObjectEx, NtWaitForSingleObjectEx);
GUEST_FUNCTION_HOOK(__imp__KeDelayExecutionThread, KeDelayExecutionThread)
GUEST_FUNCTION_HOOK(__imp__ObfDereferenceObject, ObfDereferenceObject);
GUEST_FUNCTION_HOOK(__imp__KeSetBasePriorityThread, KeSetBasePriorityThread);
GUEST_FUNCTION_HOOK(__imp__ObReferenceObjectByHandle, ObReferenceObjectByHandle);
GUEST_FUNCTION_HOOK(__imp__PsCreateSystemThreadEx, PsCreateSystemThreadEx);
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

GUEST_FUNCTION_HOOK(__imp__NtCreateFile, NtCreateFile); //get value hook error
GUEST_FUNCTION_HOOK(__imp__IoCreateSymbolicLink, IoCreateSymbolicLink);
GUEST_FUNCTION_HOOK(__imp__IoDeleteSymbolicLink, IoDeleteSymbolicLink);
GUEST_FUNCTION_HOOK(__imp__NtQuerySymbolicLinkObject, NtQuerySymbolicLinkObject);
GUEST_FUNCTION_HOOK(__imp__NtOpenSymbolicLinkObject, NtOpenSymbolicLinkObject);
GUEST_FUNCTION_HOOK(__imp__MmFreeSystemMemory, MmFreeSystemMemory);

GUEST_FUNCTION_HOOK(__imp__MmAllocateSystemMemory, MmAllocateSystemMemory);
GUEST_FUNCTION_HOOK(__imp__KfLowerIrql, KfLowerIrql);

GUEST_FUNCTION_HOOK(__imp__KeRaiseIrqlToDpcLevel, KeRaiseIrqlToDpcLevel);
GUEST_FUNCTION_HOOK(__imp__HalRegisterShutdownNotification, HalRegisterShutdownNotification);
GUEST_FUNCTION_HOOK(__imp__KeInitializeTimerEx, KeInitializeTimerEx);
GUEST_FUNCTION_HOOK(__imp__KeSetTimer, KeSetTimer);
GUEST_FUNCTION_HOOK(__imp__KeCancelTimer, KeCancelTimer);



GUEST_FUNCTION_HOOK(__imp__RtlLONGToDosError, RtlLONGToDosError);
GUEST_FUNCTION_HOOK(__imp__RtlUnwind, RtlUnwind_);
GUEST_FUNCTION_HOOK(__imp__XcHMAC, XcHMAC);
GUEST_FUNCTION_HOOK(__imp__XeLoadSection, XeLoadSection);
GUEST_FUNCTION_HOOK(__imp__XeUnloadSection, XeUnloadSection);
GUEST_FUNCTION_HOOK(__imp__HalInitiateShutdown, HalInitiateShutdown);
GUEST_FUNCTION_HOOK(__imp__RtlEqualString, RtlEqualString);
GUEST_FUNCTION_HOOK(__imp__MmSetAddressProtect, MmSetAddressProtect);
GUEST_FUNCTION_HOOK(__imp__MmQueryAllocationSize, MmQueryAllocationSize);
GUEST_FUNCTION_HOOK(__imp__MmQueryAddressProtect, MmQueryAddressProtect);

GUEST_FUNCTION_HOOK(__imp__MmLockUnlockPhysicalPage, MmLockUnlockPhysicalPage);
GUEST_FUNCTION_HOOK(__imp__MmLockUnlockBufferPages, MmLockUnlockBufferPages);
GUEST_FUNCTION_HOOK(__imp__MmGetPhysicalAddress, MmGetPhysicalAddress);
GUEST_FUNCTION_HOOK(__imp__KfRaiseIrql, KfRaiseIrql);
GUEST_FUNCTION_HOOK(__imp__KeWaitForSingleObject, KeWaitForSingleObject);
GUEST_FUNCTION_HOOK(__imp__KeSynchronizeExecution, KeSynchronizeExecution);
GUEST_FUNCTION_HOOK(__imp__MmClaimGpuInstanceMemory, MmClaimGpuInstanceMemory);
GUEST_FUNCTION_HOOK(__imp__MmAllocateContiguousMemoryEx, MmAllocateContiguousMemoryEx);
GUEST_FUNCTION_HOOK(__imp__KeStallExecutionProcessor, KeStallExecutionProcessor);
GUEST_FUNCTION_HOOK(__imp__KeSetEvent, KeSetEvent);
GUEST_FUNCTION_HOOK(__imp__KeSaveFloatingPointState, KeSaveFloatingPointState);
GUEST_FUNCTION_HOOK(__imp__KeRestoreFloatingPointState, KeRestoreFloatingPointState);

GUEST_FUNCTION_HOOK(__imp__KeRemoveQueueDpc, KeRemoveQueueDpc);
GUEST_FUNCTION_HOOK(__imp__KeInitializeDpc, KeInitializeDpc);
GUEST_FUNCTION_HOOK(__imp__KeConnectInterrupt, KeConnectInterrupt);
GUEST_FUNCTION_HOOK(__imp__KeDisconnectInterrupt, KeDisconnectInterrupt);
GUEST_FUNCTION_HOOK(__imp__KeInitializeInterrupt, KeInitializeInterrupt);
GUEST_FUNCTION_HOOK(__imp__KeInsertQueueDpc, KeInsertQueueDpc);
GUEST_FUNCTION_HOOK(__imp__KeBugCheck, KeBugCheck);
GUEST_FUNCTION_HOOK(__imp__ExQueryPoolBlockSize, ExQueryPoolBlockSize);
GUEST_FUNCTION_HOOK(__imp__HalReadWritePCISpace, HalReadWritePCISpace);
GUEST_FUNCTION_HOOK(__imp__HalGetInterruptVector, HalGetInterruptVector);
GUEST_FUNCTION_HOOK(__imp__ExAllocatePoolWithTag, ExAllocatePoolWithTag);

GUEST_FUNCTION_HOOK(__imp__ExFreePool, ExFreePool);
GUEST_FUNCTION_HOOK(__imp__AvGetSavedDataAddress, AvGetSavedDataAddress);
GUEST_FUNCTION_HOOK(__imp__AvSetSavedDataAddress, AvSetSavedDataAddress);
GUEST_FUNCTION_HOOK(__imp__AvSetDisplayMode, AvSetDisplayMode);
GUEST_FUNCTION_HOOK(__imp__AvSendTVEncoderOption, AvSendTVEncoderOption);
GUEST_FUNCTION_HOOK(__imp__DbgPrint, DbgPrintV);

