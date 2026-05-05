#include <audio/audio.h>
#include <cpu/guest_thread.h>
#include <kernel/heap.h>

#include <kernel/function.h>
#include <audio/driver/sdl2_driver.h>
#include <audio/xadpcm.h>

#include <iostream>
#include <fstream>
#include <DirectXMath.h>
#include <unordered_set>
#include <os/logger.h>
#include <user/config.h>

// Memo: 
// Two way of using buffers: Static (sfx, voices - Buffer is filled once with complete audio, Pull just read the buffer) 
// Dynamic (Music... - Buffer starts empty or partially filled, game writes chunks continuously while playing, Pull read mWrittenSize)
// Research shows that sound effect use XADPCM, a format for Xbox that need to be decoded, while music likely use PCM directly.
// We need to use host memory because the classes size can grow dynamically, but still need to trick the game that pointer exist with guest memory.

#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#define FORMAT_TO_SIZE(format) (SDL_AUDIO_BITSIZE(format) >> 3)
#define DSE_FLAG_XADPCM                 (1 << 1)
#define DSoundBufferGetPCMBufferSize(EmuFlags, size) (EmuFlags & DSE_FLAG_XADPCM) > 0 ? DWORD((size / float(XBOX_ADPCM_SRCSIZE)) * XBOX_ADPCM_DSTSIZE) : size
#define DSoundBufferGetXboxBufferSize(EmuFlags, size) (EmuFlags & DSE_FLAG_XADPCM) > 0 ? DWORD((size / float(XBOX_ADPCM_DSTSIZE)) * XBOX_ADPCM_SRCSIZE) : size

bool PlayMusic = false;
SHTHR_SOUNDDEVICE* ppDS = nullptr;
static volatile bool g_audioThreadShouldExit;

void AudioShutDown()
{
	g_audioThreadShouldExit = true;
}

void Interface::Release()
{
	delete this;
}


class SHTHR_SOUNDDEVICE_Impl;

class SHTHR_SOUND3DBUFFER_Impl : public SHTHR_SOUND3DBUFFER
{
private:
	float mMaxDistance = 1000000000.0f;
	float mMinDistance = 1.0f;
	float mPosition[3] = { 0.f, 0.f, 0.f };
public:

	int SetMaxDistance(float flMaxDistance, int dwApply) override;
	int SetMinDistance(float flMinDistance, int dwApply) override;
	int GetPosition(float* pvPosition) override;
	int SetPosition(float x, float y, float z, int dwApply) override;
	int GetMaxDistance(float* pflMaxDistance) override;
	int GetMinDistance(float* pflMinDistance) override;
};


class SHTHR_SOUNDBUFFER_Impl : public SHTHR_SOUNDBUFFER
{
private:
	std::mutex mutex;
	SDL_AudioStream* mAudioStream = nullptr;
	std::vector<Uint8> mResampleBuffer;
	std::vector<Uint8> mOutputBuffer;

	SHTHR_SOUNDDEVICE_Impl* mParent;
	SDL_AudioCVT mAudioCVT;

	struct PendingWrite {
		int offset;
		int bytes;
		bool consumed;
	};

	PendingWrite mPendingWrite{};
	unsigned char* mBuffer;
	bool mExternalBuffer = false; // track if buffer is externally owned
	DWORD mX_BufferCacheSize;

	bool isStreaming = false;
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
	int mVolumeLinear = 128;
	int mPrevVolume = 0;
	bool mVolumeChangeFlag = false; // ensure we don't scale volume multiple times

	DWORD mPlayFlags = 0;
	DWORD mG_flags;

	bool mLooping = false;
	int mLoopStartOffset = 0;
	int mLoopLength = 0;    // 0 = loop entire buffer
	int mBakedLoopPos = 0;

	int32_t keyBuffer;


	enum SourceFormat {
		SOURCE_FORMAT_PCM,
		SOURCE_FORMAT_ADPCM_XBOX,
		SOURCE_FORMAT_FLOAT
	};

	SourceFormat mSourceFormat;


public:

	SHTHR_SOUNDBUFFER_Impl(SHTHR_SOUNDDEVICE_Impl* parent, X_DSBUFFERDESC* desc);
	~SHTHR_SOUNDBUFFER_Impl();

	PullResult Pull(unsigned char* data, int32_t size);
	int GetStatus(uint32_t* pdwStatus) override;
	int GetCurrentPosition(int* pdwCurrentPlayCursor, int* pdwCurrentWriteCursor) override;
	int SetCurrentPosition(DWORD dwNewPosition) override;
	int Lock(int dwOffset, int dwBytes, void** ppvAudioPtr1, int* pdwAudioBytes1, void** ppvAudioPtr2, int* pdwAudioBytes2, int dwFlags) override;
	int Unlock(void* pvAudioPtr1, int dwAudioBytes1, void* pvAudioPtr2, int dwAudioBytes2) override;
	int Play(int dwReserved1, int dwPriority, uint32_t dwFlags) override;
	int Stop() override;
	int StopEx(int64_t rtTimeStamp, uint32_t dwFlags) override;
	int SetVolume(int lVolume) override;
	int GetVolumeLinear() const { return mVolumeLinear; }

