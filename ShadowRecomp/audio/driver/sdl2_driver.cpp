#include <audio/audio.h>
#include <cpu/guest_thread.h>
#include <kernel/heap.h>
#include <logs.h>
#include <kernel/function.h>
#include <dsound.h>

static SDL_AudioDeviceID g_audioDevice{};
static bool g_downMixToStereo;

std::vector<uint8_t> reference_data; //todo delete
static char* audio;
static char* audio2;
static std::vector<uint8_t> a2Val;
static std::vector<uint32_t> a15Val;

int* sizeAtr = nullptr;

//thread audio can be deleted after
int adxm_vsyncThread = -1;
static int adxm_fsThread = -1;
static int adxm_mwidlThread = -1;
int countBeforeBreak = 0;


static DWORD last_play_pos = 0;
static const int buffer_size = 0x2000; 
static BYTE* pcm_buffer = nullptr;
X86_FUNC_IMPL(__imp__sub_3E4274);


void ControlAccuracy(std::string funcName)
{
	if (countBeforeBreak < 1)
		return;

	std::ifstream in("outSHTH.raw", std::ios::binary);
	std::vector<uint8_t> recompAudio;
	recompAudio.assign(std::istreambuf_iterator<char>(in), {});
	audio2 = audio2;

	if (countBeforeBreak == 20)
	{
		printf("before break\n");
	}

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

			if (actual != expected)
			{
				funcName = funcName;
				countBeforeBreak = countBeforeBreak;
				//printf("[DIVERGENCE] At offset 0x%zX: expected 0x%02X, got 0x%02X\n", i, expected, actual);
			}
		}

	}
}


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
static int currentThread = 0;

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

X86_FUNC_IMPL(__imp__sub_2940D0);
X86_FUNC(sub_2940D0)
{
	auto mode = X86_LOAD_U32(ctx.ecx.u32);
	mode = mode;
	if (mode == 6)
	{
		std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
		outFile2 << '\n';
		outFile2 << "RESTARTING TITLE SCREEN" << '\n';
		outFile2 << '\n';
		outFile2.close();

		printf("Restarting title screen\n");
	}
	__imp__sub_2940D0(ctx, base);
}


X86_FUNC_IMPL(__imp__sub_336430);
int __cdecl ADXB_EvokeDecode_r(DWORD* a1)
{
	auto ctx = *GetX86Context();
	auto base = g_memory.base;
	std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
	outFile2 << "===ADXB_EvokeDecode==" << '\n';
	outFile2 << "COUNT " << countBeforeBreak << '\n';
	auto id = GetCurrentThreadId();
	outFile2 << "Thread ID: " << id << std::endl;
	outFile2 << "Thread Name: " << GetThreadName(id) << std::endl;
	outFile2 << std::hex << "a1[19]: " << a1[19] << " a1[20]: " << a1[20] << " a1[22]: " << a1[22] << " a1[24]: " << a1[24] << '\n';
	outFile2 << std::hex << "a1[26]: " << a1[26] << " a1[27]: " << a1[27] << " a1[28]: " << a1[28] << '\n';

	outFile2 << "===ADXB_EvokeDecode END ===" << '\n';
	outFile2.close();


	__imp__sub_336430(ctx, base);

	ctx.esp.u32 -= 4;

	return ctx.eax.u32;
}

