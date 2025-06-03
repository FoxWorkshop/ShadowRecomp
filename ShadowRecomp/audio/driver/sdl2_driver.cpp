#include <audio/audio.h>
#include <cpu/guest_thread.h>
#include <kernel/heap.h>
#include <logs.h>
#include <kernel/function.h>

static X86Func* g_clientCallback{};
static uint32_t g_clientCallbackParam{}; // pointer in guest memory
static SDL_AudioDeviceID g_audioDevice{};
static bool g_downMixToStereo;

std::vector<uint8_t> reference_data; //todo delete
static char* audio;
static char* audio2;
static std::vector<uint8_t> a2Val;

int* sizeAtr = nullptr;

static void CreateAudioDevice()
{

	if (g_audioDevice != NULL)
		SDL_CloseAudioDevice(g_audioDevice);


	int allowedChanges = 0;

	SDL_AudioSpec desired{}, obtained{};
	desired.freq = XAUDIO_SAMPLES_HZ;
	desired.format = AUDIO_S16SYS;
	desired.channels = 2;
	desired.samples = 1024;
	g_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, allowedChanges);


	if (!g_audioDevice)
		printf("Failed to open audio device: {}", SDL_GetError());

	SDL_PauseAudioDevice(g_audioDevice, 0);
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



#include <iostream>
#include <fstream>

bool ready = false;
int audioSize = 0;
extern bool adxTest;

DWORD* broken = nullptr;
static int countUntilFail = 0;
int __cdecl ADXB_GetDecNumSmpl(int a1)
{
	auto base = g_memory.base;
	countUntilFail = countUntilFail;
	auto res = X86_LOAD_U32(a1 + 144);
	broken = &*(DWORD*)((a1 + 144));
	return res;
}


//int ADXSJD_Init()
/**X86_FUNC_IMPL(__imp__sub_32DA80);
X86_FUNC(sub_32DA80)
{
	audio = (char*)(g_memory.Translate(0x595600));
	audio2 = (char*)(g_memory.Translate(0x5958BC));
	__imp__sub_32DA80(ctx, base);
	audio2 = audio2;

}*/

static int countBeforeBreak = 0;
X86_FUNC_IMPL(__imp__sub_336330);
int __cdecl ADXB_EntryData_r(int* a1, int a2, int a3)
{
	auto ctx = *GetX86Context();
	auto base = g_memory.base;
	auto esp = X86_LOAD_U16(ctx.esp.u32 + 4 + (2 * 4));
	esp = esp;

	__imp__sub_336330(ctx, base);
	ctx.esp.u32 -= 4;
	return ctx.eax.u32;
}

X86_FUNC(sub_336330)
{
	auto esp = X86_LOAD_U32(ctx.esp.u32 + 4 + (2 * 4));
	esp = esp;
	__imp__sub_336330(ctx, base);
	ctx.eax.u32 = ctx.eax.u32;
}

//adxsjd_decode_exec_r
//__int16 __usercall adxsjd_decode_exec@<ax>(_DWORD *a1@<esi>)
X86_FUNC_IMPL(__imp__sub_32E7C0);
X86_FUNC(sub_32E7C0)
{

	__imp__sub_32E7C0(ctx, base);

	ctx.esp.u32 -= 4;

	int* a1 = (int*)(base + ctx.esi.u32);

	std::ifstream in("outSHTH.raw", std::ios::binary);
	std::vector<uint8_t> recompAudio;
	recompAudio.assign(std::istreambuf_iterator<char>(in), {});
	audio2 = audio2;

	for (size_t i = 0; i < recompAudio.size(); i++)
	{
		if (i < reference_data.size())
		{
			uint8_t actual = recompAudio[i];
			uint8_t expected = reference_data[i];

			if (i >= 200 && i <= 215)
			{
				actual = actual;
			}
			if (i >= 16000)
			{
				countUntilFail = i;
			}
			if (actual != expected)
			{
				countBeforeBreak = countBeforeBreak;
				printf("[DIVERGENCE] At offset 0x%zX: expected 0x%02X, got 0x%02X\n", i, expected, actual);
				//__debugbreak(); // or insert log and halt
			}
		}

	}
	recompAudio.clear();


	countBeforeBreak++;
	FILE* f = fopen("D:\\GitHub\\Downloaded Projects\\ShadowRecomp\\out\\build\\x64-Clang-Debug\\ShadowRecomp\\outSHTH.raw", "ab"); // append
	void* ptr;
	if (f)
	{
		audioSize = ADXB_GetDecNumSmpl(a1[1]);
		ptr = (g_memory.Translate(a1[7]));
		auto len = fwrite((const void*)(ptr), 1, audioSize * 2, f);
		fclose(f);
	}

	ctx.esp.u32 += 4;

	/**if (countUntilFail != 0)
	{
		std::ofstream outFile("DecodeIssueRecomp.txt", std::ios::app);

		outFile << "adxsjd_decode_exec" << '\n';
		outFile << "i " << countUntilFail << '\n';
		auto test = (uint8_t*)ptr;
		outFile << std::hex << "a1[7] 0x" << (uint32_t)a1[7] << '\n';
		outFile << "size " << audioSize << '\n';
		outFile << "a1[7] data (raw bytes):  " << '\n';
		for (uint8_t i = 0; i < 4; i++)
		{
			outFile << std::hex << "0x" << (int)(test[i]) << ", ";
		}
		outFile << '\n';
		outFile << '\n';
		outFile << "a1[7] data (samples):  " << '\n';
		int16_t* samples = (int16_t*)ptr;
		outFile << samples[0] << ", " << samples[1] << '\n';
		outFile << '\n';
		outFile.close();
	}*/


}


static void CreateAudioThread()
{

	// g_audioThread = std::make_unique<std::thread>(AudioThread);
}


#include <filesystem>
namespace fs = std::filesystem;

void DebugLoadA2File()
{
	std::ifstream file("a2.txt");

	std::string line;

	if (!file.is_open()) {
		std::cerr << "Failed to open the file.\n";
		return;
	}

	while (std::getline(file, line)) 
	{
		int number = std::stoi(line);
		a2Val.push_back(number);
	}

	file.close();
}

void XAudioRegisterClient()
{
	XAudioInitializeSystem();
	CreateAudioThread();


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
			if (entry.path().string().find("a2.txt") != std::string::npos)
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

	DebugLoadA2File();
}

void XAudioSubmitFrame(void* samples)
{

}
