#include <audio/audio.h>
#include <cpu/guest_thread.h>
#include <kernel/heap.h>
#include <logs.h>
#include <kernel/function.h>
#include <audio/driver/sdl2_driver.h>

#include <iostream>
#include <fstream>
#include <DirectXMath.h>
#include <unordered_set>

// Memo: 
// Two way of using buffers: Static (sfx, voices - Buffer is filled once with complete audio, Pull just read the buffer) 
// Dynamic (Music... - Buffer starts empty or partially filled, game writes chunks continuously while playing, Pull read mWrittenSize)

static SDL_AudioDeviceID g_audioDevice{};

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define FORMAT_TO_SIZE(format) (SDL_AUDIO_BITSIZE(format) >> 3)
std::vector<Uint8> gResampleBuffer;


SHTHR_SOUNDDEVICE* ppDS = nullptr;

void Interface::Release()
{
	g_userHeap.Free(this);
}

class SHTHR_SOUNDDEVICE_Impl;

class SHTHR_SOUNDBUFFER_Impl : public SHTHR_SOUNDBUFFER
{
private:
	std::mutex mutex;
	std::vector<Uint8> mResampleBuffer;

	SHTHR_SOUNDDEVICE_Impl* mParent;
	SDL_AudioCVT mAudioCVT;

	unsigned char* mBuffer;
	bool mExternalBuffer; // track if buffer is externally owned

	int mBufferWriteLoc = 0;
	int mBufferReadLoc = 0;
	int mBufferSize = 0;
	int mWrittenSize = 0;
	int mCaps = 0;
	bool mPlay = false;


	int mChannelCount = 0;
	int mFrequency = 0;
	int mFormat = 0;

	int mVolume = 0;
	int mPan = 0;

	bool mLooping = false;
	int mLoopStartOffset = 0;
	int mLoopLength = 0;    // 0 = loop entire buffer

	//bool EffectVolume(unsigned char* data, int size);
	//void EffectPanning(unsigned char* data, int size);
	//bool Effect3D(unsigned char* data, int size);

public:

	SHTHR_SOUNDBUFFER_Impl(SHTHR_SOUNDDEVICE_Impl* parent, X_DSBUFFERDESC* desc);
	~SHTHR_SOUNDBUFFER_Impl();

	int Pull(unsigned char* data, int size);
	int GetStatus(uint32_t* pdwStatus) override;
	int GetCurrentPosition(int* pdwCurrentPlayCursor, int* pdwCurrentWriteCursor) override;
	//int Lock(int dwOffset, int dwBytes, void** ppvAudioPtr1, int* pdwAudioBytes1, void** ppvAudioPtr2, int* pdwAudioBytes2, int dwFlags) override;
	//int Unlock(void* pvAudioPtr1, int dwAudioBytes1, void* pvAudioPtr2, int dwAudioBytes2) override;
	int Play(int dwReserved1, int dwPriority, int dwFlags);
	int Stop() override;
	int SetVolume(int lVolume) override;
	//int SetPan(int lPan) override;
	int SetFrequency(int lFrequency) override;
	int SetBufferData(void* pvBufferData, int dwBufferBytes) override;
	int SetLoopRegion(int dwLoopStartOffset, int dwLoopLength) override;
};

class SHTHR_SOUNDDEVICE_Impl : public SHTHR_SOUNDDEVICE
{
public:
	std::mutex mutex;
	//SDL_mutex mutex;

	SDL_AudioDeviceID mDeviceID = 0;
	SDL_AudioSpec mAudioSpec;
	std::unordered_set<SHTHR_SOUNDBUFFER_Impl*> mAudioQueues;

	std::vector<unsigned char> read_buffer;
	Listener3D paramsListener;

	int mChannelCount = 0;
	int mFrequency = 0;
	int mFormat = 0;

	SHTHR_SOUNDDEVICE_Impl();
	~SHTHR_SOUNDDEVICE_Impl();
	void AudioCallback(Uint8* stream, int len);
	SHTHR_SOUNDBUFFER* CreateSoundBuffer(X_DSBUFFERDESC* ppDesc, SHTHR_SOUNDBUFFER** ppBuffer) override;
	bool ReleaseSoundBuffer(SHTHR_SOUNDBUFFER* pBuffer) override;
	int SetAllParameters(Listener3D* params) override;
};