	int SetFrequency(int lFrequency) override;
	int SetBufferData(void* pvBufferData, int dwBufferBytes) override;
	int SetLoopRegion(int dwLoopStartOffset, int dwLoopLength) override;
	int GetCaps() const { return mCaps; }

	void SetFormatTag(xpointer<WAVEFORMATEX> format);
	void SetBufferKey(int32_t key);
	void RebuildStream(SDL_AudioFormat srcFormat, int srcChannels, int srcFreq,
		SDL_AudioFormat dstFormat, int dstChannels, int dstFreq);
	void StopSfxEarly();

	float Calculate3DVolume();
	int mSoundID = 0; // for debugging
};

class SHTHR_SOUNDDEVICE_Impl : public SHTHR_SOUNDDEVICE
{
private:
	SDL_Thread* mPullThread = nullptr;
	SDL_atomic_t mPullThreadRunning;
	static int PullThreadStatic(void* userdata);
	int PullThreadFunc();

	// Tuning 
	static constexpr int TARGET_QUEUE_BYTES = 8192;
	static constexpr int FILL_CHUNK_BYTES = 2048;
	static constexpr int SLEEP_FULL_MS = 5;
	static constexpr int SLEEP_EMPTY_MS = 1;

	float mListenerPosition[3] = { 0, 0, 0 };
	float mDistanceFactor = 1.0f;  // Default: (no scaling)
	float mRolloffFactor = 1.0f; // Default: (realistic)

public:
	std::mutex mutex;

	SDL_AudioDeviceID mDeviceID = 0;
	SDL_AudioSpec mAudioSpec;
	std::unordered_set<SHTHR_SOUNDBUFFER_Impl*> mAudioQueues;

	std::vector<unsigned char> read_buffer;
	Listener3D paramsListener;

	int32_t mChannelCount = 0;
	int32_t mFrequency = 0;
	int32_t mFormat = 0;

	SHTHR_SOUNDDEVICE_Impl();
	~SHTHR_SOUNDDEVICE_Impl();
	void AudioCallback(Uint8* stream, int32_t len);
	int32_t CreateSoundBuffer(X_DSBUFFERDESC* ppDesc, SHTHR_SOUNDBUFFER** ppBuffer) override;
	bool ReleaseSoundBuffer(SHTHR_SOUNDBUFFER* pBuffer) override;
	int SetAllParameters(Listener3D* params) override;
	int SetRolloffFactor(float flRolloffFactor, DWORD dwApply) override;
	int SetDistanceFactor(float flDistanceFactor, DWORD dwApply) override;
	int SetPosition(float x, float y, float z, DWORD dwApply) override;
	float GetRolloffFactor() const { return mRolloffFactor; }
	float GetDistanceFactor() const { return mDistanceFactor; }
	float* GetListenerPosition() { return mListenerPosition; }
};

void SHTHR_SOUNDBUFFER_Impl::SetBufferKey(int32_t key)
{
	keyBuffer = key;
}

void SHTHR_SOUNDBUFFER_Impl::SetFormatTag(xpointer<WAVEFORMATEX> format)
{

	if (format.get() != nullptr)
	{
		WORD bitsPerSample = format->wBitsPerSample;

		switch (format->wFormatTag)
		{
		case WAVE_FORMAT_PCM:
			this->mSourceFormat = SOURCE_FORMAT_PCM;
			this->mG_flags |= DSE_FLAG_PCM;

			switch (bitsPerSample)
			{
			case 8:
				this->mFormat = AUDIO_U8;
				break;
			case 16:
			default:
				this->mFormat = AUDIO_S16;
				break;
			case 24:
			case 32:
				this->mFormat = AUDIO_S32;
				break;
			}

			break;
		case WAVE_FORMAT_XBOX_ADPCM:
			this->mG_flags |= DSE_FLAG_XADPCM;
			this->mSourceFormat = SOURCE_FORMAT_ADPCM_XBOX;
			this->mFormat = AUDIO_S16;
			break;
		default:
			this->mG_flags |= DSE_FLAG_PCM_UNKNOWN;
			this->mFormat = AUDIO_S16;
			break;

		}
	}
}

int SHTHR_SOUND3DBUFFER_Impl::GetPosition(float* pvPosition)
{
	if (!pvPosition)
	{
		return DSERR_INVALIDPARAM;
	}

	pvPosition[0] = mPosition[0];
	pvPosition[1] = mPosition[1];
	pvPosition[2] = mPosition[2];
	return S_OK;
}

