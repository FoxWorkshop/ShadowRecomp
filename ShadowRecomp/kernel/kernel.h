
#pragma once
#include <cstdint>
#include "heap.h"
#include "memory.h"
#include <unordered_map>

#define GUEST_INVALID_HANDLE_VALUE 0xFFFFFFFF

struct KernelObject
{
    virtual ~KernelObject()
    {
    }

    virtual uint32_t Wait(uint32_t timeout)
    {
        assert(false && "Wait not implemented for this kernel object.");
        return STATUS_TIMEOUT;
    }
};

template<typename T, typename... Args>
inline T* CreateKernelObject(Args&&... args)
{
	printf("CreateKernelObject %s\n", typeid(T).name());    
    static_assert(std::is_base_of_v<KernelObject, T>);
    return g_userHeap.AllocPhysical<T>(std::forward<Args>(args)...);
}

template<typename T = KernelObject>
inline T* GetKernelObject(uint32_t handle)
{
	printf("GetKernelObject %s\n", typeid(T).name());
    assert(handle != GUEST_INVALID_HANDLE_VALUE);
    return reinterpret_cast<T*>(g_memory.Translate(handle));
}

uint32_t GetKernelHandle(KernelObject* obj);

void DestroyKernelObject(KernelObject* obj);
void DestroyKernelObject(uint32_t handle);

bool IsKernelObject(uint32_t handle);
bool IsKernelObject(void* obj);

bool IsInvalidKernelObject(void* obj);

template<typename T = void>
inline T* GetInvalidKernelObject()
{
    return reinterpret_cast<T*>(g_memory.Translate(GUEST_INVALID_HANDLE_VALUE));
}

extern Mutex g_kernelLock;