//adxsjd_get_wr
X86_FUNC_IMPL(__imp__sub_32DE50);
X86_FUNC(sub_32DE50)
{
	auto ptr = X86_LOAD_U32(ctx.esp.u32 + 4 + (0 * 4));
	auto a1 = (int*)(base + ptr);
	ptr = X86_LOAD_U32(ctx.esp.u32 + 4 + (2 * 4));
	auto a3 = (int*)(base + ptr);
	auto res = &a1[8];


	std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
	outFile2 << "===adxsjd_get_wr===" << '\n';
	outFile2 << "count: " << countBeforeBreak << '\n';
	outFile2 << "a1[8] " << std::hex << a1[8] << '\n';
	outFile2 << "a1[8] / 2 " << std::hex << a1[8] / 2 << '\n';
	outFile2 << "a1[14] " << std::hex << a1[14] << '\n';
	outFile2 << '\n';
	outFile2.close();

	if (countBeforeBreak == 10)
	{
		printf("t\n");
	}

	__imp__sub_32DE50(ctx, base);

	std::ofstream  outFile("ADXB_EvokeDecode.txt", std::ios::app);
	outFile << "===POST adxsjd_get_wr===" << '\n';
	outFile << "a1[8] " << std::hex << a1[8] << '\n';
	if (a1[8] / 2 >= a1[14])
	{
		outFile << "a1[8] / 2 was superior! Size adjusted. " << std::hex << a1[8] / 2 << '\n';
	}


	outFile << "a3 (a1[27]) " << std::hex << *a3 << '\n';
	if (countBeforeBreak == 16)
	{
		a1[8] = a1[8];

	}
	outFile << "===adxsjd_get_wr END===" << '\n';
	outFile << '\n';
	outFile.close();
}

//ADXDecodeStereo
X86_FUNC_IMPL(__imp__sub_3391B0);
X86_FUNC(sub_3391B0)
{
	auto a2 = X86_LOAD_U32(ctx.esp.u32 + 4 + (1 * 4));
	auto a3 = (WORD*)(base + ctx.esp.u32 + 4 + (2 * 4));
	auto a5 = (WORD*)(base + ctx.esp.u32 + 4 + (4 * 4));

	std::ofstream  outFile("ADXB_EvokeDecode.txt", std::ios::app);
	outFile << '\n';
	outFile << "ADXDecodeStereo" << '\n';
	//outFile << "leftPcm " << std::hex << *a3 << '\n';
	//outFile << "rightPcm " << std::hex << *a5 << '\n';
	outFile << "a2 " << std::hex << a2 << '\n';
	outFile << '\n';
	outFile.close();


	if (countBeforeBreak == 1)
	{
		currentThread = GetCurrentThreadId();
	}


	if (countBeforeBreak == 20) 
	{
	
	}

	//printf("count before break: %d\n", countBeforeBreak++);

	__imp__sub_3391B0(ctx, base);
}


GUEST_FUNCTION_HOOK(sub_336430, ADXB_EvokeDecode_r);

X86_FUNC_IMPL(__imp__sub_336330);
/**int __cdecl ADXB_EntryData_r(int* a1, int a2, int a3)
{
	auto ctx = *GetX86Context();
	auto base = g_memory.base;
	auto esp = X86_LOAD_U16(ctx.esp.u32 + 4 + (2 * 4));
	esp = esp;

	__imp__sub_336330(ctx, base);
	ctx.esp.u32 -= 4;
	return ctx.eax.u32;
}*/