int SHTHR_SOUND3DBUFFER_Impl::SetMaxDistance(float flMaxDistance, int dwApply)
{
	mMaxDistance = flMaxDistance;
	return S_OK;
}

int SHTHR_SOUND3DBUFFER_Impl::SetMinDistance(float flMinDistance, int dwApply)
{

	mMinDistance = flMinDistance;
	return S_OK;
}

int SHTHR_SOUND3DBUFFER_Impl::SetPosition(float x, float y, float z, int dwApply)
{
	mPosition[0] = x;
	mPosition[1] = y;
	mPosition[2] = z;
	return S_OK;
}

int SHTHR_SOUND3DBUFFER_Impl::GetMaxDistance(float* pflMaxDistance)
{
	if (!pflMaxDistance)
	{
		return DSERR_INVALIDPARAM;
	}

	*pflMaxDistance = mMaxDistance;
	return S_OK;
}

int SHTHR_SOUND3DBUFFER_Impl::GetMinDistance(float* pflMinDistance)
{
	if (!pflMinDistance)
	{
		return DSERR_INVALIDPARAM;
	}

	*pflMinDistance = mMinDistance;
	return S_OK;
}

float SHTHR_SOUNDBUFFER_Impl::Calculate3DVolume()
{
	float vecPos[3];
	this->m3DBuffer->GetPosition(vecPos);

	float mMinDistance = 1.0f;
	this->m3DBuffer->GetMinDistance(&mMinDistance);

	float mMaxDistance = 1000000000.0f;
	this->m3DBuffer->GetMaxDistance(&mMaxDistance);

	float* listenerPos = mParent->GetListenerPosition();


	float posX = vecPos[0] - listenerPos[0];
	float posY = vecPos[1] - listenerPos[1];
	float posZ = vecPos[2] - listenerPos[2];


	float distance = sqrtf(posX * posX + posY * posY + posZ * posZ);

	//float distanceFactor = mParent->GetDistanceFactor();
	float rolloff = mParent->GetRolloffFactor();

	// DirectSound inverse distance model
	// float scaledDistance = distance * distanceFactor;
	float dist_clamped = std::max(mMinDistance, std::min(distance, mMaxDistance));
	float attenuation = mMinDistance / (mMinDistance + rolloff * (dist_clamped - mMinDistance));

	if (attenuation > 0.8f)
		attenuation = 0.8f;

	return attenuation;
}


SHTHR_SOUNDBUFFER_Impl::SHTHR_SOUNDBUFFER_Impl(SHTHR_SOUNDDEVICE_Impl* parent, X_DSBUFFERDESC* desc) : mParent(parent)
{
	SetFormatTag(desc->lpwfxFormat);
	mCaps = desc->dwFlags;
	mChannelCount = desc->lpwfxFormat->nChannels;
	mFrequency = desc->lpwfxFormat->nSamplesPerSec;
	memset(&mAudioCVT, 0, sizeof(mAudioCVT));

	mBufferSize = desc->dwBufferBytes;
	mBuffer = (unsigned char*)g_userHeap.Alloc(mBufferSize);

	// translator between what the game provides and what audio device expects. (ex: 22050 hs vs 41800)
	RebuildStream(mFormat, mChannelCount, mFrequency, mParent->mFormat, mParent->mChannelCount, mParent->mFrequency);


	if (mCaps & XTL_DSBCAPS_CTRL3D)
	{
		m3DBuffer = new SHTHR_SOUND3DBUFFER_Impl();
	}
}

std::unordered_map<int32_t, SHTHR_SOUNDBUFFER*> SoundBufferList;

SHTHR_SOUNDBUFFER_Impl::~SHTHR_SOUNDBUFFER_Impl()
{
	mPlay = false;

	if (mBuffer && !mExternalBuffer)
	{
		g_userHeap.Free(mBuffer);
		mBuffer = nullptr;
	}

	auto guestMemory = g_memory.Translate(keyBuffer);
	if (guestMemory)
	{
		g_userHeap.Free(guestMemory);
	}

	SoundBufferList.erase(keyBuffer);
}


SHTHR_SOUNDBUFFER* GetCurrentSoundBuffer(const int32_t key)
{
	return SoundBufferList[key];
}

