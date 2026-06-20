#include <kernel/function.h>
#include <kernel/kernel.h>
#include <gpu/video.h>
#include <cpu/guest_thread.h>
#include <os/logger.h>


// While OG Xbox has thread support, it was originally a mono-core console, meaning threads weren't actually running in parallel
// Thus, true multi-core creates race issues on PC, specially at load time since it has its own thread running while the main-game loop runs too.
// So we patch functions that can edit same memory to avoid race issue using mutex and others tweaks.
// It would be nice to find a better way, but so far this is the best compromise I found other than forcing mono-core, which I'd prefer not to.


static std::unordered_map<uint32_t, uint32_t> g_threadDicts;
static std::mutex g_threadDictsMutex;
static std::atomic<int> loadingThreadID{0};

XRTL_CRITICAL_SECTION dirtyListLock;
XRTL_CRITICAL_SECTION csdNodeLock;
XRTL_CRITICAL_SECTION fmvPtrDataLock;
XRTL_CRITICAL_SECTION globalcurCamera;
XRTL_CRITICAL_SECTION globalCurWorld;

void InitMultiCoreMutexesPatches()
{
	RtlInitializeCriticalSection(&dirtyListLock);
	RtlInitializeCriticalSection(&csdNodeLock);
	RtlInitializeCriticalSection(&fmvPtrDataLock);
	RtlInitializeCriticalSection(&globalcurCamera);
	RtlInitializeCriticalSection(&globalCurWorld);
}

/* --- RwFrame Dirty link list race ---

 When the loading thread is running, two threads are updating the dirty link list (dword_57DEBC) which can create a race issue 
 and corrupt some random RwFrames which will eventually crash the game.
 we can fix that by adding mutex to prevent the race.
 */

// RwFrameUpdateObject
X86_FUNC_IMPL(__imp__sub_2E1300);
X86_FUNC(sub_2E1300)
{
	auto frame = X86_LOAD_U32(ctx.esp.u32 + 4);
	bool valid = false;
	if (frame)
	{
		auto frameRoot = X86_LOAD_U32(frame + 0xa0);
		auto flag = (uint32_t)X86_LOAD_U8(frameRoot + 0x3);
		if ((flag & 3) == 0) //dirty list only get updated if this condition pass
		{
			RtlEnterCriticalSection(&dirtyListLock);
			valid = true;
		}
	}

	__imp__sub_2E1300(ctx, base);

	if (valid)
		RtlLeaveCriticalSection(&dirtyListLock);
}

//RwFrameSyncDirty
X86_FUNC_IMPL(__imp__sub_2EC720);
X86_FUNC(sub_2EC720)
{
	RtlEnterCriticalSection(&dirtyListLock);
	__imp__sub_2EC720(ctx, base);
	RtlLeaveCriticalSection(&dirtyListLock);
}


/* --- Null Textures Fix, Set Texture Dictionary race ---

 Multiple threads can change the current texture dictionary from RW
 If thread B changes the dictionary right when thread A is about to call RwTexDictionaryFindNamedTexture, it will
 make thread A not find the texture it asked, which will eventually result to the game not 
 applying some textures. We can fix this by creating one global dictionary list per thread and give the intended one to the game 
 when it requests it.
 */


void SetNewDictionaryThread(const uint32_t texDictionaryPtr)
{
	g_threadDicts[GuestThread::GetCurrentThreadId()] = texDictionaryPtr;
}

uint32_t GetCurrentDictionaryThreadUnsafe()
{
	auto it = g_threadDicts.find(GuestThread::GetCurrentThreadId());
	if (it != g_threadDicts.end())
	{
		return it->second;
	}

	return 0;
}

uint32_t GetCurrentDictionaryThread()
{
	std::lock_guard<std::mutex> lock(g_threadDictsMutex);
	return GetCurrentDictionaryThreadUnsafe();
}


// RwTexDictionarySetCurrent
X86_FUNC_IMPL(__imp__sub_2D8440);
X86_FUNC(sub_2D8440)
{

	std::lock_guard<std::mutex> lock(g_threadDictsMutex);

	auto ptr = X86_LOAD_U32(ctx.esp.u32 + 4);

	//printf("[RwTexDictionarySetCurrent] %s ptr %x count %d\n", GetThreadName(), ptr, g_dictTexCount[ptr]);
	SetNewDictionaryThread(ptr);
	__imp__sub_2D8440(ctx, base);
}

