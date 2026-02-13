#include <audio/audio.h>
#include <cpu/guest_thread.h>
#include <kernel/heap.h>
#include <logs.h>
#include <kernel/function.h>
#include <dsound.h>

#include <iostream>
#include <fstream>

static SDL_AudioDeviceID g_audioDevice{};


//thread audio can be deleted after
int adxm_vsyncThread = -1;
static int adxm_fsThread = -1;
static int adxm_mwidlThread = -1;
static volatile bool g_audioThreadShouldExit = false;


std::string GetThreadName(int threadID)
{
	if (threadID == adxm_vsyncThread)
		return "VSYNC THREAD";

	if (threadID == adxm_fsThread)
		return "FS THREAD";

	if (threadID == adxm_mwidlThread)
		return "MWIDL THREAD";

	return "no thread name found";
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


static void CreateAudioDevice()
{

	if (g_audioDevice != NULL)
		SDL_CloseAudioDevice(g_audioDevice);


	int allowedChanges = 0;

	SDL_AudioSpec desired{}, obtained{};
	desired.freq = XAUDIO_SAMPLES_HZ;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 2048;
	g_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, allowedChanges);


	if (!g_audioDevice)
		printf("Failed to open audio device: {}", SDL_GetError());

	SDL_PauseAudioDevice(g_audioDevice, SDL_FALSE);
}

void XAudioInitializeSystem()
{
#ifdef _WIN32
	// Force wasapi on Windows.
	SDL_setenv("SDL_AUDIODRIVER", "wasapi", true);
#endif

	SDL_SetHint(SDL_HINT_AUDIO_CATEGORY, "playback");
	SDL_SetHint(SDL_HINT_AUDIO_DEVICE_APP_NAME, "Shadow Recompiled");

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		//LOGFN_ERROR("Failed to init audio subsystem: {}", SDL_GetError());
		return;
	}

	CreateAudioDevice();
	g_audioThreadShouldExit = false;
}

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


void initSDL2Driver()
{

}