static int globalSoundID = 0;
int32_t SHTHR_SOUNDDEVICE_Impl::CreateSoundBuffer(X_DSBUFFERDESC* ppDesc, SHTHR_SOUNDBUFFER** ppBuffer)
{
	if (!mDeviceID)
	{
		return E_FAIL;
	}

	if (!ppDesc || !ppBuffer)
	{
		return E_FAIL;
	}

	// Since the sound classes have size that can grow dynamically due to vector and other stuff, we need to use host memory here
	// otherwise guest memory will overwrite some region that the class will take after growing.
	// but even if we use host memory, the guest memory still need to hold the pointer to pass it through different sounds function
	// sadly we can't just pass a host pointer to guest just like that, that will create issues due to region difference and 64 vs 32 bits moment.
	// as a workaround, we can store the sound buffers in a map and give the key to the guest in a valid 32 bits address.

	SHTHR_SOUNDBUFFER_Impl* buffer = new SHTHR_SOUNDBUFFER_Impl(this, ppDesc);
	if (!buffer)
	{
		return -1;
	}

	//alloc memory for the guest so it can hold the key of the map for this sound buffer
	auto ptr = g_userHeap.Alloc(sizeof(int));
	if (!ptr)
		return -1;

	auto key = g_memory.MapVirtual(ptr);
	SoundBufferList[key] = buffer;
	buffer->SetBufferKey(key);
	buffer->mSoundID = globalSoundID++;

	mutex.lock();
	mAudioQueues.insert(buffer);
	mutex.unlock();

	return key; //give the key to the guest so it won't hold directly the pointer of the host, avoiding memory overwrite.
}

int SHTHR_SOUNDBUFFER_Impl::SetBufferData(void* pvBufferData, int dwBufferBytes)
{
	std::lock_guard<std::mutex> lock(mutex);


	if (dwBufferBytes == 0)
	{
		return 0;
	}

	// Free old internal buffer if we own it
	if (mBuffer && !mExternalBuffer)
	{
		g_userHeap.Free(mBuffer);
		mBuffer = nullptr;
	}

	int pcmSize = DSoundBufferGetPCMBufferSize(this->mG_flags, dwBufferBytes);


	if (pvBufferData == nullptr)
	{

		mBuffer = (unsigned char*)g_userHeap.Alloc(pcmSize);
		if (!mBuffer) {
			return -1; // Out of memory
		}
		memset(mBuffer, 0, dwBufferBytes);
		this->mG_flags ^= DSE_FLAG_BUFFER_EXTERNAL;
		mExternalBuffer = false;
	}
	else if (mSourceFormat == SOURCE_FORMAT_ADPCM_XBOX)
	{

		mBuffer = (unsigned char*)g_userHeap.Alloc(pcmSize);
		if (!mBuffer) {
			return -1;
		}

		// Decode ADPCM PCM (function returns decoded size)
		int32_t actualPcmSize = TXboxAdpcmDecoder_Decode_Memory(
			(uint8_t*)pvBufferData,
			dwBufferBytes,
			(uint8_t*)mBuffer,
			mChannelCount
		);


		if (actualPcmSize <= 0)
		{
			g_userHeap.Free(mBuffer);
			mBuffer = nullptr;
			return -1;
		}

		mBufferSize = actualPcmSize;
		mExternalBuffer = false;
		mG_flags &= ~DSE_FLAG_BUFFER_EXTERNAL;

	}
	else  	// if not use external buffer
	{
		this->mBuffer = (unsigned char*)pvBufferData;
		this->mExternalBuffer = true;
		this->mX_BufferCacheSize = dwBufferBytes;
		this->mG_flags |= DSE_FLAG_BUFFER_EXTERNAL;
	}

	// Update size and reset positions
	mBufferReadLoc = 0;
	mBufferWriteLoc = 0;
	mLoopStartOffset = 0;
	mLoopLength = 0;
	mWrittenSize = pcmSize;
	isStreaming = false;

	this->RebuildStream(mFormat, mChannelCount, mFrequency, mParent->mFormat, mParent->mChannelCount, mParent->mFrequency);

	return 0; // Success
}

void SHTHR_SOUNDBUFFER_Impl::RebuildStream(
	SDL_AudioFormat srcFormat, int srcChannels, int srcFreq,
	SDL_AudioFormat dstFormat, int dstChannels, int dstFreq)
{

	if (mAudioStream)
	{
		SDL_FreeAudioStream(mAudioStream);
		mAudioStream = nullptr;
	}

	mAudioStream = SDL_NewAudioStream(
		srcFormat, srcChannels, srcFreq,
		dstFormat, dstChannels, dstFreq);

	if (!mAudioStream)
		LOGFN_ERROR("Failed to create audio stream: {}", SDL_GetError());
}

int SHTHR_SOUNDBUFFER_Impl::SetLoopRegion(int dwLoopStartOffset, int dwLoopLength)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (mSourceFormat == SOURCE_FORMAT_ADPCM_XBOX)
	{
		dwLoopStartOffset = DSoundBufferGetPCMBufferSize(mG_flags, dwLoopStartOffset);
		dwLoopLength = DSoundBufferGetPCMBufferSize(mG_flags, dwLoopLength);
	}

	if (dwLoopStartOffset < 0 || dwLoopStartOffset >= mBufferSize) {
		return -1;
	}

	if (dwLoopLength < 0 || dwLoopStartOffset + dwLoopLength > mBufferSize) {
		return -1;
	}

	this->mLoopStartOffset = dwLoopStartOffset;
	this->mLoopLength = dwLoopLength;

	return 0;
}

