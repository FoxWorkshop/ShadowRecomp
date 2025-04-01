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

using uchar_xt = std::uint8_t;
typedef uint32_t ulong_xt;
using size_xt = ulong_xt;
using long_xt = std::int32_t;
typedef uchar_xt KIRQL, * PKIRQL;
using void_xt = void;
using boolean_xt = std::uint8_t;
using dword_xt = std::uint32_t;

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

ulong_xt AvSetDisplayMode
(
	IN  PVOID   RegisterBase,
	IN  ulong_xt   Step,
	IN  ulong_xt   Mode,
	IN  ulong_xt   Format,
	IN  ulong_xt   Pitch,
	IN  ulong_xt   FrameBuffer
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


xbox::ntstatus_xt NtFreeVirtualMemory(IN OUT PVOID* BaseAddress, IN OUT PULONG FreeSize, IN ulong_xt FreeType)
{
	g_userHeap.Free(g_memory.Translate((size_t)BaseAddress));
	return X_STATUS_SUCCESS;
}

xbox::ntstatus_xt NtAllocateVirtualMemory(PHANDLE ProcessHandle, PVOID* BaseAddress,
	ULONG_PTR ZeroBits, PSIZE_T RegionSize, ULONG AllocationType,  ULONG Protect)
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

ulong_xt DbgPrint(PCHAR  Format, ...)
{
	va_list args;
	va_start(args, Format);
	vprintf(Format, args); 
	va_end(args);
	return X_STATUS_SUCCESS; 
}


void ExFreePool()
{
	LOG_UTILITY("!!! STUB !!!");
}

void* ExAllocatePoolWithTag
(
	IN size_xt NumberOfBytes,
	IN ulong_xt Tag
)
{
	void* ptr = reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.Alloc(NumberOfBytes > 0 ? NumberOfBytes : 1)));
	printf("  Allocated dummy ptr: %p\n", ptr);
	return ptr;
}

ulong_xt NTAPI ExQueryPoolBlockSize
(
	IN PVOID PoolBlock
)
{
	printf("[STUB] ExQueryPoolBlockSize(%p) -> Dummy 4096\n", PoolBlock);
	return 4096;
}

// Xbox NV setting types (example)
typedef enum {
	NV_SETTING_VIDEO_MODE = 0,
	NV_SETTING_LANGUAGE,
	NV_SETTING_TIMEZONE,
	// Add others as needed
} NV_SETTING_TYPE;