SHTHR_SOUNDBUFFER_Impl::SHTHR_SOUNDBUFFER_Impl(SHTHR_SOUNDDEVICE_Impl* parent, X_DSBUFFERDESC* desc) : mParent(parent)
{
	printf("FormatTag: 0x%X\n", desc->lpwfxFormat->wFormatTag);
	printf("Game format: %d Hz, %d channels, bits: %d\n",
		desc->lpwfxFormat->nSamplesPerSec,
		desc->lpwfxFormat->nChannels,
		desc->lpwfxFormat->wBitsPerSample);

	printf("Device format: %d Hz, %d channels\n",
		mParent->mFrequency,
		mParent->mChannelCount);

	mCaps = desc->dwFlags;
	mChannelCount = desc->lpwfxFormat->nChannels;
	mFrequency = desc->lpwfxFormat->nSamplesPerSec; //desc->lpwfxFormat->nAvgBytesPerSec;
	mFormat = desc->lpwfxFormat->wFormatTag == 3 ? AUDIO_F32 : (0x8000 | desc->lpwfxFormat->wBitsPerSample);

	mBufferSize = desc->dwBufferBytes;
	mBuffer = (unsigned char*)malloc(mBufferSize);

	SDL_BuildAudioCVT(&mAudioCVT, // translator between what the game provides and what audio device expects. (ex: 22050 hs vs 41800)
		mFormat, mChannelCount, mFrequency,
		mParent->mFormat, mParent->mChannelCount, mParent->mFrequency);

	if (mCaps & XTL_DSBCAPS_CTRL3D)
	{
		//m3DBuffer = new SHTHR_SOUND3DBUFFER_Impl();
	}


	//mutex = SDL_CreateMutex();
}

SHTHR_SOUNDBUFFER_Impl::~SHTHR_SOUNDBUFFER_Impl()
{
	mutex.lock();

	if (mBuffer)
	{
		free(mBuffer);
		mBuffer = nullptr;
	}

	//SAFE_DELETE(m3DBuffer);
	mutex.unlock();

	mParent->ReleaseSoundBuffer(this);
}

SHTHR_SOUNDBUFFER* SHTHR_SOUNDDEVICE_Impl::CreateSoundBuffer(X_DSBUFFERDESC* ppDesc, SHTHR_SOUNDBUFFER** ppBuffer)
{
	if (!mDeviceID)
	{
		return nullptr;
	}

	if (!ppDesc || !ppBuffer)
	{
		return nullptr;
	}

	SHTHR_SOUNDBUFFER_Impl* buffer = g_userHeap.Alloc<SHTHR_SOUNDBUFFER_Impl>(this, ppDesc);

	if (!buffer)
	{
		return nullptr;
	}

	mutex.lock();
	mAudioQueues.insert(buffer);
	mutex.unlock();
	//SDL_UnlockMutex(mutex);


	return buffer;
}

int SHTHR_SOUNDBUFFER_Impl::SetBufferData(void* pvBufferData, int dwBufferBytes)
{
	std::lock_guard<std::mutex> lock(mutex);

	Stop();

	if (dwBufferBytes == 0)
	{
		return 0; 
	}

	// Free old internal buffer if we own it
	if (mBuffer && !mExternalBuffer) 
	{
		free(mBuffer);
		mBuffer = nullptr;
	}

	if (pvBufferData == nullptr) 
	{

		mBuffer = (unsigned char*)malloc(dwBufferBytes);
		if (!mBuffer) {
			return -1; // Out of memory
		}
		memset(mBuffer, 0, dwBufferBytes); // Clear it
		mExternalBuffer = false;
	}
	else  	// if not use external buffer
	{	
		mBuffer = (unsigned char*)pvBufferData;
		mExternalBuffer = true;
	}

	// Update size and reset positions
	mBufferSize = dwBufferBytes;
	mBufferReadLoc = 0;
	mBufferWriteLoc = 0;
	mWrittenSize = 0;

	// Rebuild audio converter if needed (in case format changed)
	SDL_BuildAudioCVT(&mAudioCVT,
		mFormat, mChannelCount, mFrequency,
		mParent->mFormat, mParent->mChannelCount, mParent->mFrequency);

	return 0; // Success
}