void SHTHR_SOUNDBUFFER_Impl::StopSfxEarly()
{
	if (!mLooping && mWrittenSize <= 0)
	{
		mPlay = false;
		if (mAudioStream)
			SDL_AudioStreamClear(mAudioStream);
	}
}

PullResult SHTHR_SOUNDBUFFER_Impl::Pull(unsigned char* data, int32_t size)
{
	std::lock_guard<std::mutex> lock(mutex);

	PullResult result;

	bool sfxEmpty = mWrittenSize <= 0 && !mLooping;
	if (!mPlay || !mBuffer || sfxEmpty || !mAudioStream)
	{
		StopSfxEarly();
		return result;
	}


	int srcBytesPerSample = SDL_AUDIO_BITSIZE(mFormat) / 8 * mChannelCount;
	int dstBytesPerSample = SDL_AUDIO_BITSIZE(mParent->mFormat) / 8 * mParent->mChannelCount;
	float ratio = (float)mParent->mFrequency / (float)mFrequency
		* (float)dstBytesPerSample / (float)srcBytesPerSample;
	int read_size = (int)(size / ratio);
	read_size -= read_size % srcBytesPerSample; // align to sample boundary

	read_size = mLooping
		? read_size
		: std::min(read_size, mWrittenSize);

	if (read_size <= 0)
	{
		StopSfxEarly();
		return result;
	}

	int actualRead = read_size;

	if (this->isStreaming == false && mLooping && mLoopLength > 0)
	{
		int loopEnd = mLoopStartOffset + mLoopLength;
		mResampleBuffer.resize(read_size);
		int srcPos = mBufferReadLoc;
		int distToEnd = loopEnd - srcPos;

		if (distToEnd <= 0)
		{
			srcPos = mLoopStartOffset;
			distToEnd = mLoopLength;
		}

		if (read_size <= distToEnd)
		{
			memcpy(mResampleBuffer.data(), &mBuffer[srcPos], read_size);
			mBufferReadLoc = srcPos + read_size;
		}
		else
		{
			int copied = 0;
			while (copied < read_size)
			{
				distToEnd = loopEnd - srcPos;
				if (distToEnd <= 0)
				{
					srcPos = mLoopStartOffset;
					distToEnd = mLoopLength;
				}

				int chunk = std::min(read_size - copied, distToEnd);
				memcpy(mResampleBuffer.data() + copied, &mBuffer[srcPos], chunk);
				copied += chunk;
				srcPos += chunk;
			}
			mBufferReadLoc = srcPos;
		}

		mWrittenSize = mLoopLength;
	}
	else
	{
		mResampleBuffer.resize(read_size);
		if (mBufferReadLoc + read_size >= mBufferSize)
		{
			int remaining = mBufferSize - mBufferReadLoc;
			memcpy(mResampleBuffer.data(), &mBuffer[mBufferReadLoc], remaining);
			mBufferReadLoc = (read_size - remaining) % mBufferSize;
			memcpy(mResampleBuffer.data() + remaining, &mBuffer[0], mBufferReadLoc);
		}
		else
		{
			memcpy(mResampleBuffer.data(), &mBuffer[mBufferReadLoc], read_size);
			mBufferReadLoc += read_size;
		}
		mWrittenSize -= read_size;

		if (mWrittenSize < 0)
			mWrittenSize = 0;

		if (this->isStreaming == false)
		{
			if (mLooping && mLoopLength == 0)
			{
				if (mBufferReadLoc >= mBufferSize)
				{
					mBufferReadLoc %= mBufferSize;
					mWrittenSize = mBufferSize;
				}
			}
		}
	}

	if (!mLooping && mWrittenSize <= 0)
	{
		mPlay = false;
		SDL_AudioStreamClear(mAudioStream);
	}

	SDL_AudioStreamPut(mAudioStream, mResampleBuffer.data(), actualRead);

	int available = SDL_AudioStreamAvailable(mAudioStream);
	int writesize = std::min(size, available);

	if (writesize <= 0)
	{
		StopSfxEarly();
		return result;
	}

	mOutputBuffer.resize(writesize);
	SDL_AudioStreamGet(mAudioStream, mOutputBuffer.data(), writesize);

	memcpy(data, mOutputBuffer.data(), writesize);

	result.bytesRead = writesize;

	float vol3D = (mCaps & XTL_DSBCAPS_CTRL3D)
		? Calculate3DVolume()
		: 1.0f;
	result.mixVolume = mVolumeLinear * vol3D;

	return result;
}