X86_FUNC(sub_336330)
{
	auto ptr = X86_LOAD_U32(ctx.esp.u32 + 4);
	auto a1 = (int32_t*)(base + ptr);
	auto a3 = X86_LOAD_U32(ctx.esp.u32 + 4 + (2 * 4));
	a1 = a1;

	std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);

	auto v3 = *((WORD*)a1 + 76) == 0;

	int v4 = 0;
	if (v3)
	{
		v4 = *((char*)a1 + 15);
	}
	else
	{
		auto v5 = *((char*)a1 + 13);
		v4 = *((char*)a1 + 14) * (v5 / 8);
	}

	outFile2 << "=== ADXB_EntryData ===" << '\n';
	outFile2 << "COUNT " << countBeforeBreak << '\n';
	outFile2 << std::hex << "a3: " << a3 << " v4: " << v4 << '\n';

	outFile2 << "=== ADXB_EntryData END ===" << '\n';
	outFile2 << '\n';
	outFile2.close();



	__imp__sub_336330(ctx, base);
	ctx.eax.u32 = ctx.eax.u32;
}
static int countThisBs = 0;
static int indexFix = 0;
X86_FUNC_IMPL(__imp__sub_32CE80);
X86_FUNC(sub_32CE80)
{

	int oldA41 = 0;
	auto ptr = X86_LOAD_U32(ctx.esp.u32 + 4);
	auto a1 = (int32_t*)(base + ptr);
	ptr = X86_LOAD_U32(ctx.esp.u32 + 16);
	auto a4 = (int32_t*)(base + ptr);
	auto a3 = X86_LOAD_U32(ctx.esp.u32 + 12);
	a3 = a3;

	if (currentThread == GetCurrentThreadId())
	{
		countThisBs++;


		a4[1] = a4[1];
		a1[4] = a1[4];

		if (a3 == 0x40 && countBeforeBreak == 21)
		{
			countThisBs = countThisBs;
		}


		if (countBeforeBreak >= 7 && countBeforeBreak <= 21)
		{

			std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
			outFile2 << "===sub_32CE80==" << '\n';
			outFile2 << "COUNT " << countBeforeBreak << '\n';
			auto id = GetCurrentThreadId();
			outFile2 << "Thread ID: " << id << std::endl;
			outFile2 << "Thread Name: " << GetThreadName(id) << std::endl;

			auto bufferData = a1[5] + a1[7];
			oldA41 = a1[4];
			outFile2 << std::hex << "a1[9]: " << a1[9] << " a1[8]: " << a1[8] << " a1[5] + a1[7]: " << X86_LOAD_U32(bufferData) << " a1[6]: " << a1[6] << " a1[5]: " << a1[5] << " a1[4] : " << a1[4] << '\n';
			outFile2 << "a3: " << (uint32_t)a3 << '\n';


			auto v7 = a1[8] + a1[9] - a1[5];

			countBeforeBreak = countBeforeBreak;
			outFile2 << "v4 " << v7 << '\n';
			if (a1[4] < v7)
			{
				outFile2 << "warning, a1[4] INFERIOR TO V7, new size: " << a1[4] << '\n';
			}
			auto sizeA = a1[9] - a1[6] + a1[8];
			outFile2 << "sizeA res: " << sizeA << '\n';
			if (a1[3] < sizeA)
			{
				outFile2 << "warning, a1[3] INFERIOR TO sizeA: " << a1[3] << '\n';
				sizeA = a1[3];
			}
			if (sizeA < a3)
			{
				outFile2 << "warning, sizeA INFERIOR TO a3: " << a3 << '\n';
			}
			//outFile2 << "*(uint32_t*)0x10059AC90: " << *(uint32_t*)0x10059AC90 << '\n';
			//outFile2 << "*(uint32_t*)0x10059ACD0: " << *(uint32_t*)0x10059ACD0 << '\n';
			//outFile2 << "*(uint32_t*)0x10059AD10: " << *(uint32_t*)0x10059AD10 << '\n';
			outFile2 << "===sub_32CE80 END==" << '\n';
			outFile2 << '\n';
			outFile2.close();

		}
	}


	__imp__sub_32CE80(ctx, base);

	if (currentThread == GetCurrentThreadId())
	{
		if (countBeforeBreak >= 7 && countBeforeBreak <= 21)
		{
			std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
			outFile2 << "===post sub_32CE80==" << '\n';

			outFile2 << "old a1[4] " << std::hex << oldA41 << '\n';
			outFile2 << "a1[4] " << std::hex << a1[4] << '\n';
			outFile2 << "a4[1] " << std::hex << a4[1] << '\n';
			outFile2 << '\n';
			outFile2.close();
		}
	}
}

int __cdecl sub_337310_r(DWORD* a1)
{
	a1 = a1;
	auto a5Res = -a1[5];
	auto a5Pos = a1[5];
	auto a4 = a1[4];
	return 0;
}

//GUEST_FUNCTION_HOOK(sub_337310, sub_337310_r);

