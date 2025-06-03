#include <stdafx.h>
#include <kernel/function.h>
#include <kernel/io/file_system.h>
#include <cpu/guest_thread.h>
#include <kernel/kernel.h>
#include <iostream>
#include <filesystem>


void PlayMusic(const char* filename)
{
	/**VGMSTREAM* vgmstream = init_vgmstream(filename);

	if (!vgmstream) {
		printf("Failed to load ADX file: %s\n", filename);
	}*/
}

void* unknownSoundPtr = nullptr;
uint8_t* ppDS = nullptr;

uint32_t DSound_CMemoryManager_PoolAlloc(int tag, int NumberOfBytes, int a3)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	uint32_t content = 0;
	if (!unknownSoundPtr) //I can't figure out where they init this so let's do it manually here once
	{
		unknownSoundPtr = g_userHeap.AllocPhysical(0x2000, 1);
		content = g_memory.MapVirtual(unknownSoundPtr);
		X86_STORE_U32(0x3FF838, content);
	}


	void* ptr = (g_userHeap.Alloc(NumberOfBytes > 0 ? NumberOfBytes : 1));
	content = g_memory.MapVirtual(ptr);
	auto size = g_userHeap.Size(ptr);
	X86_STORE_U32(0x3FF838, *reinterpret_cast<volatile uint32_t*>(base + 0x3FF838 + size));


	return content;
}

int sub_3E7516_r(DWORD* this_)
{
	return 0;
}

void FreeAudioPtrs()
{
	g_userHeap.Free(unknownSoundPtr);
}

int sub_3E5881_r(DWORD* a1)
{
	return 0;
}

int sub_3E353C_r(DWORD* this_, DWORD* a2)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 4;
	return 0;
}

int IDirectSound_DownloadEffectsImage(int a1, DWORD* a2, int a3, int a4, DWORD* a5)
{
	return 0;
}

HRESULT DirectSoundCreate_r(LPCGUID pcGuidDevice, uint32_t* ppDS_, LPUNKNOWN pUnkOuter)
{
	/**auto base = g_memory.base;
	ppDS = (uint8_t*)g_userHeap.AllocPhysical(0x24A0, 1);
	*(uint32_t*)(ppDS + 8) = g_memory.MapVirtual(g_userHeap.Alloc(0x200));
	*(uint32_t*)(ppDS + 56) = g_memory.MapVirtual(g_userHeap.Alloc(0x200));
	*(uint32_t*)(ppDS + 8 + 56) = g_memory.MapVirtual(g_userHeap.Alloc(0x200));
	auto map = g_memory.MapVirtual(ppDS);


	//*ppDS_ = map;
	*ppDS_ = map;

	auto map2 = g_memory.MapVirtual(g_userHeap.AllocPhysical(0x200, 1));
	X86_STORE_U32(0x550310, map);
	auto res = X86_LOAD_U32(0x550310);*/
	return 0;
}

GUEST_FUNCTION_HOOK(sub_3E5A27, DirectSoundCreate_r);

int sub_3EBA02_r(int a1)
{
	return 1;
}

int sub_3EBAFA_r(char* a1)
{
	return 1;
}

int sub_3E8B60_r(int a1)
{
	return 0;
}


int CDirectSound_DownloadEffectsImage_r(int a1, DWORD* a2, int a3, int a4, DWORD* a5)
{
	return 0;
}

int CMcpxAPU_ServiceDeferredCommandsLow(int this_)
{
	return 0;
}



X86_FUNC_IMPL(__imp__sub_3356C0);
X86_FUNC(sub_3356C0)
{
	__imp__sub_3356C0(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_335090);
X86_FUNC(sub_335090)
{
	__imp__sub_335090(ctx, base);
}

void* sub_3E5ACD_r(int a1, int a2, int a3, int a4)
{
	return 0;
}

int  sub_3E6D68_r(void** this_)
{
	return 0;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3EBA02, sub_3EBA02_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3EB429, sub_3EBA02_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3EB460, sub_3EBA02_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3EB4F2, sub_3EBA02_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3EB497, sub_3EBA02_r);
GUEST_FUNCTION_HOOK_THISCALL(sub_3EBAFA, sub_3EBAFA_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E5ACD, sub_3E5ACD_r);
//GUEST_FUNCTION_HOOK_THISCALL(sub_3E7516, sub_3E7516_r);
//GUEST_FUNCTION_HOOK_THISCALL(sub_3E8B60, sub_3E8B60_r);
GUEST_FUNCTION_HOOK_THISCALL(sub_3E6D68, sub_3E6D68_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E35B5, CDirectSound_DownloadEffectsImage_r);
//GUEST_FUNCTION_HOOK_THISCALL(sub_3E6FA8, CMcpxAPU_ServiceDeferredCommandsLow);

