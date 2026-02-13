#include <stdafx.h>
#include <kernel/function.h>
#include <kernel/io/file_system.h>
#include <cpu/guest_thread.h>
#include <kernel/kernel.h>
#include <iostream>
#include <filesystem>


const uint32_t bufferSize = 0x10000; 


//todo
int32_t IDirectSoundBuffer_Lock(int32_t a1, int32_t dwOffset, int32_t dwBytes, LPVOID* ppvAudioPtr1, int32_t* pdwAudioBytes1, void** ppvAudioPtr2, int32_t* pdwAudioBytes2, uint32_t dwFlags)
{


	return 0; //success
}

//Xbox doesn't seem to use this todo check if we can still use it
int32_t IDirectSoundBuffer_Unlock(int32_t pHybridThis, void* ppvAudioPtr1, int32_t pdwAudioBytes1, void* ppvAudioPtr2, int32_t pdwAudioBytes2)
{
	return 0;
}

//todo
HRESULT IDirectSoundBuffer_GetStatus(int32_t buffer, DWORD* pdwStatus)
{

	*pdwStatus = 0x00010000;
	return S_OK;
}


//todo
int DirectSoundCreate_r(int* pcGuidDevice, int* ppDS, int* pUnkOuter)
{
	return 0;
}


//DirectSoundCreate
/**X86_FUNC_IMPL(__imp__sub_3E5A27);
X86_FUNC(sub_3E5A27)
{
	//TODO RE IMPLEMENT
	__imp__sub_3E5A27(ctx, base);

	auto Dsound = X86_LOAD_U32(0x550310);
	if (Dsound)
	{
		LOGN_UTIL("Successfully created DirectSound Buffer.");
	}
	else
	{
		LOGN_ERROR("Failed to create DirectSound Buffer.");
	}
}*/

//TODO
int32_t CDirectSound_DownloadEffectsImage(int* a1, LPCVOID pvImageBuffer, int32_t dwImageSize, void* pImageLoc, void** ppImageDesc)
{
	return 0;
}

int32_t IDirectSoundBuffer_GetCurrentPosition(int32_t a1, int32_t* CurrentPlayCursor, int32_t* pdwCurrentWriteCursor)
{
	return 0;
}

int32_t IDirectSoundBuffer_SetCurrentPosition(int32_t* pHybridThis, int32_t dwNewPosition)
{
	return 0;
}

int32_t IDirectSoundBuffer_Play(int32_t* a1, int32_t dwReserved1, int32_t dwReserved2, int32_t dwFlags)
{

	return 0;
}


int32_t IDirectSoundBuffer_Stop(int32_t a1)
{
	return 0;
}

//todo double check if it's 3 parameters or 4.
int32_t IDirectSoundBuffer_StopEx(int32_t a1, longlong_xt rtTimeStamp, dword_xt dwFlags)
{
	return 0;
}

int32_t IDirectSoundBuffer_SetVolume(int32_t this_, int32_t lVolume)
{
	return 0;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3E5A27, DirectSoundCreate_r)
GUEST_FUNCTION_HOOK_STDCALL(sub_3E421C, IDirectSoundBuffer_GetStatus);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4274, IDirectSoundBuffer_Lock);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E34B8, IDirectSoundBuffer_Unlock);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E35B5, CDirectSound_DownloadEffectsImage);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4238, IDirectSoundBuffer_GetCurrentPosition);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4258, IDirectSoundBuffer_SetCurrentPosition);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E419C, IDirectSoundBuffer_Play);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E41C0, IDirectSoundBuffer_Stop);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E41D8, IDirectSoundBuffer_StopEx);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E412C, IDirectSoundBuffer_SetVolume);