static int countChunk = 0;
static 	uint32_t* whatisThisSHIT = nullptr;
//_DWORD *__usercall SJRBF_PutChunk@<eax>(_DWORD *result@<eax>, int a2@<ecx>, char **a3@<ebx>)
X86_FUNC_IMPL(__imp__sub_32C900);
X86_FUNC(sub_32C900)
{



	if (countBeforeBreak >= 1 && countBeforeBreak <= 21 && GetCurrentThreadId() == adxm_vsyncThread)
	{

		if (countChunk >= 52)
		{
			whatisThisSHIT = whatisThisSHIT;
		}


		char** a3 = (char**)(base + ctx.ebx.u32);
		DWORD* a1 = (DWORD*)(base + ctx.eax.u32);
		std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
		outFile2 << "===SJRBF_PutChunk==" << '\n';
		auto id = GetCurrentThreadId();
		outFile2 << "Thread ID: " << id << std::endl;
		outFile2 << "Thread Name: " << GetThreadName(id) << std::endl;
		outFile2 << "COUNT BEFORE BREAK " << countBeforeBreak << '\n';
		outFile2 << "CALL COUNT " << countChunk++ << '\n';

		auto first = (uint32_t)(base + (uint32_t)a3 + 4);
		auto second = (uint32_t)(base + (uint32_t)a3);
		whatisThisSHIT = (uint32_t*)(base + (uint32_t)a1 + 4 * 4);
		a1[4] = a1[4];
		outFile2 << "a1[4] " << std::hex << (uint32_t)whatisThisSHIT << " - " << a1[4] << '\n';


		if (a3 && a3[1])
		{
			outFile2 << std::hex << "a3[1]: " << X86_LOAD_U32(first) << " *a3: " << X86_LOAD_U32(second) << '\n';
		}
		else
		{
			outFile2 << "a3 was null" << '\n';
		}

		outFile2 << "===SJRBF_PutChunk END==" << '\n';
		outFile2 << '\n';
		outFile2.close();


	}


	__imp__sub_32C900(ctx, base);


	if (countBeforeBreak >= 1 && countBeforeBreak <= 21 && GetCurrentThreadId() == adxm_vsyncThread)
	{
		DWORD* a1 = (DWORD*)(base + ctx.eax.u32);
		char** a3 = (char**)(base + ctx.ebx.u32);

		std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
		outFile2 << "===POST SJRBF_PutChunk==" << '\n';
		auto first = (uint32_t)(base + (uint32_t)a3 + 4);
		auto second = (uint32_t)(base + (uint32_t)a3);

		if (a3 && a3[1])
		{
			outFile2 << std::hex << "a3[1]: " << X86_LOAD_U32(first) << " *a3: " << X86_LOAD_U32(second) << '\n';
		}
		else
		{
			outFile2 << "a3 was null" << '\n';
		}

		outFile2 << "a1[4] " << a1[4] << '\n';
		outFile2 << "===SJRBF_PutChunk END==" << '\n';
		outFile2 << '\n';
		outFile2.close();
	}
}

static long long v2Res = 0;

X86_FUNC_IMPL(__imp__sub_335110);
X86_FUNC(sub_335110)
{

	auto a1 = X86_LOAD_U32(ctx.esp.u32 + 4);
	unsigned int v1; // esi
	unsigned int v2; // edi
	int v3; // eax
	char v5[4]; // [esp+8h] [ebp-4h] BYREF

	v1 = a1;
	v2 = 0;
	bool giveup = false;
	std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
	outFile2 << "===PRE sub_335110===" << '\n';

	v3 = *(DWORD*)((base + a1 + 8));

	if (X86_LOAD_U32(a1 + 52) == 1)
	{
		outFile2 << "a1 + 52 is equal 1, giving up." << '\n';
		giveup = true;
	}


	if (giveup == false)
	{
		outFile2 << std::hex << "v3: " << X86_LOAD_U32(a1 + 8) << '\n';
		outFile2 << std::hex << "a1: " << X86_LOAD_U32(a1) << '\n';
		outFile2 << std::hex << "v1: " << X86_LOAD_U32(v1) << '\n';
		outFile2 << std::hex << "v1 + 34 >> 3: " << (X86_LOAD_U32(v1 + 34) >> 3) << '\n';
		outFile2 << std::hex << "v1 + 44: " << (X86_LOAD_U32(v1 + 44)) << '\n';
		outFile2 << std::hex << "v2: " << v2 << '\n';

		outFile2 << std::hex << "final res: " << v2Res << '\n';
		
	}
	outFile2 << '\n';
	outFile2.close();


	__imp__sub_335110(ctx, base);

	if (giveup == false)
	{
		ctx.esp.u32 -= 4;

		int addr = ctx.esp.u32 + 4;

		a1 = X86_LOAD_U32(ctx.esp.u32 + 4);
		std::ofstream  outFile("ADXB_EvokeDecode.txt", std::ios::app);
		outFile << "===POST sub_335110===" << '\n';
		if (a1 > 0)
		{
			outFile << std::hex << "a1: " << X86_LOAD_U32(a1) << '\n';
		}
		else
		{
			addr = addr;
			outFile << std::hex << "a1 WAS 0 LOLMAO" << '\n';
		}
		outFile << std::hex << "v1: " << X86_LOAD_U32(v1) << '\n';
		outFile << std::hex << "v1 + 34 >> 3: " << (X86_LOAD_U32(v1 + 34) >> 3) << '\n';
		outFile << std::hex << "v1 + 44: " << (X86_LOAD_U32(v1 + 44)) << '\n';
		outFile << std::hex << "v2: " << ctx.eax.u32 << '\n';
		v2Res = ctx.eax.u32;
		outFile << std::hex << "final res: " << v2Res << '\n';
		outFile << '\n';
		outFile.close();

		ctx.esp.u32 += 4;
	}
}