int SHTHR_SOUNDBUFFER_Impl::SetLoopRegion(int dwLoopStartOffset, int dwLoopLength)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (dwLoopStartOffset < 0 || dwLoopStartOffset >= mBufferSize) {
		return -1; 
	}

	if (dwLoopLength < 0 || dwLoopStartOffset + dwLoopLength > mBufferSize) {
		return -1; 
	}

	mLoopStartOffset = dwLoopStartOffset;
	mLoopLength = dwLoopLength;

	return 0;
}

// todo rework to add resampling
int SHTHR_SOUNDBUFFER_Impl::Pull(unsigned char* data, int size)
{
	std::lock_guard<std::mutex> lock(mutex);


	if (!mPlay || !mBuffer) {
		return 0;
	}


	int loopStart = mLoopStartOffset;
	int loopEnd;

	if (mLooping && mLoopLength > 0) {

		loopEnd = mLoopStartOffset + mLoopLength;
	}
	else 		// Loop entire buffer
	{

		loopStart = 0;
		loopEnd = mBufferSize;
	}

	int loopSize = loopEnd - loopStart;

	int read_size = std::min(size, loopEnd - mBufferReadLoc);

	if (read_size <= 0) {
		if (mLooping) {
	
			mBufferReadLoc = loopStart;
			read_size = std::min(size, loopSize);
		}
		else {
			// Reached end of buffer
			mPlay = false;
			return 0;
		}
	}

	// Handle wrapping within loop region
	if (mBufferReadLoc + read_size > loopEnd) 
	{
		int first_part = loopEnd - mBufferReadLoc;
		int second_part = read_size - first_part;

		memcpy(data, &mBuffer[mBufferReadLoc], first_part);

		if (mLooping)  	// Wrap to loop start
		{
		
			memcpy(data + first_part, &mBuffer[loopStart], second_part);
			mBufferReadLoc = loopStart + second_part;
		}
		else {

			mPlay = false;
			return first_part;
		}
	}
	else
	{
		memcpy(data, &mBuffer[mBufferReadLoc], read_size);
		mBufferReadLoc += read_size;

		// Check if we hit the loop end
		if (mBufferReadLoc >= loopEnd) {
			if (mLooping) {
				mBufferReadLoc = loopStart;
			}
			else {
				mPlay = false;
			}
		}
	}

	return read_size;
}

int SHTHR_SOUNDBUFFER_Impl::GetStatus(uint32_t* pdwStatus)
{
	int status = 0;

	if (mLooping)
	{
		status |= X_DSBSTATUS_LOOPING;
	}

	if (mPlay && mWrittenSize)
	{
		status |= X_DSBSTATUS_PLAYING;
	}

	//*pdwStatus = status;
	return status;
}

int SHTHR_SOUNDBUFFER_Impl::GetCurrentPosition(int* pdwCurrentPlayCursor, int* pdwCurrentWriteCursor)
{
	if (pdwCurrentPlayCursor)
	{
		*pdwCurrentPlayCursor = mBufferReadLoc;
	}

	if (pdwCurrentWriteCursor)
	{
		*pdwCurrentWriteCursor = mBufferWriteLoc;
	}

	return S_OK;
}

int SHTHR_SOUNDBUFFER_Impl::Play(int dwReserved1, int dwPriority, int dwFlags)
{
	std::lock_guard<std::mutex> lock(mutex);

	mPlay = true;
	if (dwFlags & X_DSBPLAY_LOOPING)
	{
		mLooping = true;
	}
	return S_OK;
}

int SHTHR_SOUNDBUFFER_Impl::Stop()
{
	mPlay = false;
	return S_OK;
}


int SHTHR_SOUNDBUFFER_Impl::SetVolume(int lVolume)
{
	return S_OK;
}

//todo
int SHTHR_SOUNDBUFFER_Impl::SetFrequency(int lFrequency)
{
	return E_FAIL;
}