// RwTexDictionaryGetCurrent
X86_FUNC_IMPL(__imp__sub_2D8460);
X86_FUNC(sub_2D8460)
{
	__imp__sub_2D8460(ctx, base);
	auto ogRes = ctx.eax.u32;
	auto res = GetCurrentDictionaryThread();
	if (ogRes > 0 && res > 0)
	{
		ctx.eax.u32 = res;
	}
}

// for inlined RwTexDictionaryGetCurrent
void GetCurrentDictionaryThreadMidAsmHook(X86Register& eax)
{
	auto res = GetCurrentDictionaryThread();
	if (res)
	{
		eax.u32 = res;
	}

}

// RwTexDictionaryDestroy
X86_FUNC_IMPL(__imp__sub_2D8D80);
X86_FUNC(sub_2D8D80)
{
	std::lock_guard<std::mutex> lock(g_threadDictsMutex);
	uint32_t textureModule = X86_LOAD_U32(0x55f634);
	uint32_t DictBackup = X86_LOAD_U32(textureModule + 0x57de10);

	auto thread = GetCurrentDictionaryThreadUnsafe();

	if (thread && thread != DictBackup)
		X86_STORE_U32(textureModule + 0x57de10, thread);

	__imp__sub_2D8D80(ctx, base);

	auto newDictState = X86_LOAD_U32(textureModule + 0x57de10);

	if (thread && newDictState && thread != DictBackup)
		X86_STORE_U32(textureModule + 0x57de10, DictBackup);
}


// RwTextureRead
X86_FUNC_IMPL(__imp__sub_2D8470);
X86_FUNC(sub_2D8470)
{
	__imp__sub_2D8470(ctx, base);
}

/* --- CSD Node Race --- */


struct CSDTransfoMatrixNode
{
	float xA1;
	DWORD xA2;
	BYTE gap8[4];
	float yA1;
	float yA2;
	float float14;
	uint32_t pdword18;
	uint32_t pdword1C;
};

CSDTransfoMatrixNode* csdTransfoMatrixNodeSourcePtr = nullptr;

//CSDPushNewNode
static std::atomic<int> lastThreadIDCSDNode{0};
static std::atomic<int> countExtraSleepFailSafe{0};
X86_FUNC_IMPL(__imp__sub_2B4440);
X86_FUNC(sub_2B4440)
{
	// hacky fix to prevent the CSD node transformation function to be called by two threads at the same time, 
	// which can cause some random nodes to be corrupted and crash the game when it tries to read them.
	// mutex sadly does not work
	while (lastThreadIDCSDNode && GuestThread::GetCurrentThreadId() != lastThreadIDCSDNode)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		countExtraSleepFailSafe++;

		// this shouldn't be reached ever, tests show that it never sleep more than twice per loading, but you never know.
		if (countExtraSleepFailSafe > 300)
		{
			countExtraSleepFailSafe = 0;
			break;
		}
	}

	lastThreadIDCSDNode = GuestThread::GetCurrentThreadId();
	csdTransfoMatrixNodeSourcePtr = (CSDTransfoMatrixNode*)g_memory.Translate(X86_LOAD_U32(0x559E94));

	RtlEnterCriticalSection(&csdNodeLock);
	__imp__sub_2B4440(ctx, base);
	RtlLeaveCriticalSection(&csdNodeLock);

}

//CSDPopNode
X86_FUNC_IMPL(__imp__sub_2B3ED0);
X86_FUNC(sub_2B3ED0)
{
	RtlEnterCriticalSection(&csdNodeLock);
	__imp__sub_2B3ED0(ctx, base);
	RtlLeaveCriticalSection(&csdNodeLock);

	lastThreadIDCSDNode = 0;
}


// this function is where the crash originally happens
// on top of the sleep added above, an extra failsafe is added here in case this wasn't enough
// this will be used mostly for debugging purpose.
X86_FUNC_IMPL(__imp__sub_2B39A0);
X86_FUNC(sub_2B39A0)
{
	uint32_t transfoNodeSource = X86_LOAD_U32(0x559e94);
	uint32_t transfoNodeDest = X86_LOAD_U32(0x559e98);

	if (transfoNodeSource != transfoNodeDest)
	{
		uint32_t transfoNodeDest1C = X86_LOAD_U32(transfoNodeDest + 0x1c);

		if (transfoNodeDest1C)
		{
			uint32_t pword1C = X86_LOAD_U32(transfoNodeSource + 0x1c);
			if (!pword1C)
			{
				ctx.eax.u32 = transfoNodeSource;
				LOGFN_ERROR("pword1C is null, skipping CSD node transformation\n", "CSD");
				ctx.AddEsp(1);
				return;
			}
		}
	}


	__imp__sub_2B39A0(ctx, base);

}

