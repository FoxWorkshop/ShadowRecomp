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

	if (device <= 0)
		return E_FAIL;

	auto ctx = GetX86Context();
	auto base = g_memory.base;
	X86_STORE_U32(ppDSAddr, device);
	return S_OK;
}


int32_t IDirectSoundBuffer_Lock(int32_t keyBuffer, int32_t dwOffset, int32_t dwBytes, uint32_t ppvAudioPtr1, uint32_t pdwAudioBytes1, uint32_t ppvAudioPtr2, uint32_t pdwAudioBytes2, uint32_t dwFlags)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	// Host-side temporary variables to receive the lock results
	void* hostPpvAudioPtr1 = nullptr;
	int32_t hostpdwAudioBytes1 = 0;
	void* hostPpvAudioPtr2 = nullptr;
	int32_t hostpdwAudioBytes2 = 0;

	auto res = pThis->Lock(dwOffset, dwBytes, &hostPpvAudioPtr1, &hostpdwAudioBytes1, &hostPpvAudioPtr2, &hostpdwAudioBytes2, dwFlags);

	if (res != 0)
		return res;

	auto base = g_memory.base;

	X86_STORE_U32(ppvAudioPtr1, g_memory.MapVirtual(hostPpvAudioPtr1));
	X86_STORE_U32(pdwAudioBytes1, hostpdwAudioBytes1);
	X86_STORE_U32(ppvAudioPtr2, g_memory.MapVirtual(hostPpvAudioPtr2));
	X86_STORE_U32(pdwAudioBytes2, hostpdwAudioBytes2);
	return res;
}

//Xbox doesn't seem to use this todo check if we can still use it
int32_t IDirectSoundBuffer_Unlock(int32_t keyBuffer, void* ppvAudioPtr1, int32_t pdwAudioBytes1, void* ppvAudioPtr2, int32_t pdwAudioBytes2)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	return pThis->Unlock(ppvAudioPtr1, pdwAudioBytes1, ppvAudioPtr2, pdwAudioBytes2);
}


HRESULT IDirectSoundBuffer_GetStatus(int32_t keyBuffer, uint32_t* pdwStatus)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	auto res = pThis->GetStatus(pdwStatus);
	auto base = g_memory.base;
	auto ptr = pdwStatus;

	X86_STORE_U32(g_memory.MapVirtual(pdwStatus), res);
	return S_OK;
}


//TODO(?)
int32_t CDirectSound_DownloadEffectsImage(int* a1, LPCVOID pvImageBuffer, int32_t dwImageSize, void* pImageLoc, void** ppImageDesc)
{
	return E_FAIL;
}

int32_t IDirectSoundBuffer_GetCurrentPosition(int32_t keyBuffer, int32_t CurrentPlayCursor, int32_t pdwCurrentWriteCursor)
{

	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	int32_t hostCurrentPlayCursor = 0;
	int32_t hostpdwCurrentWriteCursor = 0;

	auto res = pThis->GetCurrentPosition(&hostCurrentPlayCursor, &hostpdwCurrentWriteCursor);
	auto base = g_memory.base;
	X86_STORE_U32(CurrentPlayCursor, hostCurrentPlayCursor);
	X86_STORE_U32(pdwCurrentWriteCursor, hostpdwCurrentWriteCursor);
	return res;
}

int32_t IDirectSoundBuffer_SetCurrentPosition(int32_t keyBuffer, int32_t dwNewPosition)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	return pThis->SetCurrentPosition(dwNewPosition);
}


int32_t IDirectSound_CreateSoundBuffer(int32_t keyDevice, X_DSBUFFERDESC* pdsbd, SHTHR_SOUNDBUFFER** ppBufferOut, int a4)
{

	auto ctx = GetX86Context();
	auto base = g_memory.base;

	auto pThis = ppDS;

	if (!pThis)
		return E_FAIL;

	if (pdsbd && pdsbd->dwFlags & XTL_DSBCAPS_CTRL3D) //3D sound aren't supported yet todo implement
	{
		return E_FAIL;

	}

	auto bufferKey = pThis->CreateSoundBuffer(pdsbd, ppBufferOut);


	if (bufferKey)
	{

		X86_STORE_U32(g_memory.MapVirtual(ppBufferOut), bufferKey);

		return S_OK;
	}

	return E_FAIL;
}

//This doesn't seem to exist on PC originally, it looks like it replace the buffer’s entire PCM data with this memory.
int32_t IDirectSoundBuffer_SetBufferData(int32_t key, void* pvBufferData, uint32_t dwBufferBytes)
{
	auto pThis = GetCurrentSoundBuffer(key);

	if (!pThis)
		return E_FAIL;

	return pThis->SetBufferData(pvBufferData, dwBufferBytes);
}

int32_t IDirectSoundBuffer_Play(int32_t key, int32_t dwReserved1, int32_t dwReserved2, int32_t dwFlags)
{
	auto pThis = GetCurrentSoundBuffer(key);

	if (!pThis)
		return E_FAIL;

	return pThis->Play(dwReserved1, dwReserved2, dwFlags);
}


int32_t IDirectSoundBuffer_Stop(int32_t keyDevice)
{
	auto pThis = GetCurrentSoundBuffer(keyDevice);

	if (!pThis)
		return E_FAIL;

	return pThis->Stop();
}


