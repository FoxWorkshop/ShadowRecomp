#include <stdafx.h>
#include "heap.h"
#include "memory.h"

constexpr size_t XBE_LOAD_ADDR = 0x00010000ull;
constexpr size_t XBE_END_ADDR = 0x006A1000ull;
constexpr size_t HEAP_BEGIN = XBE_END_ADDR;    //Start heap after XBE 
constexpr size_t HEAP_END = (0x01FFFFFFull & ~0xFFF);  // Align to 4KB boundary
constexpr size_t PHYS_HEAP_END = 0x03FFFFFFull; //todo check if this is correct

void Heap::Init()
{
    printf("Heap Init\n");
    heap = o1heapInit(g_memory.Translate(HEAP_BEGIN), HEAP_END - HEAP_BEGIN);
    physicalHeap = o1heapInit(g_memory.Translate(HEAP_END), PHYS_HEAP_END - HEAP_END);

    printf("Regular Heap: %p - %p\n", heap, (char*)heap + (HEAP_END - HEAP_BEGIN));
    printf("Physical Heap: %p - %p\n", physicalHeap, (char*)physicalHeap + (PHYS_HEAP_END - HEAP_END));
    printf("Heap and PhysicaHeap Done.\n");
}

std::atomic_flag allocFlag = ATOMIC_FLAG_INIT;

void* Heap::Alloc(size_t size)
{
    std::lock_guard lock(mutex);
    void* ptr = o1heapAllocate(heap, std::max<size_t>(1, size));
    printf("[ALLOC] %p (size: 0x%zX)\n", ptr, size);
    return ptr;
}

void* Heap::AllocPhysical(size_t size, size_t alignment)
{
    size = std::max<size_t>(1, size);
    alignment = alignment == 0 ? 0x1000 : std::max<size_t>(16, alignment);

    std::lock_guard lock(physicalMutex);

    void* ptr = o1heapAllocate(physicalHeap, size + alignment);
    size_t aligned = ((size_t)ptr + alignment) & ~(alignment - 1);

    *((void**)aligned - 1) = ptr;
    *((size_t*)aligned - 2) = size + O1HEAP_ALIGNMENT;

    printf("[ALLOCPHYSICAL] %p ( aligned: 0x%zX, size: 0x%zX)\n", ptr, aligned, size);

    return (void*)aligned;
}

void Heap::Free(void* ptr)
{

    // Check if pointer is in physical heap region
    if (ptr >= physicalHeap)
    {
        printf("[FREE PHYSICAL] ptr: %p\n", ptr);
        std::lock_guard lock(physicalMutex);
        o1heapFree(physicalHeap, *((void**)ptr - 1));
    }
    else
    {
        printf("[FREE REGULAR] ptr: %p\n", ptr);
        std::lock_guard lock(mutex);
        o1heapFree(heap, ptr);
    }

}

size_t Heap::Size(void* ptr)
{
    if (ptr)
        return *((size_t*)ptr - 2) - O1HEAP_ALIGNMENT; // relies on fragment header in o1heap.c

    return 0;
}

uint32_t RtlAllocateHeap(uint32_t heapHandle, uint32_t flags, uint32_t size)
{
    void* ptr = g_userHeap.Alloc(size);
    if ((flags & 0x8) != 0)
        memset(ptr, 0, size);

    assert(ptr);
    return g_memory.MapVirtual(ptr);
}

uint32_t RtlReAllocateHeap(uint32_t heapHandle, uint32_t flags, uint32_t memoryPointer, uint32_t size)
{
    void* ptr = g_userHeap.Alloc(size);
    if ((flags & 0x8) != 0)
        memset(ptr, 0, size);

    if (memoryPointer != 0)
    {
        void* oldPtr = g_memory.Translate(memoryPointer);
        memcpy(ptr, oldPtr, std::min<size_t>(size, g_userHeap.Size(oldPtr)));
        g_userHeap.Free(oldPtr);
    }

    assert(ptr);
    return g_memory.MapVirtual(ptr);
}

uint32_t RtlFreeHeap(uint32_t heapHandle, uint32_t flags, uint32_t memoryPointer)
{
    if (memoryPointer != NULL)
        g_userHeap.Free(g_memory.Translate(memoryPointer));

    return true;
}

uint32_t RtlSizeHeap(uint32_t heapHandle, uint32_t flags, uint32_t memoryPointer)
{
    if (memoryPointer != NULL)
        return (uint32_t)g_userHeap.Size(g_memory.Translate(memoryPointer));

    return 0;
}

uint32_t XAllocMem(uint32_t size, uint32_t flags)
{
    void* ptr = (flags & 0x80000000) != 0 ?
        g_userHeap.AllocPhysical(size, (1ull << ((flags >> 24) & 0xF))) :
        g_userHeap.Alloc(size);

    if ((flags & 0x40000000) != 0)
        memset(ptr, 0, size);

    assert(ptr);
    return g_memory.MapVirtual(ptr);
}

void XFreeMem(uint32_t baseAddress, uint32_t flags)
{
    if (baseAddress != NULL)
        g_userHeap.Free(g_memory.Translate(baseAddress));
}