NTSTATUS ExQueryNonVolatileSetting(
	NV_SETTING_TYPE SettingType,
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

NTSTATUS FscSetCacheSize(ULONG NumberOfCachePages) 
{
	printf("[STUB] FscSetCacheSize, Page: %u)\n", NumberOfCachePages);
	return X_STATUS_SUCCESS;
}

ulong_xt HalGetInterruptVector(IN ulong_xt BusInterruptLevel, OUT PKIRQL Irql)
{

	printf("[STUB] HalGetInterruptVector(IRQ %d, Pin %d)\n", BusInterruptLevel, Irql);
	return 0x20; //Return a dummy vector (typical for ISA IRQs)
}

void_xt HalRegisterShutdownNotification
(
	IN int ShutdownRegistration,
	IN boolean_xt Register
)
{
	printf("[STUB] HalRegisterShutdownNotification(%p, %d)\n",
		ShutdownRegistration, Register);
}

void_xt DECLSPEC_NORETURN NTAPI HalReturnToFirmware(ULONG Action)
{
	printf("[STUB] HalReturnToFirmware(%d) - Would reboot\n", Action);
}

void_xt HalReadWritePCISpace
(
	IN ulong_xt   BusNumber,
	IN ulong_xt   SlotNumber,
	IN ulong_xt   RegisterNumber,
	IN PVOID   Buffer,
	IN ulong_xt   Length,
	IN boolean_xt WritePCISpace
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

NTSTATUS IoCreateSymbolicLink(xbox::PUNICODE_STRING SymbolicLinkName, xbox::PUNICODE_STRING DeviceName) 
{
	printf("[STUB] IoCreateSymbolicLink(%.*S -> %.*S)\n",
		SymbolicLinkName->Length / 2, SymbolicLinkName->Buffer,
		DeviceName->Length / 2, DeviceName->Buffer);

	return X_STATUS_SUCCESS;
}

NTSTATUS IoDeleteSymbolicLink(IN xbox::PSTRING SymbolicLinkName) 
{
	printf("[STUB] IoDeleteSymbolicLink(%.*S)\n",
		SymbolicLinkName->Length / 2, SymbolicLinkName->Buffer);
	
	return X_STATUS_SUCCESS;
}

xbox::ntstatus_xt KeBugCheckEx(IN dword_xt BugCheckCode,IN PVOID BugCheckParameter1,IN PVOID BugCheckParameter2,IN PVOID BugCheckParameter3,IN PVOID BugCheckParameter4)
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


void_xt KeBugCheck(IN ulong_xt BugCheckMode)
{
	printf("KeBugCheckEx");

	KeBugCheckEx(BugCheckMode, 0, 0, 0, 0);
}

uchar_xt KeRaiseIrqlToDpcLevel()
{
	printf("[STUB] KeRaiseIrqlToDpcLevel()\n");
	return DISPATCH_LEVEL;
}

void_xt KeQuerySystemTime(PLARGE_INTEGER CurrentTime)
{
	static LONGLONG fakeTime = 0;
	fakeTime += 10000000; // Increment by 1 second

	CurrentTime->QuadPart = fakeTime;
	printf("[STUB] KeQuerySystemTime() -> %lld\n", CurrentTime->QuadPart);
}

LONG KeQueryBasePriorityThread(IN xbox::PKTHREAD Thread) 
{
	printf("[STUB] KeQueryBasePriorityThread(%p)\n", Thread);
	return 8; 
}

boolean_xt KeInsertQueueDpc(xbox::PKDPC Dpc, PVOID Argument1, PVOID Argument2) 
{
	printf("[STUB] KeInsertQueueDpc(%p, %p, %p)\n", Dpc, Argument1, Argument2);
	return TRUE;
}

#define IRQ_BASE 0x30 
#define VECTOR2IRQ(vector)  ((vector)-IRQ_BASE)
void KeInitializeInterrupt(xbox::PKINTERRUPT Interrupt, xbox::PKSERVICE_ROUTINE ServiceRoutine,
	PVOID ServiceContext, ULONG Vector, KIRQL Irql,
	xbox::KINTERRUPT_MODE InterruptMode, BOOLEAN ShareVector) 
{
	Interrupt->ServiceRoutine = ServiceRoutine;
	Interrupt->ServiceContext = ServiceContext;
	Interrupt->BusInterruptLevel = VECTOR2IRQ(Vector);
	Interrupt->Irql = Irql;
	Interrupt->Connected = FALSE;
	Interrupt->Mode = InterruptMode;

	printf("[STUB] KeInitializeInterrupt(Vector 0x%X, Irql %d)\n", Vector, Irql);
}

void KeDisconnectInterrupt(xbox::PKINTERRUPT Interrupt) 
{
	printf("[STUB] KeDisconnectInterrupt(%p)\n", Interrupt);
}

NTSTATUS KeDelayExecutionThread(xbox::KPROCESSOR_MODE WaitMode, BOOLEAN Alertable, PLARGE_INTEGER Interval) 
{

	DWORD ms = Interval->QuadPart / -10000;
	printf("[STUB] KeDelayExecutionThread(%d ms)\n", ms);

	//Actual sleep would go here
	// Sleep(ms);
	return X_STATUS_SUCCESS;
}

boolean_xt KeConnectInterrupt(xbox::PKINTERRUPT Interrupt) 
{
	printf("[STUB] KeConnectInterrupt(%p)\n", Interrupt);
	return X_STATUS_SUCCESS;
}

boolean_xt KeCancelTimer(xbox::PKTIMER Timer) 
{
	printf("[STUB] KeCancelTimer(%p)\n", Timer);
	return TRUE; 
}

VOID KeInitializeDpc(xbox::PKDPC Dpc,
	xbox::PKDEFERRED_ROUTINE DeferredRoutine,
	PVOID DeferredContext
) {
	printf("[STUB] KeInitializeDpc(%p, %p, %p)\n",
		Dpc, DeferredRoutine, DeferredContext);
	Dpc->Type = 19;
	Dpc->Inserted = FALSE;
	Dpc->DeferredRoutine = DeferredRoutine;
	Dpc->DeferredContext = DeferredContext;
}

void InitializeListHead(xbox::PLIST_ENTRY pListHead)
{
	pListHead->Flink = pListHead->Blink = pListHead;
}

VOID KeInitializeTimerEx(xbox::PKTIMER Timer, xbox::TIMER_TYPE Type) 
{
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

boolean_xt KeRemoveQueueDpc(xbox::PKDPC Dpc)
{
	printf("[STUB] KeRemoveQueueDpc(%p)\n", Dpc);
	return TRUE; 
}

// In a real implementation, would restore FPU/XMM registers
xbox::ntstatus_xt KeRestoreFloatingPointState(xbox::PKFLOATING_SAVE  SaveArea)
{
	printf("[STUB] KeRestoreFloatingPointState(%p)\n", SaveArea);
	NTSTATUS ret = X_STATUS_SUCCESS;
	return ret;
}

NTSTATUS KeSaveFloatingPointState(xbox::PKFLOATING_SAVE SaveArea) 
{
	printf("[STUB] KeSaveFloatingPointState(%p)\n", SaveArea);
	return X_STATUS_SUCCESS;
}

long_xt KeSetBasePriorityThread(xbox::PKTHREAD Thread, LONG Priority) 
{
	Thread->Priority = Priority;
	long_xt ret = Thread->Priority;
	printf("[STUB] KeSetBasePriorityThread(%p, %ld)\n", Thread, Priority);
	return ret; 
}

boolean_xt KeSetDisableBoostThread(xbox::PKTHREAD Thread, BOOLEAN Disable) 
{
	printf("[STUB] KeSetDisableBoostThread(%p, %s)\n",
		Thread, Disable ? "TRUE" : "FALSE");
	boolean_xt prevDisableBoost = Thread->DisableBoost;
	Thread->DisableBoost = (CHAR)Disable;
	return prevDisableBoost;
}

LONG KeSetEvent(xbox::PKEVENT Event, xbox::KPRIORITY Increment, BOOLEAN Wait) 
{
	printf("[STUB] KeSetEvent(%p, %d, %s)\n", Event, Increment, Wait ? "TRUE" : "FALSE");
	return 1; 
}

BOOLEAN KeSetTimer(xbox::PKTIMER Timer, LARGE_INTEGER DueTime, xbox::PKDPC Dpc) 
{
	printf("[STUB] KeSetTimer(%p, %lld ns, Dpc %p)\n", Timer, DueTime.QuadPart, Dpc);
	return FALSE; // Timer wasn't already active
}

VOID KeStallExecutionProcessor(ULONG Microseconds) 
{
	printf("[STUB] KeStallExecutionProcessor(%u)\n", Microseconds);
}

BOOLEAN KeSynchronizeExecution(xbox::PKINTERRUPT Interrupt, xbox::PKSYNCHRONIZE_ROUTINE SynchronizeRoutine, PVOID SynchronizeContext) 
{
	printf("[STUB] KeSynchronizeExecution(Int %p, Routine %p, Ctx %p)\n", Interrupt, SynchronizeRoutine, SynchronizeContext);
	return SynchronizeRoutine(SynchronizeContext);
}

NTSTATUS KeWaitForSingleObject(PVOID Object, xbox::KWAIT_REASON WaitReason, xbox::KPROCESSOR_MODE WaitMode, BOOLEAN Alertable,
	PLARGE_INTEGER Timeout
) {
	printf("[STUB] KeWaitForSingleObject(%p, Reason %d, %s, Timeout %lld)\n",
		Object, WaitReason, Alertable ? "Alertable" : "Non-Alertable",
		Timeout ? Timeout->QuadPart : -1);

	return X_STATUS_SUCCESS; //Pretend object was signaled
}

KIRQL KfRaiseIrql(KIRQL NewIrql) 
{	KIRQL old = PASSIVE_LEVEL;
	printf("[STUB] KfRaiseIrql(%d) -> %d\n", NewIrql, old);
	return old; // Return previous IRQL
}

PVOID MmAllocateContiguousMemory(SIZE_T NumberOfBytes) 
{
	printf("[STUB] MmAllocateContiguousMemory(%zu bytes)\n", NumberOfBytes);
	return reinterpret_cast<PVOID>(g_memory.MapVirtual(g_userHeap.AllocPhysical(NumberOfBytes, GPU_ALIGNMENT)));
}

PVOID MmAllocateContiguousMemoryEx(IN ulong_xt NumberOfBytes, IN xbox::physical_address_xt LowestAcceptableAddress,
	IN xbox::physical_address_xt HighestAcceptableAddress, IN ulong_xt Alignment OPTIONAL,
	IN ulong_xt ProtectionType
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

void KfLowerIrql(KIRQL NewIrql) 
{
	printf("[STUB] KfLowerIrql(%d)\n", NewIrql);
}


NTSTATUS NtClose(HANDLE Handle) 
{
	printf("NtClose(%p)", Handle);
	return X_STATUS_SUCCESS;
}

NTSTATUS NtCreateEvent(OUT PHANDLE EventHandle,IN  xbox::POBJECT_ATTRIBUTES  ObjectAttributes OPTIONAL,
	IN  xbox::EVENT_TYPE EventType, IN boolean_xt InitialState
) {
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

NTSTATUS NtCreateFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	xbox::POBJECT_ATTRIBUTES ObjectAttributes,
	xbox::PIO_STATUS_BLOCK IoStatusBlock,
	PLARGE_INTEGER AllocationSize,
	ULONG FileAttributes,
	ULONG ShareAccess,
	ULONG CreateDisposition,
	ULONG CreateOptions,
	PVOID EaBuffer,
	ULONG EaLength
) 
{

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

NTSTATUS NtDeviceIoControlFile(
	HANDLE FileHandle,
	HANDLE Event,
	xbox::PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	xbox::PIO_STATUS_BLOCK IoStatusBlock,
	ULONG IoControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength
) {
	LOG_UTILITY("NtDeviceIoControlFile(0x%X)", IoControlCode);
	return X_STATUS_SUCCESS;
}

NTSTATUS NtFlushBuffersFile(HANDLE FileHandle, xbox::PIO_STATUS_BLOCK IoStatusBlock) 
{
#ifdef _WIN32
	FlushFileBuffers(FileHandle);
#else
	fsync(reinterpret_cast<int>(FileHandle));
#endif
	return X_STATUS_SUCCESS;
}

NTSTATUS NtFsControlFile(
	HANDLE FileHandle,
	HANDLE Event,
	xbox::PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	xbox::PIO_STATUS_BLOCK IoStatusBlock,
	ULONG FsControlCode,
	PVOID InputBuffer,
	ULONG InputBufferLength,
	PVOID OutputBuffer,
	ULONG OutputBufferLength
) {
	LOG_UTILITY("NtFsControlFile(0x%X)", FsControlCode);
	return X_STATUS_SUCCESS;
}

NTSTATUS NtOpenFile(
	PHANDLE FileHandle,
	ACCESS_MASK DesiredAccess,
	xbox::POBJECT_ATTRIBUTES ObjectAttributes,
	xbox::PIO_STATUS_BLOCK IoStatusBlock,
	ULONG ShareAccess,
	ULONG OpenOptions
) {
	// Reuse NtCreateFile logic
	return NtCreateFile(FileHandle, DesiredAccess, ObjectAttributes, IoStatusBlock,
		nullptr, 0, ShareAccess, FILE_OPEN, OpenOptions, nullptr, 0);
}

NTSTATUS NtOpenSymbolicLinkObject(OUT PHANDLE LinkHandle, IN xbox::POBJECT_ATTRIBUTES ObjectAttributes
) {
	*LinkHandle = reinterpret_cast<HANDLE>(0xCAFEBABE);
	return X_STATUS_SUCCESS;
}

NTSTATUS NtQueryDirectoryFile(
	IN  HANDLE FileHandle,
	IN  HANDLE Event OPTIONAL,
	IN  PVOID ApcRoutine, // Todo: define this routine's prototype
	IN  PVOID ApcContext,
	OUT xbox::PIO_STATUS_BLOCK IoStatusBlock,
	OUT xbox::FILE_DIRECTORY_INFORMATION* FileInformation,
	IN  ulong_xt Length,
	IN  xbox::FILE_INFORMATION_CLASS FileInformationClass,
	IN  xbox::PSTRING FileMask,
	IN  boolean_xt RestartScan
) 
{
	memset(FileInformation, 0, Length);
	return X_STATUS_SUCCESS;
}

NTSTATUS NtQueryFullAttributesFile(
	xbox::POBJECT_ATTRIBUTES ObjectAttributes,
	xbox::PFILE_NETWORK_OPEN_INFORMATION FileInformation
) {
	FileInformation->EndOfFile.QuadPart = 0x1000; // Dummy size
	return X_STATUS_SUCCESS;
}

NTSTATUS NtQueryInformationFile(
	HANDLE FileHandle,
	xbox::PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG FileInformationLength,
	xbox::FILE_INFORMATION_CLASS FileInformationClass
) {
	if (FileInformationClass == xbox::FileStandardInformation) 
	{
		auto* info = reinterpret_cast<xbox::FILE_STANDARD_INFORMATION*>(FileInformation);
		info->EndOfFile.QuadPart = 0x1000; // Dummy 4KB file
	}
	return X_STATUS_SUCCESS;
}

NTSTATUS NtQuerySymbolicLinkObject(
	HANDLE LinkHandle,
	xbox::PUNICODE_STRING TargetName,
	PULONG ReturnedLength
) {
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

NTSTATUS NtQueryVirtualMemory(
	PVOID BaseAddress,
	PMEMORY_BASIC_INFORMATION Buffer
) {

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

NTSTATUS MmLockUnlockBufferPages(PVOID BaseAddress, SIZE_T NumberOfBytes, BOOLEAN Lock) 
{
	LOG_UTILITY("MmLockUnlockBufferPages(%p, %zu, %s)", BaseAddress, NumberOfBytes, Lock ? "Lock" : "Unlock");
	return X_STATUS_SUCCESS; // No-op in recompiler
}

NTSTATUS MmLockUnlockPhysicalPage(xbox::physical_address_xt PhysicalAddress, BOOLEAN Lock) 
{
	uint32_t addr = static_cast<uint32_t>(PhysicalAddress);
	LOG_UTILITY("MmLockUnlockPhysicalPage(0x%08X, %s)", addr, Lock ? "Lock" : "Unlock");
	return X_STATUS_SUCCESS;
}

VOID MmPersistContiguousMemory(PVOID BaseAddress, SIZE_T NumberOfBytes, BOOLEAN Persist) {
	LOG_UTILITY("MmPersistContiguousMemory(%p, %zu, %s)", BaseAddress, NumberOfBytes, Persist ? "Persist" : "Unpersist");
}

NTSTATUS MmQueryAddressProtect(PVOID BaseAddress, PULONG Protect) 
{
	*Protect = PAGE_READWRITE; 
	return X_STATUS_SUCCESS;
}

SIZE_T MmQueryAllocationSize(PVOID BaseAddress) 
{
	return g_userHeap.Size(BaseAddress); 
}

NTSTATUS MmSetAddressProtect(PVOID BaseAddress, SIZE_T NumberOfBytes, ULONG NewProtect) 
{
	LOG_UTILITY("MmSetAddressProtect(%p, %zu, 0x%X)", BaseAddress, NumberOfBytes, NewProtect);
	return X_STATUS_SUCCESS;
}

NTSTATUS NtWaitForSingleObject(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout)
{
	return X_STATUS_SUCCESS; // Pretend the object is always signaled
}

NTSTATUS NtSetEvent(HANDLE Handle, PULONG PreviousState)
{
	if (PreviousState) 
		*PreviousState = 1;

	return X_STATUS_SUCCESS;
}

NTSTATUS NtWriteFile(
	HANDLE FileHandle,
	HANDLE Event,
	xbox::PIO_APC_ROUTINE ApcRoutine,
	PVOID ApcContext,
	xbox::PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Buffer,
	ULONG Length,
	PLARGE_INTEGER ByteOffset
) {
	if (IoStatusBlock) {
		IoStatusBlock->Status = X_STATUS_SUCCESS;
		IoStatusBlock->Information = Length; // Pretend all bytes were written
	}
	return X_STATUS_SUCCESS;
}

ULONG RtlNtStatusToDosError(NTSTATUS Status) 
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

NTSTATUS NtSuspendThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount) 
{
	if (PreviousSuspendCount) 
		*PreviousSuspendCount = 0;

	return X_STATUS_SUCCESS;
}

NTSTATUS NtResumeThread(HANDLE ThreadHandle, PULONG PreviousSuspendCount) 
{
	if (PreviousSuspendCount) 
		*PreviousSuspendCount = 0;

	return X_STATUS_SUCCESS;
}

NTSTATUS PsCreateSystemThreadEx(
	PHANDLE ThreadHandle,
	ULONG Flags,
	SIZE_T StackSize,
	SIZE_T Reserved,
	PVOID StartContext
) {
	*ThreadHandle = reinterpret_cast<HANDLE>(0x12345678); // Dummy handle
	return X_STATUS_SUCCESS;
}

void PsTerminateSystemThread(NTSTATUS ExitStatus) 
{
	// No-op (thread cleanup is handled by the recompiler)
}

NTSTATUS ObReferenceObjectByHandle(
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

void RtlInitializeCriticalSection(PRTL_CRITICAL_SECTION CritSec) {

}

void RtlEnterCriticalSection(PRTL_CRITICAL_SECTION CritSec) {
	
}

void RtlLeaveCriticalSection(PRTL_CRITICAL_SECTION CritSec) {
	
}

void RtlInitAnsiString(xbox::PANSI_STRING Destination, xbox::PCSZ Source) {
	Destination->Buffer = const_cast<PCHAR>(Source);
	Destination->Length = static_cast<USHORT>(strlen(Source));
	Destination->MaximumLength = Destination->Length + 1;
}

NTSTATUS NtQueryVolumeInformationFile(
	HANDLE FileHandle,
	xbox::PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FsInformation,
	ULONG Length,
	xbox::FS_INFORMATION_CLASS FsInformationClass
) {
	if (IoStatusBlock) {
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

NTSTATUS NtSetInformationFile(
	HANDLE FileHandle,
	xbox::PIO_STATUS_BLOCK IoStatusBlock,
	PVOID FileInformation,
	ULONG Length,
	xbox::FILE_INFORMATION_CLASS FileInformationClass
) {
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

NTSTATUS XeLoadSection(const char* Name) 
{
	return X_STATUS_SUCCESS;
}

NTSTATUS XeUnloadSection(const char* Name) 
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

void RtlRaiseException(PEXCEPTION_RECORD ExceptionRecord) {
	LOG_UTILITY("[STUB] RtlRaiseException() - Crashing intentionally");
	std::terminate(); 
}

void RtlUnwind_() 
{
	LOG_UTILITY("[STUB] RtlUnwind() - Ignoring");
}

NTSTATUS NtWaitForSingleObjectEx(
	HANDLE Handle,
	BOOLEAN Alertable,
	PLARGE_INTEGER Timeout,
	DWORD WaitMode
) {
	return NtWaitForSingleObject(Handle, Alertable, Timeout); // Reuse stub
}

GUEST_FUNCTION_HOOK(__imp__NtWaitForSingleObjectEx, NtWaitForSingleObjectEx);
GUEST_FUNCTION_HOOK(__imp__RtlRaiseException, RtlRaiseException);
GUEST_FUNCTION_HOOK(__imp__RtlUnwind, RtlUnwind_);

GUEST_FUNCTION_HOOK(__imp__XcSHAInit, XcSHAInit);
GUEST_FUNCTION_HOOK(__imp__XcSHAUpdate, XcSHAUpdate);
GUEST_FUNCTION_HOOK(__imp__XcSHAFinal, XcSHAFinal);
GUEST_FUNCTION_HOOK(__imp__XcHMAC, XcHMAC);

GUEST_FUNCTION_HOOK(__imp__XeLoadSection, XeLoadSection);
GUEST_FUNCTION_HOOK(__imp__XeUnloadSection, XeUnloadSection);
GUEST_FUNCTION_HOOK(__imp__HalInitiateShutdown, HalInitiateShutdown);
GUEST_FUNCTION_HOOK(__imp__NtSetInformationFile, NtSetInformationFile);
GUEST_FUNCTION_HOOK(__imp__NtQueryVolumeInformationFile, NtQueryVolumeInformationFile);

GUEST_FUNCTION_HOOK(__imp__RtlInitAnsiString, RtlInitAnsiString);
GUEST_FUNCTION_HOOK(__imp__RtlInitializeCriticalSection, RtlInitializeCriticalSection);
GUEST_FUNCTION_HOOK(__imp__RtlEnterCriticalSection, RtlEnterCriticalSection);
GUEST_FUNCTION_HOOK(__imp__RtlLeaveCriticalSection, RtlLeaveCriticalSection);

GUEST_FUNCTION_HOOK(__imp__ObfDereferenceObject, ObfDereferenceObject);
GUEST_FUNCTION_HOOK(__imp__ObReferenceObjectByHandle, ObReferenceObjectByHandle);
GUEST_FUNCTION_HOOK(__imp__PsTerminateSystemThread, PsTerminateSystemThread);
GUEST_FUNCTION_HOOK(__imp__PsCreateSystemThreadEx, PsCreateSystemThreadEx);
GUEST_FUNCTION_HOOK(__imp__NtResumeThread, NtResumeThread);
GUEST_FUNCTION_HOOK(__imp__NtSuspendThread, NtSuspendThread);
GUEST_FUNCTION_HOOK(__imp__RtlEqualString, RtlEqualString);
GUEST_FUNCTION_HOOK(__imp__RtlCompareMemoryUlong, RtlCompareMemoryUlong);
GUEST_FUNCTION_HOOK(__imp__RtlNtStatusToDosError, RtlNtStatusToDosError);
GUEST_FUNCTION_HOOK(__imp__NtWriteFile, NtWriteFile);
GUEST_FUNCTION_HOOK(__imp__NtWaitForSingleObject, NtWaitForSingleObject);
GUEST_FUNCTION_HOOK(__imp__NtSetEvent, NtSetEvent);
GUEST_FUNCTION_HOOK(__imp__NtYieldExecution, NtYieldExecution);

GUEST_FUNCTION_HOOK(__imp__MmSetAddressProtect, MmSetAddressProtect);
GUEST_FUNCTION_HOOK(__imp__MmQueryAllocationSize, MmQueryAllocationSize);
GUEST_FUNCTION_HOOK(__imp__MmQueryAddressProtect, MmQueryAddressProtect);
GUEST_FUNCTION_HOOK(__imp__MmPersistContiguousMemory, MmPersistContiguousMemory);
GUEST_FUNCTION_HOOK(__imp__MmLockUnlockPhysicalPage, MmLockUnlockPhysicalPage);
GUEST_FUNCTION_HOOK(__imp__MmLockUnlockBufferPages, MmLockUnlockBufferPages);
GUEST_FUNCTION_HOOK(__imp__MmGetPhysicalAddress, MmGetPhysicalAddress);

GUEST_FUNCTION_HOOK(__imp__NtQueryVirtualMemory, NtQueryVirtualMemory);
GUEST_FUNCTION_HOOK(__imp__NtQuerySymbolicLinkObject, NtQuerySymbolicLinkObject);
GUEST_FUNCTION_HOOK(__imp__NtQueryInformationFile, NtQueryInformationFile);
GUEST_FUNCTION_HOOK(__imp__NtQueryFullAttributesFile, NtQueryFullAttributesFile);
GUEST_FUNCTION_HOOK(__imp__NtQueryDirectoryFile, NtQueryDirectoryFile);
GUEST_FUNCTION_HOOK(__imp__NtOpenSymbolicLinkObject, NtOpenSymbolicLinkObject);
GUEST_FUNCTION_HOOK(__imp__NtOpenFile, NtOpenFile);
GUEST_FUNCTION_HOOK(__imp__NtFsControlFile, NtFsControlFile);
GUEST_FUNCTION_HOOK(__imp__NtFlushBuffersFile, NtFlushBuffersFile);
GUEST_FUNCTION_HOOK(__imp__NtDeviceIoControlFile, NtDeviceIoControlFile);

GUEST_FUNCTION_HOOK(__imp__NtCreateFile, NtCreateFile);
GUEST_FUNCTION_HOOK(__imp__NtClose, NtClose);
GUEST_FUNCTION_HOOK(__imp__NtCreateEvent, NtCreateEvent);


GUEST_FUNCTION_HOOK(__imp__KfLowerIrql, KfLowerIrql);
GUEST_FUNCTION_HOOK(__imp__KfRaiseIrql, KfRaiseIrql);
GUEST_FUNCTION_HOOK(__imp__KeWaitForSingleObject, KeWaitForSingleObject);
GUEST_FUNCTION_HOOK(__imp__KeSynchronizeExecution, KeSynchronizeExecution);

GUEST_FUNCTION_HOOK(__imp__MmAllocateSystemMemory, MmAllocateSystemMemory);
GUEST_FUNCTION_HOOK(__imp__MmClaimGpuInstanceMemory, MmClaimGpuInstanceMemory);
GUEST_FUNCTION_HOOK(__imp__MmFreeSystemMemory, MmFreeSystemMemory);
GUEST_FUNCTION_HOOK(__imp__MmFreeContiguousMemory, MmFreeContiguousMemory);
GUEST_FUNCTION_HOOK(__imp__MmAllocateContiguousMemoryEx, MmAllocateContiguousMemoryEx);
GUEST_FUNCTION_HOOK(__imp__MmAllocateContiguousMemory, MmAllocateContiguousMemory);
GUEST_FUNCTION_HOOK(__imp__NtFreeVirtualMemory, NtFreeVirtualMemory);
GUEST_FUNCTION_HOOK(__imp__NtAllocateVirtualMemory, NtAllocateVirtualMemory);

GUEST_FUNCTION_HOOK(__imp__KeStallExecutionProcessor, KeStallExecutionProcessor);
GUEST_FUNCTION_HOOK(__imp__KeSetTimer, KeSetTimer);
GUEST_FUNCTION_HOOK(__imp__KeSetEvent, KeSetEvent);
GUEST_FUNCTION_HOOK(__imp__KeSetDisableBoostThread, KeSetDisableBoostThread);
GUEST_FUNCTION_HOOK(__imp__KeSetBasePriorityThread, KeSetBasePriorityThread);
GUEST_FUNCTION_HOOK(__imp__KeSaveFloatingPointState, KeSaveFloatingPointState);
GUEST_FUNCTION_HOOK(__imp__KeRestoreFloatingPointState, KeRestoreFloatingPointState);
GUEST_FUNCTION_HOOK(__imp__KeRemoveQueueDpc, KeRemoveQueueDpc);
GUEST_FUNCTION_HOOK(__imp__KeInitializeTimerEx, KeInitializeTimerEx);
GUEST_FUNCTION_HOOK(__imp__KeInitializeDpc, KeInitializeDpc);

GUEST_FUNCTION_HOOK(__imp__KeCancelTimer, KeCancelTimer);
GUEST_FUNCTION_HOOK(__imp__KeConnectInterrupt, KeConnectInterrupt);
GUEST_FUNCTION_HOOK(__imp__KeDelayExecutionThread, KeDelayExecutionThread);
GUEST_FUNCTION_HOOK(__imp__KeDisconnectInterrupt, KeDisconnectInterrupt);
GUEST_FUNCTION_HOOK(__imp__KeInitializeInterrupt, KeInitializeInterrupt);
GUEST_FUNCTION_HOOK(__imp__KeInsertQueueDpc, KeInsertQueueDpc);
GUEST_FUNCTION_HOOK(__imp__KeQueryBasePriorityThread, KeQueryBasePriorityThread);
GUEST_FUNCTION_HOOK(__imp__KeQuerySystemTime, KeQuerySystemTime);
GUEST_FUNCTION_HOOK(__imp__KeRaiseIrqlToDpcLevel, KeRaiseIrqlToDpcLevel);
GUEST_FUNCTION_HOOK(__imp__FscSetCacheSize, FscSetCacheSize);
GUEST_FUNCTION_HOOK(__imp__KeBugCheck, KeBugCheck);
GUEST_FUNCTION_HOOK(__imp__IoCreateSymbolicLink, IoCreateSymbolicLink);
GUEST_FUNCTION_HOOK(__imp__IoDeleteSymbolicLink, IoDeleteSymbolicLink);
GUEST_FUNCTION_HOOK(__imp__ExQueryPoolBlockSize, ExQueryPoolBlockSize);
GUEST_FUNCTION_HOOK(__imp__HalRegisterShutdownNotification, HalRegisterShutdownNotification);
GUEST_FUNCTION_HOOK(__imp__HalReadWritePCISpace, HalReadWritePCISpace);
GUEST_FUNCTION_HOOK(__imp__HalReturnToFirmware, HalReturnToFirmware);
GUEST_FUNCTION_HOOK(__imp__HalGetInterruptVector, HalGetInterruptVector);
GUEST_FUNCTION_HOOK(__imp__ExAllocatePoolWithTag, ExAllocatePoolWithTag);
GUEST_FUNCTION_HOOK(__imp__ExFreePool, ExFreePool);
GUEST_FUNCTION_HOOK(__imp__ExQueryNonVolatileSetting, ExQueryNonVolatileSetting);
GUEST_FUNCTION_HOOK(__imp__AvGetSavedDataAddress, AvGetSavedDataAddress);
GUEST_FUNCTION_HOOK(__imp__AvSetSavedDataAddress, AvSetSavedDataAddress);
GUEST_FUNCTION_HOOK(__imp__AvSetDisplayMode, AvSetDisplayMode);
GUEST_FUNCTION_HOOK(__imp__AvSendTVEncoderOption, AvSendTVEncoderOption);
GUEST_FUNCTION_HOOK(__imp__NtReadFile, NtReadFile);

GUEST_FUNCTION_HOOK(__imp__DbgPrint, DbgPrint);