SHTHR_SOUNDDEVICE_Impl::~SHTHR_SOUNDDEVICE_Impl()
{
	mutex.lock();
	if (mDeviceID)
	{
		SDL_CloseAudioDevice(mDeviceID);
		mDeviceID = 0;
	}

	mAudioQueues.clear();
	mutex.unlock();

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int SHTHR_SOUNDDEVICE_Impl::SetAllParameters(Listener3D* params)
{
	paramsListener = *params;
	return S_OK;
}


void SHTHR_SOUNDDEVICE_Impl::AudioCallback(Uint8* stream, int len)
{
	memset(stream, mAudioSpec.silence, len);

	if (mAudioQueues.size() == 0)
	{
		return;
	}

	read_buffer.resize(len);
	mutex.lock();

	for (auto& queue : mAudioQueues)
	{
		int readsize = queue->Pull(&read_buffer[0], len);
		if (readsize > 0)
		{
			if (readsize < len)
			{
				memset(&read_buffer[readsize], mAudioSpec.silence, len - readsize);
			}
			SDL_MixAudioFormat(stream, &read_buffer[0], mFormat, len, SDL_MIX_MAXVOLUME);
		}
	}
	mutex.unlock();
	//SDL_UnlockMutex(mutex);
}


static void AudioCallbackStatic(void* userdata, Uint8* stream, int len)
{
	SHTHR_SOUNDDEVICE_Impl* _this = (SHTHR_SOUNDDEVICE_Impl*)userdata;
	_this->AudioCallback(stream, len);
}


SHTHR_SOUNDDEVICE_Impl::SHTHR_SOUNDDEVICE_Impl()
{

#ifdef _WIN32
	// Force wasapi on Windows.
	SDL_setenv("SDL_AUDIODRIVER", "wasapi", true);
#endif

	SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "playback");
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, "Shadow Recompiled");

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		LOGFN_ERROR("Failed to init audio subsystem: {}", SDL_GetError());
		return;
	}


	SDL_AudioSpec desired{}, obtained{};
	desired.freq = XAUDIO_SAMPLES_HZ;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 2048;
	desired.userdata = this;
	desired.callback = AudioCallbackStatic;

	mDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desired, &mAudioSpec, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);

	if (mDeviceID)
	{
		mChannelCount = mAudioSpec.channels;
		mFrequency = mAudioSpec.freq;
		mFormat = mAudioSpec.format;

		SDL_PauseAudioDevice(mDeviceID, SDL_FALSE);
		printf("Init SDL2 Audio ok\n");
	}
}



//thread audio can be deleted after
int adxm_vsyncThread = -1;
static int adxm_fsThread = -1;
static int adxm_mwidlThread = -1;
static volatile bool g_audioThreadShouldExit = false;



#include <filesystem>
namespace fs = std::filesystem;

void AudioQuit()
{
	if (g_audioDevice)
	{
		SDL_CloseAudioDevice(g_audioDevice);
		SDL_Quit();
	}
}


bool SHTHR_SOUNDDEVICE_Impl::ReleaseSoundBuffer(SHTHR_SOUNDBUFFER* pBuffer)
{

	mutex.lock();
	mAudioQueues.erase((SHTHR_SOUNDBUFFER_Impl*)pBuffer);
	mutex.unlock();
	return true;
}

SHTHR_SOUNDDEVICE* SHTHRSoundCreate(SHTHR_SOUNDDEVICE** ppDevice)
{
	if (!ppDevice)
		return nullptr;

	SHTHR_SOUNDDEVICE* device = g_userHeap.AllocPhysical<SHTHR_SOUNDDEVICE_Impl>();

	ppDS = device;
	printf("created sound device\n");
	return device;
}



//adxm_mwidlThread
X86_FUNC_IMPL(__imp__sub_328E80);
X86_FUNC(sub_328E80)
{
	if (adxm_mwidlThread == -1)
	{
		adxm_mwidlThread = GetCurrentThreadId();
	}

	__imp__sub_328E80(ctx, base);
}

//adxm_fsThread
X86_FUNC_IMPL(__imp__sub_329260);
X86_FUNC(sub_329260)
{
	if (adxm_fsThread == -1)
	{
		adxm_fsThread = GetCurrentThreadId();
	}

	__imp__sub_329260(ctx, base);
}

//adxm_id_vsyncThread
X86_FUNC_IMPL(__imp__sub_3291E0);
X86_FUNC(sub_3291E0)
{
	if (adxm_vsyncThread == -1)
	{
		adxm_vsyncThread = GetCurrentThreadId();
	}

	__imp__sub_3291E0(ctx, base);
}