//this function had originally 3 params, but ended being 4 by the compiler due to the use of int 64 bits.
int32_t IDirectSoundBuffer_StopEx(int32_t key, uint32_t rtTimeStamp_Low, uint32_t rtTimeStamp_High, dword_xt dwFlags)
{
	auto ctx = GetX86Context();

	auto pThis = GetCurrentSoundBuffer(key);

	if (!pThis)
		return E_FAIL;

	// Recombine into 64-bit value
	int64_t rtTimeStamp = (static_cast<int64_t>(rtTimeStamp_High) << 32) | rtTimeStamp_Low;

	return pThis->StopEx(rtTimeStamp, dwFlags);
}

int32_t IDirectSoundBuffer_Release(int32_t keyBuffer)
{

	auto pDevice = ppDS;

	if (!pDevice)
		return E_FAIL;

	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	pThis->Stop();

	pDevice->ReleaseSoundBuffer(pThis);

	delete pThis;

	return S_OK;
}

int32_t IDirectSoundBuffer_SetVolume(int32_t keyDevice, int32_t lVolume)
{
	auto pThis = GetCurrentSoundBuffer(keyDevice);

	if (!pThis)
		return E_FAIL;

	return pThis->SetVolume(lVolume);
}

//todo
int IDirectSoundBuffer_SetPosition(int32_t keyBuffer, float a2, float a3, float a4, int a5)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	return E_FAIL;
}

int32_t IDirectSoundBuffer_SetLoopRegion(int32_t keyBuffer, int32_t dwLoopStart, int32_t dwLoopLength)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	return pThis->SetLoopRegion(dwLoopStart, dwLoopLength);
}

int32_t IDirectSound_SetAllParameters(int32_t keyDevice, Listener3D* pDS3DListenerParameters, char dwApply)
{
	auto pThis = ppDS;

	if (!pThis)
		return E_FAIL;

	pThis->SetAllParameters(pDS3DListenerParameters);
	return 0;
}

//to do
int32_t IDirectSoundBuffer_SetMinDistance(int32_t keyBuffer, float flMinDistance, int32_t dwApply)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	return E_FAIL;
}

int IDirectSoundBuffer_SetMixBinVolumes_8(int32_t keyBuffer, int a2)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	return E_FAIL;
}

int32_t IDirectSoundBuffer_SetMaxDistance(int32_t keyBuffer, float flMaxDistance, int32_t dwApply)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	return E_FAIL;
}

int32_t IDirectSoundBuffer_SetMixBins(int32_t keyBuffer, int mixBins)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

	return E_FAIL;
}

int32_t IDirectSoundBuffer_SetConeAngles(SHTHR_SOUNDBUFFER* pThis, int a2, int a3, int a4)
{

	return E_FAIL;
}

//todo
int32_t IDirectSoundBuffer_SetPitch(SHTHR_SOUNDBUFFER* pThis, int a2)
{
	return E_FAIL;
}

//todo
int IDirectSoundBuffer_SetHeadroom(SHTHR_SOUNDBUFFER* pThis, int dwHeadroom)
{
	return E_FAIL;
}

int IDirectSoundBuffer_SetFrequency(int32_t keyBuffer, int frequency)
{
	auto pThis = GetCurrentSoundBuffer(keyBuffer);

	if (!pThis)
		return E_FAIL;

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
	//printf("IDirectSound_SetPosition\n");
	return E_FAIL;
}

//todo(?)
int32_t IDirectSound_SetOrientation(int a1, float xFront, float yFront, float zFront, float xTop, float yTop, float zTop, int dwApply)
{
	//printf("IDirectSound_SetOrientation\n");
	return E_FAIL;
}

int32_t CDirectSound_CommitDeferredSettings(int a1)
{
	return E_FAIL;
}

int32_t IDirectSound_CommitDeferredSettings(int a1)
{
	//printf("IDirectSound_CommitDeferredSettings\n");
	return E_FAIL;
}

// needed to make the game load adx files
// should implement a better emulation later
void BlockUntilVerticalBlank()
{
	std::this_thread::sleep_for(std::chrono::microseconds(16667));
}

GUEST_FUNCTION_HOOK(sub_3CFE70, BlockUntilVerticalBlank);

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
GUEST_FUNCTION_HOOK_STDCALL(sub_3E48FA, IDirectSoundBuffer_SetFrequency);

//3D Sound
GUEST_FUNCTION_HOOK_STDCALL(sub_3E497A, IDirectSoundBuffer_SetMinDistance);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4180, IDirectSoundBuffer_SetMixBinVolumes_8);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4916, IDirectSoundBuffer_SetMixBins);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4956, IDirectSoundBuffer_SetMaxDistance);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4932, IDirectSoundBuffer_SetConeAngles);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E499E, IDirectSoundBuffer_SetCurrentPosition);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E577A, IDirectSound_SetAllParameters);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E5861, IDirectSound_SetI3DL2Listener);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E579A, IDirectSound_SetDistanceFactor);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E583D, IDirectSound_SetRolloffFactor);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E5808, IDirectSound_SetPosition);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E57BE, IDirectSound_SetOrientation);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E52DC, IDirectSound_CommitDeferredSettings);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4FB4, CDirectSound_CommitDeferredSettings);