X86_FUNC_IMPL(__imp__sub_3316E0);
X86_FUNC(sub_3316E0)
{
	auto ptr = (int*)(base + ctx.esi.u32);
	int a1 = (int)(base + ctx.esi.u32);
	std::ofstream  outFile2("ADXB_EvokeDecode.txt", std::ios::app);
	outFile2 << "===sub_3316E0===" << '\n';

	outFile2 << std::hex << "*(_DWORD *)(a1 + 76): " << X86_LOAD_U32((DWORD)a1 + 76) << '\n';
	outFile2 << std::hex << "v3: " << 8 * (32 / (X86_LOAD_U32((DWORD)(a1 + 8)))) << '\n';
	outFile2 << "a1 + 8: " << (X86_LOAD_U32((uint32_t)(a1 + 8))) << '\n';
	outFile2 << '\n';
	outFile2.close();

	__imp__sub_3316E0(ctx, base);

	a1 = (int)(ptr);
	auto v1 = X86_LOAD_U32((a1 + 72));
	auto v3 = 8 * (32 / (X86_LOAD_U32(a1 + 8)));
	//auto v4 = v3 * (v2 / v3);
	std::ofstream  outFile("ADXB_EvokeDecode.txt", std::ios::app);
	outFile << "===post sub_3316E0===" << '\n';
	outFile << std::hex << "*(_DWORD *)(a1 + 76): " << X86_LOAD_U32((DWORD)a1 + 76) << '\n';

	outFile << "a1 + 8: " << (X86_LOAD_U32((uint32_t)(a1 + 8))) << '\n';
	outFile << std::hex << "v1: " << (v1) << '\n';
	outFile << std::hex << "v2: " << v2Res << '\n';
	outFile << std::hex << "v3: " << 8 * (32 / (X86_LOAD_U32((DWORD)(a1 + 8)))) << '\n';
	auto v4 = v3 * (v2Res / v3);
	outFile << std::hex << "v4: " << v4 << '\n';
	auto v5 = v4 - v1;
	outFile << std::hex << "v5: " << v5 << '\n';


	//outFile2 << std::hex << "  v3 = 8 * (32 / *(_DWORD *)(a1 + 8)) " << 8 * (32 / *(DWORD*)(ptr + 8)) << '\n';

	outFile << '\n';
	outFile.close();
	//outFile2 << std::hex << "  v4 = v3 * (v2 / v3) " << v4 = v3 * (v2 / v3) << '\n';

}

int* RNAA1Ptr = nullptr;
//RNA_Init
X86_FUNC_IMPL(__imp__sub_330E60);
X86_FUNC(sub_330E60)
{

	__imp__sub_330E60(ctx, base);
	RNAA1Ptr = (int*)(base + 0x593860 + 0x40);
	RNAA1Ptr = RNAA1Ptr;

}