int SHTHR_SOUNDBUFFER_Impl::GetStatus(uint32_t* pdwStatus)
{
	std::lock_guard<std::mutex> lock(mutex);
	int status = 0;

	if (mLooping)
	{
		status |= X_DSBSTATUS_LOOPING;
	}


	if (mPlay)
	{
		status |= X_DSBSTATUS_PLAYING;
	}

	if (mLooping)
	{
		status |= X_DSBSTATUS_LOOPING;
	}

	//*pdwStatus = status;
	return status;
}

int SHTHR_SOUNDBUFFER_Impl::GetCurrentPosition(int* pdwCurrentPlayCursor, int* pdwCurrentWriteCursor)
{
	std::lock_guard<std::mutex> lock(mutex);

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

int SHTHR_SOUNDBUFFER_Impl::SetCurrentPosition(DWORD dwNewPosition)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (dwNewPosition >= mBufferSize)
	{
		return -1;
	}

	mBufferReadLoc = dwNewPosition;
	return S_OK;
}

//the lock

int SHTHR_SOUNDBUFFER_Impl::Lock(int dwOffset, int dwBytes, void** ppvAudioPtr1, int* pdwAudioBytes1, void** ppvAudioPtr2, int* pdwAudioBytes2, int dwFlags)
{

	std::lock_guard<std::mutex> lock(mutex);
	isStreaming = true;


	if (dwFlags & DSBLOCK_ENTIREBUFFER)
	{
		*ppvAudioPtr1 = mBuffer;
		*pdwAudioBytes1 = mBufferSize;
		*ppvAudioPtr2 = NULL;
		*pdwAudioBytes2 = 0;

		mPendingWrite = { 0, mBufferSize };
	}
	else
	{
		int offset = dwFlags & DSBLOCK_FROMWRITECURSOR ? mBufferWriteLoc : dwOffset;
		int bytes = dwBytes;

		if (bytes > mBufferSize)
		{
			return DSERR_INVALIDPARAM;
		}

		if (offset + bytes > mBufferSize)
		{
			if (ppvAudioPtr1 && pdwAudioBytes1 && ppvAudioPtr2 && pdwAudioBytes2)
			{
				int remaining_size = mBufferSize - offset;
				*ppvAudioPtr1 = &mBuffer[offset];
				*pdwAudioBytes1 = remaining_size;
				*ppvAudioPtr2 = &mBuffer[0];
				*pdwAudioBytes2 = bytes - remaining_size;
			}
			else
			{

				return DSERR_INVALIDPARAM;
			}
		}
		else
		{
			if (ppvAudioPtr1 && pdwAudioBytes1)
			{
				*ppvAudioPtr1 = &mBuffer[offset];
				*pdwAudioBytes1 = bytes;
			}
			else
			{
				return DSERR_INVALIDPARAM;
			}
			if (ppvAudioPtr2 && pdwAudioBytes2)
			{
				*ppvAudioPtr2 = NULL;
				*pdwAudioBytes2 = 0;
			}
		}


		mPendingWrite = { offset, bytes };
	}


	return S_OK;

}

int SHTHR_SOUNDBUFFER_Impl::Unlock(void* pvAudioPtr1, int dwAudioBytes1, void* pvAudioPtr2, int dwAudioBytes2)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (!mPendingWrite.consumed)
	{
		mBufferWriteLoc = (mPendingWrite.offset + mPendingWrite.bytes) % mBufferSize;
		mWrittenSize = std::min(mWrittenSize + mPendingWrite.bytes, mBufferSize);
		mPendingWrite.consumed = true;
	}

	return S_OK;
}

int SHTHR_SOUNDBUFFER_Impl::Play(int dwReserved1, int dwPriority, uint32_t dwFlags)
{
	std::lock_guard<std::mutex> lock(mutex);

	this->mPlay = true;
	this->mPlayFlags = dwFlags;

	if (!mVolumeChangeFlag || mPrevVolume != this->mVolumeLinear)
	{
		if (PlayMusic)
		{
			this->mVolumeLinear *= Config::MusicVolume.Value;
			PlayMusic = false;
		}
		else
		{
			this->mVolumeLinear *= Config::VoicesVolume.Value;
		}

		mPrevVolume = this->mVolumeLinear;
		mVolumeChangeFlag = true;
	}

	if ((dwFlags & X_DSBPLAY_FROMSTART)) {
		mBufferReadLoc = 0;
		if (mAudioStream)
			SDL_AudioStreamClear(mAudioStream);
		this->mPlayFlags &= ~X_DSBPLAY_FROMSTART;
	}

	if (dwFlags & X_DSBPLAY_LOOPING)
	{
		this->mLooping = true;
	}
	else
	{
		this->mLooping = false;
		mBufferReadLoc = 0;
		if (!isStreaming)
		{
			mWrittenSize = mBufferSize;
		}
	}

	return S_OK;
}

