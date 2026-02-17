#include <stdafx.h>
#include <kernel/function.h>
#include <kernel/io/file_system.h>
#include <cpu/guest_thread.h>
#include <kernel/kernel.h>
#include <iostream>
#include <filesystem>
#include <audio/audio.h>
#include <audio/driver/sdl2_driver.h>


HRESULT DirectSoundCreate_r(int* pcGuidDevice, SHTHR_SOUNDDEVICE* ppDS, int* pUnkOuter)
{

	auto device = SHTHRSoundCreate(&ppDS);

	if (device == nullptr)
		return E_FAIL;

	auto ctx = GetX86Context();
	auto base = g_memory.base;
	X86_STORE_U32(ppDSAddr, g_memory.MapVirtual(device));
	return S_OK;
}

//todo
int32_t IDirectSoundBuffer_Lock(SHTHR_SOUNDBUFFER* pThis, int32_t dwOffset, int32_t dwBytes, LPVOID* ppvAudioPtr1, int32_t* pdwAudioBytes1, void** ppvAudioPtr2, int32_t* pdwAudioBytes2, uint32_t dwFlags)
{


	return 0; //success
}

//Xbox doesn't seem to use this todo check if we can still use it
int32_t IDirectSoundBuffer_Unlock(SHTHR_SOUNDBUFFER* pThis, void* ppvAudioPtr1, int32_t pdwAudioBytes1, void* ppvAudioPtr2, int32_t pdwAudioBytes2)
{
	return 0;
}


HRESULT IDirectSoundBuffer_GetStatus(SHTHR_SOUNDBUFFER* pThis, uint32_t* pdwStatus)
{

	if (!pThis)
		return E_FAIL;

	auto res = pThis->GetStatus(pdwStatus);


	auto ctx = GetX86Context();
	auto base = g_memory.base;

	auto ptr = pdwStatus;

	X86_STORE_U32((uint32_t)pdwStatus, res);

	return S_OK;
}


//TODO(?)
int32_t CDirectSound_DownloadEffectsImage(int* a1, LPCVOID pvImageBuffer, int32_t dwImageSize, void* pImageLoc, void** ppImageDesc)
{
	return E_FAIL;
}

int32_t IDirectSoundBuffer_GetCurrentPosition(SHTHR_SOUNDBUFFER* pThis, int32_t* CurrentPlayCursor, int32_t* pdwCurrentWriteCursor)
{
	if (!pThis)
		return E_FAIL;

	return pThis->GetCurrentPosition(CurrentPlayCursor, pdwCurrentWriteCursor);
}

int32_t IDirectSoundBuffer_SetCurrentPosition(SHTHR_SOUNDBUFFER* pThis, int32_t dwNewPosition)
{
	return 0;
}


int32_t IDirectSound_CreateSoundBuffer(SHTHR_SOUNDDEVICE* pThis, X_DSBUFFERDESC* pdsbd, SHTHR_SOUNDBUFFER** ppBufferOut, int a4)
{

	auto ctx = GetX86Context();
	auto base = g_memory.base;

	auto buffer = pThis->CreateSoundBuffer(pdsbd, ppBufferOut);


	if (buffer)
	{
		ctx->esp.u32 += 20;
		X86_STORE_U32(ctx->esp.u32 + 12, g_memory.MapVirtual(buffer));
		ctx->esp.u32 -= 20;
		return S_OK;
	}


	return E_FAIL;
}

//This doesn't seem to exist on PC originally, it looks like it replace the buffer’s entire PCM data with this memory.
int32_t IDirectSoundBuffer_SetBufferData(SHTHR_SOUNDBUFFER* pThis, void* pvBufferData, uint32_t dwBufferBytes)
{
	if (!pThis)
		return E_FAIL;

	return pThis->SetBufferData(pvBufferData, dwBufferBytes);
}

int32_t IDirectSoundBuffer_Play(SHTHR_SOUNDBUFFER* pThis, int32_t dwReserved1, int32_t dwReserved2, int32_t dwFlags)
{
	if (!pThis)
		return E_FAIL;

	return pThis->Play(dwReserved1, dwReserved2, dwFlags);
}


int32_t IDirectSoundBuffer_Stop(SHTHR_SOUNDBUFFER* pThis)
{
	if (!pThis)
		return E_FAIL;

	return pThis->Stop();
}


//todo implementation might be more complex for this variation...
int32_t IDirectSoundBuffer_StopEx(SHTHR_SOUNDBUFFER* pThis, longlong_xt rtTimeStamp, dword_xt dwFlags)
{
	auto ctx = GetX86Context();


	if (!pThis)
		return E_FAIL;

	ctx->esp.u32 += 4; // rtTimeStamp is a 64 bits parameter so the compiler split that into 4 parameters
	return pThis->Stop();
}

int32_t IDirectSoundBuffer_Release(SHTHR_SOUNDBUFFER* pThis)
{
	pThis->Stop();
	pThis->Release();
	return S_OK;
}

