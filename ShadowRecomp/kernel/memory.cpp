#include <stdafx.h>
#include "heap.h"
#include "memory.h"
#include "function.h"

Memory::Memory()
{
#ifdef _WIN32
	base = (uint8_t*)VirtualAlloc((void*)0x100000000ull, X86_MEMORY_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (base == nullptr) {
		DWORD error = GetLastError();
		fprintf(stderr, "Failed to allocate at 0x10000000 (Error: 0x%lX)\n", error);
		base = (uint8_t*)VirtualAlloc(nullptr, X86_MEMORY_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	}

	if (base == nullptr)
		base = (uint8_t*)VirtualAlloc(nullptr, X86_MEMORY_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

	if (base == nullptr)
		return;

	DWORD oldProtect;
	VirtualProtect(base, 4096, PAGE_NOACCESS, &oldProtect);
#else
	base = (uint8_t*)mmap((void*)0x100000000ull, X86_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	if (base == (uint8_t*)MAP_FAILED)
		base = (uint8_t*)mmap(NULL, X86_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);

	mprotect(base, 4096, PROT_NONE);
#endif


	for (size_t i = 0; X86FuncMappings[i].guest != 0; i++)
	{
		if (X86FuncMappings[i].host != nullptr)
			InsertFunction(X86FuncMappings[i].guest, X86FuncMappings[i].host);
	}
}

void* MmGetHostAddress(uint32_t ptr)
{
	return g_memory.Translate(ptr);
}

void* ObjectHeapMallocThing(int Size)
{
	void* result;

	while (1)
	{
		void* result = g_userHeap.Alloc(Size);

		if (result)
			break;

		printf("bad allocation");
		__debugbreak();
	}

	return (void*)g_memory.MapVirtual(result);
}

void* mAlloc(size_t Size)
{
	return (void*)g_memory.MapVirtual(g_userHeap.Alloc(Size));
}