int SHTHR_SOUNDBUFFER_Impl::Stop()
{
	std::lock_guard<std::mutex> lock(mutex);
	if (mAudioStream)
		SDL_AudioStreamClear(mAudioStream);

	mPlay = false;
	return S_OK;
}

int SHTHR_SOUNDBUFFER_Impl::StopEx(int64_t rtTimeStamp, uint32_t dwFlags)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (mAudioStream)
		SDL_AudioStreamClear(mAudioStream);

	mPlay = false;

	if (dwFlags > X_DSBSTOPEX_ALL)
	{
		return -1;
	}


	return S_OK;
}


int SHTHR_SOUNDBUFFER_Impl::SetVolume(int lVolume)
{
	std::lock_guard<std::mutex> lock(mutex);

	// adjust to DirectSound range
	if (lVolume < DSBVOLUME_MIN)
		lVolume = DSBVOLUME_MIN;

	if (lVolume > DSBVOLUME_MAX)
		lVolume = DSBVOLUME_MAX;

	mVolume = lVolume;

	// Convert decibels to linear (0-128 for SDL)
	// Formula: linear = 128 * 10^(dB/2000)
	float db = lVolume / 100.0f;  // Convert centibels to decibels
	float linear = powf(10.0f, db / 20.0f);  // dB to linear
	mVolumeLinear = static_cast<int>(linear * SDL_MIX_MAXVOLUME);  // 0-128

	const float volScale = this->GetCaps() & XTL_DSBCAPS_CTRL3D ? SFX_3DVOLUME_SCALE : SFX_VOLUME_SCALE;
	if (this->isStreaming == false)
	{
		// SFX tend to be way too loud so we scale them in addition to the user config
		mVolumeLinear = static_cast<int>(mVolumeLinear * volScale * Config::EffectsVolume.Value);
	}

	// Clamp
	if (mVolumeLinear < 0)
		mVolumeLinear = 0;

	if (mVolumeLinear > SDL_MIX_MAXVOLUME)
		mVolumeLinear = SDL_MIX_MAXVOLUME;

	return S_OK;
}

int SHTHR_SOUNDBUFFER_Impl::SetFrequency(int lFrequency)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (lFrequency < 100 || lFrequency > 200000)
		return -1;

	mFrequency = lFrequency;

	RebuildStream(mFormat, mChannelCount, mFrequency, mParent->mFormat, mParent->mChannelCount, mParent->mFrequency);


	mBufferReadLoc = 0;
	mWrittenSize = 0;

	return S_OK;
}

SHTHR_SOUNDDEVICE_Impl::~SHTHR_SOUNDDEVICE_Impl()
{
	mutex.lock();

	SDL_AtomicSet(&mPullThreadRunning, 0);
	if (mPullThread)
		SDL_WaitThread(mPullThread, nullptr);
	if (mDeviceID)
		SDL_CloseAudioDevice(mDeviceID);

	if (mDeviceID)
	{
		SDL_CloseAudioDevice(mDeviceID);
		mDeviceID = 0;
	}

	mAudioQueues.clear();
	mutex.unlock();

	SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

int SHTHR_SOUNDDEVICE_Impl::PullThreadStatic(void* userdata)
{
	return static_cast<SHTHR_SOUNDDEVICE_Impl*>(userdata)->PullThreadFunc();
}

int SHTHR_SOUNDDEVICE_Impl::PullThreadFunc()
{
	std::vector<uint8_t> mix_buf(FILL_CHUNK_BYTES, 0);
	std::vector<uint8_t> src_buf(FILL_CHUNK_BYTES, 0);

	while (SDL_AtomicGet(&mPullThreadRunning) && g_audioThreadShouldExit == false)
	{
		const Uint32 queued = SDL_GetQueuedAudioSize(mDeviceID);

		if (queued >= TARGET_QUEUE_BYTES)
		{
			SDL_Delay(SLEEP_FULL_MS);
			continue;
		}


		memset(mix_buf.data(), mAudioSpec.silence, FILL_CHUNK_BYTES);


		std::lock_guard<std::mutex> lk(mutex);
		for (auto& queue : mAudioQueues)
		{
			auto result = queue->Pull(src_buf.data(), FILL_CHUNK_BYTES);
			if (result.bytesRead > 0)
			{
			float finalVol = result.mixVolume * Config::MasterVolume.Value;
				finalVol = std::min(finalVol, (float)SDL_MIX_MAXVOLUME);


				if (result.bytesRead < FILL_CHUNK_BYTES)
					memset(src_buf.data() + result.bytesRead, mAudioSpec.silence,
						FILL_CHUNK_BYTES - result.bytesRead);


				SDL_MixAudioFormat(mix_buf.data(), src_buf.data(), mFormat, FILL_CHUNK_BYTES, (int)finalVol);
			}
		}


		SDL_QueueAudio(mDeviceID, mix_buf.data(), FILL_CHUNK_BYTES);

		// If queue was nearly empty, don't sleep — immediately refill
		if (queued < FILL_CHUNK_BYTES)
			continue;

		SDL_Delay(SLEEP_EMPTY_MS);
	}

	if (g_audioThreadShouldExit == true)
	{
		this->Release();
	}

	return 0;
}

int SHTHR_SOUNDDEVICE_Impl::SetAllParameters(Listener3D* params)
{
	paramsListener = *params;
	return S_OK;
}


int SHTHR_SOUNDDEVICE_Impl::SetDistanceFactor(float flDistanceFactor, DWORD dwApply)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (flDistanceFactor < 0.0f)
		flDistanceFactor = 0.0f;

	if (flDistanceFactor > 10.0f)
		flDistanceFactor = 10.0f;

	mDistanceFactor = flDistanceFactor;
	return S_OK;
}