std::atomic<bool> aboutToDrawLoading{false};
X86_FUNC_IMPL(__imp__sub_59BB0);
X86_FUNC(sub_59BB0)
{
	aboutToDrawLoading = true;
	loadingThreadID = GuestThread::GetCurrentThreadId();
	__imp__sub_59BB0(ctx, base);
	aboutToDrawLoading = false;
}

// attempt to fix a rare FMV crash when skipping them (Multi-threads race too)
X86_FUNC_IMPL(__imp__sub_34AB40);
X86_FUNC(sub_34AB40)
{
	RtlEnterCriticalSection(&fmvPtrDataLock);

	auto a1 = X86_LOAD_U32(ctx.esp.u32 + 4);

	if (!a1)
	{
		ctx.esp.u32 += 4;
		RtlLeaveCriticalSection(&fmvPtrDataLock);
		return;
	}

	__imp__sub_34AB40(ctx, base);

	RtlLeaveCriticalSection(&fmvPtrDataLock);
}

// part 2 
// MWSFSVR_IsMwsstStatError
X86_FUNC_IMPL(__imp__sub_34C140);
X86_FUNC(sub_34C140)
{
	RtlEnterCriticalSection(&fmvPtrDataLock);

	auto a1 = X86_LOAD_U32(ctx.esp.u32 + 4);

	if (!a1)
	{
		ctx.esp.u32 += 4;
		RtlLeaveCriticalSection(&fmvPtrDataLock);
		return;
	}

	__imp__sub_34C140(ctx, base);

	RtlLeaveCriticalSection(&fmvPtrDataLock);
}

// it seems like the global camera and global camera world can have a race issue during loading
// as always we mutex and add fail safe

// CameraBeginUpdate
X86_FUNC_IMPL(__imp__sub_2DCDE0);
X86_FUNC(sub_2DCDE0)
{
	RtlEnterCriticalSection(&globalcurCamera);

	__imp__sub_2DCDE0(ctx, base);

	if (!ctx.eax.u32) // end update won't be called if function failed
	{
		LOGFN("camera update failed, leaving early", "CameraUpdate");
		RtlLeaveCriticalSection(&globalcurCamera);
	}

}

// CameraEndUpdate
X86_FUNC_IMPL(__imp__sub_2DCDB0);
X86_FUNC(sub_2DCDB0)
{
	__imp__sub_2DCDB0(ctx, base);
	RtlLeaveCriticalSection(&globalcurCamera);
}

// WorldCameraBeginUpdate
X86_FUNC_IMPL(__imp__sub_2CBA40);
X86_FUNC(sub_2CBA40)
{
	RtlEnterCriticalSection(&globalCurWorld);
	__imp__sub_2CBA40(ctx, base);

	if (!ctx.eax.u32) // end update won't be called if function failed
	{
		LOGFN("camera world update failed, leaving early", "CameraUpdate");
		RtlLeaveCriticalSection(&globalCurWorld);
	}

}

// WorldCameraEndUpdate
X86_FUNC_IMPL(__imp__sub_2CBA70);
X86_FUNC(sub_2CBA70)
{
	__imp__sub_2CBA70(ctx, base);
	RtlLeaveCriticalSection(&globalCurWorld);
}

X86_FUNC_IMPL(__imp__sub_2D4A90);
X86_FUNC(sub_2D4A90)
{
	auto res = X86_LOAD_U32(0x57DE04);
	if (!res)
	{
		LOG_ERROR("sub_2D4A90: globalCurWorld was null, aborting function for safety.\n");
		ctx.eax.u32 = 0;
		return; // no need to adjust esp this function is originally a jmp tail
	}

	__imp__sub_2D4A90(ctx, base);

}

// left over in case that happens again
X86_FUNC_IMPL(__imp__sub_2DD080);
X86_FUNC(sub_2DD080)
{
	auto a1 = X86_LOAD_U32(ctx.esp.u32 + 4);
	if (!a1 || a1 < 0x10000) //safety check
	{
		LOGFN_ERROR("RwCameraFrustumTestSphere it's over..\n", "GG");
	}
	__imp__sub_2DD080(ctx, base);
}

int GetLoadingThreadID()
{
	return loadingThreadID;
}