int32_t IDirectSoundBuffer_SetVolume(SHTHR_SOUNDBUFFER* pThis, int32_t lVolume)
{
	if (!pThis)
		return E_FAIL;

	return pThis->SetVolume(lVolume);
}

int32_t IDirectSoundBuffer_SetLoopRegion(SHTHR_SOUNDBUFFER* pThis, int32_t dwLoopStart, int32_t dwLoopLength)
{
	if (!pThis)
		return E_FAIL;

	return pThis->SetLoopRegion(dwLoopStart, dwLoopLength);
}

int32_t IDirectSound_SetAllParameters(SHTHR_SOUNDDEVICE* this_, Listener3D* pDS3DListenerParameters, char dwApply)
{
	pDS3DListenerParameters = pDS3DListenerParameters;
	this_->SetAllParameters(pDS3DListenerParameters);
	return 0;
}

//to do
int32_t IDirectSoundBuffer_SetMinDistance(SHTHR_SOUNDBUFFER* pThis, float flMinDistance, int32_t dwApply)
{
	return 1;
}

int IDirectSoundBuffer_SetMixBinVolumes_8(SHTHR_SOUNDBUFFER* pThis, int a2)
{
	return 1;
}

int32_t IDirectSoundBuffer_SetMaxDistance(SHTHR_SOUNDBUFFER* pThis, float flMaxDistance, int32_t dwApply)
{
	return 1;
}

int32_t IDirectSoundBuffer_SetMixBins(SHTHR_SOUNDBUFFER* pThis, int mixBins)
{
	return 1;
}

int32_t IDirectSoundBuffer_SetConeAngles(SHTHR_SOUNDBUFFER* pThis, int a2, int a3, int a4)
{
	return 1;
}

//todo
int32_t IDirectSoundBuffer_SetPitch(SHTHR_SOUNDBUFFER* pThis, int a2)
{
	return 0;
}

//todo
int IDirectSoundBuffer_SetHeadroom(SHTHR_SOUNDBUFFER* pThis, int dwHeadroom)
{
	return 0;
}

int IDirectSoundBuffer_SetFrequency(SHTHR_SOUNDBUFFER* pThis, int frequency)
{
	if (!pThis)
		return FALSE;

	return pThis->SetFrequency(frequency);
}



//====== IDIRECT SOUND FUNCTIONS ==========

//stub
int32_t IDirectSound_SetI3DL2Listener(int ppDS_, int a2, char a3)
{
	return E_FAIL;
}

//todo(?)
int32_t IDirectSound_SetDistanceFactor(int32_t pThis, float fDistanceFactor, int32_t dwApply)
{
	return E_FAIL;
}

//todo(?)
int32_t IDirectSound_SetRolloffFactor(int32_t pThis, float RolloffFactor, int32_t dwApply)
{
	return E_FAIL;
}

//todo
int32_t IDirectSound_SetPosition(int pThis, float x, float y, float z, int dwApply)
{
	return E_FAIL;
}

//todo(?)
int32_t IDirectSound_SetOrientation(int a1, float xFront, float yFront, float zFront, float xTop, float yTop, float zTop, int dwApply)
{
	return E_FAIL;
}

int32_t CDirectSound_CommitDeferredSettings(int a1)
{
	return E_FAIL;
}

int32_t IDirectSound_CommitDeferredSettings(int a1)
{
	return E_FAIL;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3E5A27, DirectSoundCreate_r)
GUEST_FUNCTION_HOOK_STDCALL(sub_3E5756, IDirectSound_CreateSoundBuffer);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4EE9, IDirectSoundBuffer_SetBufferData);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E421C, IDirectSoundBuffer_GetStatus);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4274, IDirectSoundBuffer_Lock);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E34B8, IDirectSoundBuffer_Unlock);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E35B5, CDirectSound_DownloadEffectsImage);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4238, IDirectSoundBuffer_GetCurrentPosition);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4258, IDirectSoundBuffer_SetCurrentPosition);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E419C, IDirectSoundBuffer_Play);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E41C0, IDirectSoundBuffer_Stop);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E41D8, IDirectSoundBuffer_StopEx);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E34BD, IDirectSoundBuffer_Release);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E412C, IDirectSoundBuffer_SetVolume);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E41FC, IDirectSoundBuffer_SetLoopRegion);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4148, IDirectSoundBuffer_SetPitch);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4164, IDirectSoundBuffer_SetHeadroom);

//3D Sound
GUEST_FUNCTION_HOOK_STDCALL(sub_3E497A, IDirectSoundBuffer_SetMinDistance);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4180, IDirectSoundBuffer_SetMixBinVolumes_8);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4956, IDirectSoundBuffer_SetMaxDistance);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4932, IDirectSoundBuffer_SetConeAngles);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E577A, IDirectSound_SetAllParameters);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E5861, IDirectSound_SetI3DL2Listener);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E579A, IDirectSound_SetDistanceFactor);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E583D, IDirectSound_SetRolloffFactor);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E5808, IDirectSound_SetPosition);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E57BE, IDirectSound_SetOrientation);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E52DC, IDirectSound_CommitDeferredSettings);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4FB4, CDirectSound_CommitDeferredSettings);