int SHTHR_SOUNDDEVICE_Impl::SetRolloffFactor(float flRolloffFactor, DWORD dwApply)
{
	std::lock_guard<std::mutex> lock(mutex);

	if (flRolloffFactor < 0.0f)
		flRolloffFactor = 0.0f;

	if (flRolloffFactor > 10.0f)
		flRolloffFactor = 10.0f;

	mRolloffFactor = flRolloffFactor;

	return S_OK;
}

int SHTHR_SOUNDDEVICE_Impl::SetPosition(float x, float y, float z, DWORD dwApply)
{
	std::lock_guard<std::mutex> lock(mutex);

	mListenerPosition[0] = x;
	mListenerPosition[1] = y;
	mListenerPosition[2] = z;

	return S_OK;
}


void SHTHR_SOUNDDEVICE_Impl::AudioCallback(Uint8* stream, int32_t len)
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
		auto result = queue->Pull(&read_buffer[0], len);
		if (result.bytesRead > 0)
		{
			if (result.bytesRead < len)
			{
				memset(&read_buffer[result.bytesRead], mAudioSpec.silence, len - result.bytesRead);
			}
			float finalVol = result.mixVolume * Config::MasterVolume.Value;
			SDL_MixAudioFormat(stream, &read_buffer[0], mFormat, len, (int)finalVol);
		}
	}
	mutex.unlock();
}


static void AudioCallbackStatic(void* userdata, Uint8* stream, int32_t len)
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
	desired.userdata = nullptr; // this;
	desired.callback = nullptr; // AudioCallbackStatic;

	mDeviceID = SDL_OpenAudioDevice(nullptr, 0, &desired, &mAudioSpec, SDL_AUDIO_ALLOW_SAMPLES_CHANGE);

	if (mDeviceID)
	{
		mChannelCount = mAudioSpec.channels;
		mFrequency = mAudioSpec.freq;
		mFormat = mAudioSpec.format;

		SDL_AtomicSet(&mPullThreadRunning, 1);
		mPullThread = SDL_CreateThread(PullThreadStatic, "AudioPull", this);

		SDL_PauseAudioDevice(mDeviceID, SDL_FALSE);
		LOGFN_UTILITY("Init SDL2 Audio ok\n");
	}
}


//thread audio can be deleted after
int adxm_vsyncThread = -1;
static int adxm_fsThread = -1;
static int adxm_mwidlThread = -1;



bool SHTHR_SOUNDDEVICE_Impl::ReleaseSoundBuffer(SHTHR_SOUNDBUFFER* pBuffer)
{
	mutex.lock();
	mAudioQueues.erase((SHTHR_SOUNDBUFFER_Impl*)pBuffer);
	mutex.unlock();
	return true;
}

int32_t SHTHRSoundCreate(SHTHR_SOUNDDEVICE** ppDevice)
{
	if (!ppDevice)
		return -1;


	SHTHR_SOUNDDEVICE* device = new SHTHR_SOUNDDEVICE_Impl();

	if (!device)
		return -1;

	if (ppDS)
	{
		delete(ppDS);
	}

	auto ptr = g_userHeap.Alloc(sizeof(int));
	if (!ptr)
		return -1;

	auto key = g_memory.MapVirtual(ptr);

	ppDS = device;
	LOGN_UTILITY("created sound device\n");
	return key;
}

// hack to detect if we're about to play a music
// TObjBGM::Play
X86_FUNC_IMPL(__imp__sub_25CBB0);
X86_FUNC(sub_25CBB0)
{
	auto specialMode = X86_LOAD_U8(ctx.ecx.u32 + 0x2A);
	bool valid = specialMode == 2;
	auto bgmTaskAddr = GetHostParameter(1, CallConvention::Thiscall, ParameterType::Pointer);


	if (valid == false)
	{
		char* s = (char*)(base + ctx.ecx.u32 + 0x50);
		valid = strlen(s) > 0;
	}
	__imp__sub_25CBB0(ctx, base);

	if (valid)
		PlayMusic = true;
}
