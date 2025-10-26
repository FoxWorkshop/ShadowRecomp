#include <audio/audio.h>
#include <cpu/guest_thread.h>
#include <kernel/heap.h>
#include <logs.h>
#include <kernel/function.h>
#include <dsound.h>

#include <iostream>
#include <fstream>

static SDL_AudioDeviceID g_audioDevice{};

std::vector<uint8_t> reference_data; //todo delete
//thread audio can be deleted after
int adxm_vsyncThread = -1;
static int adxm_fsThread = -1;
static int adxm_mwidlThread = -1;
int countBeforeBreak = 0;
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

struct adxT
{
	BYTE gap0[12];
	DWORD dwordC;
	DWORD dword10;
	DWORD dword14;
	char* pchar18;
	DWORD dword1C;
	WORD* leftPCM;
	WORD* rightPCM;
	__int16 int1628;
	BYTE gap2A[2];
	__int16 int162C;
	BYTE gap2E[2];
	WORD word30;
	WORD word32;
	WORD word34;
	WORD word36;
	WORD word38;
};



int audioSize = 0;
extern bool adxTest;
static int currentThread = 0;

int __cdecl ADXB_GetDecNumSmpl(int a1)
{
	auto base = g_memory.base;
	auto res = X86_LOAD_U32(a1 + 144);
	return res;
}


//adxsjd_decode_exec_r
//__int16 __usercall adxsjd_decode_exec@<ax>(_DWORD *a1@<esi>)
X86_FUNC_IMPL(__imp__sub_32E7C0);
X86_FUNC(sub_32E7C0)
{

	__imp__sub_32E7C0(ctx, base);

	ctx.esp.u32 -= 4;

	int* a1 = (int*)(base + ctx.esi.u32);


	//FILE* f = fopen("D:\\GitHub\\Downloaded Projects\\ShadowRecomp\\out\\build\\x64-Clang-Debug\\ShadowRecomp\\outSHTH.raw", "ab"); // append
	void* ptr;
	//if (f)
	{

		audioSize = ADXB_GetDecNumSmpl(a1[1]);
		ptr = (g_memory.Translate(a1[7]));
		SDL_QueueAudio(g_audioDevice, ptr, audioSize * 2);
		//auto len = fwrite((const void*)(ptr), 1, audioSize * 2, f);
		//fclose(f);
	}

	ctx.esp.u32 += 4;
}

void CaptureRawAudio(uint32_t data)
{
	if (data < 0x1000)
		return;

	FILE* f = fopen("D:\\GitHub\\Downloaded Projects\\ShadowRecomp\\out\\build\\x64-Clang-Debug\\ShadowRecomp\\outSHTH.raw", "ab"); // append
	void* ptr;
	if (f)
	{
		//fwrite((void*)data, 1, 500 * 2, f);
		fclose(f);
	}
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
	XAudioInitializeSystem();

	for (const auto& entry : fs::directory_iterator(fs::current_path()))
	{
		if (entry.path().extension() == ".raw")
		{
			auto s = entry.path().string();
			if (s.find("outSHTH") != std::string::npos)
			{
				//outSHTH.raw
				std::error_code ec; // optional: avoid exceptions
				if (fs::remove(entry.path(), ec))
				{
					std::cout << "Deleted: " << entry.path() << '\n';
					break;
				}
			}
		}
	}

	for (const auto& entry : fs::directory_iterator(fs::current_path())) {
		if (entry.path().extension() == ".txt")
		{
			if (entry.path().string().find("a2.txt") != std::string::npos || (entry.path().string().find("testA15Fix.txt") != std::string::npos))
			{
				continue;
			}

			std::error_code ec; // optional: avoid exceptions
			if (fs::remove(entry.path(), ec))
				std::cout << "Deleted: " << entry.path() << '\n';
		}
	}

	std::ifstream in("outEmulator.raw", std::ios::binary);
	reference_data.assign(std::istreambuf_iterator<char>(in), {});
}