int* wtfisThis;
//adxsjd_decode_exec_r
//__int16 __usercall adxsjd_decode_exec@<ax>(_DWORD *a1@<esi>)
X86_FUNC_IMPL(__imp__sub_32E7C0);
X86_FUNC(sub_32E7C0)
{
	ControlAccuracy("PRE adxsjd_decode_exec");
	__imp__sub_32E7C0(ctx, base);

	ctx.esp.u32 -= 4;

	int* a1 = (int*)(base + ctx.esi.u32);

	std::ifstream in("outSHTH.raw", std::ios::binary);
	std::vector<uint8_t> recompAudio;
	recompAudio.assign(std::istreambuf_iterator<char>(in), {});
	audio2 = audio2;


	recompAudio.clear();

	//FILE* f = fopen("D:\\GitHub\\Downloaded Projects\\ShadowRecomp\\out\\build\\x64-Clang-Debug\\ShadowRecomp\\outSHTH.raw", "ab"); // append
	void* ptr;
	//if (f)
	{
		wtfisThis = &a1[7];
		if (countBeforeBreak == 19)
		{

			wtfisThis = wtfisThis;
		}
		audioSize = ADXB_GetDecNumSmpl(a1[1]);
		ptr = (g_memory.Translate(a1[7]));
		SDL_QueueAudio(g_audioDevice, ptr, audioSize * 2);
		//auto len = fwrite((const void*)(ptr), 1, audioSize * 2, f);
		//fclose(f);
	}

	ctx.esp.u32 += 4;
}


static std::unique_ptr<std::thread> g_audioThread;
static volatile bool g_audioThreadShouldExit = false;

static void AudioThread()
{

	
}

static void CreateAudioThread()
{
	SDL_PauseAudioDevice(g_audioDevice, 0);
	g_audioThreadShouldExit = false;
}


bool QuickBoot = true;
//skip FMV
X86_FUNC_IMPL(__imp__sub_24F80);
X86_FUNC(sub_24F80)
{
	if (QuickBoot)
	{
		ctx.eax.u32 = 0;
		ctx.esp.u32 += 4;
		return;
	}
	else
	{
		__imp__sub_24F80(ctx, base);
	}

}

X86_FUNC_IMPL(__imp__sub_29EA60);
X86_FUNC(sub_29EA60)
{

	if (QuickBoot)
	{
		auto ptr = X86_LOAD_U32(ctx.ecx.u32 + 4);
		auto mode = X86_LOAD_U32(ptr + 0x64);


		if (mode < 2)
		{
			X86_STORE_U8(ptr + 0x64, 0x2);
		}


	}

	__imp__sub_29EA60(ctx, base);
}

//get current timer
X86_FUNC_IMPL(__imp__sub_2BB140);
X86_FUNC(sub_2BB140)
{

	__imp__sub_2BB140(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_377360);
X86_FUNC(sub_377360)
{

	/**static auto lastTime = std::chrono::high_resolution_clock::now();
	auto now = std::chrono::high_resolution_clock::now();
	auto dt = std::chrono::duration<float>(now - lastTime).count();
	lastTime = now;

	float dword_532A48 = X86_LOAD_F32(0x532A48);
	float frameTime_HHC = X86_LOAD_F32(0x541FA8);
	float a1 = X86_LOAD_F32(ctx.esp.u32 + 4);
	std::ofstream outFile2("framerate.txt", std::ios::app);
	outFile2 << "===framerate==" << '\n';
	outFile2 << "a1 " << (float)a1 << '\n';
	outFile2 << "dword_532A48 " << (float)dword_532A48 << '\n';
	outFile2 << "frameTime_HHC " << (float)frameTime_HHC << '\n';
	outFile2 << "Real frame time: " << dt << '\n';
	outFile2 << '\n';
	outFile2.close();*/

	//std::this_thread::sleep_for(std::chrono::milliseconds(16));
	__imp__sub_377360(ctx, base);
}


#include <filesystem>
namespace fs = std::filesystem;



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
