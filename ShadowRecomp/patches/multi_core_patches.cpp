#include <kernel/function.h>
#include <kernel/kernel.h>
#include <gpu/video.h>
#include <cpu/guest_thread.h>


// While OG Xbox has thread support, it was originally a mono-core console, meaning threads weren't actually running in parallel
// Thus, true multi-core creates race issues on PC, specially at load time since it has its own thread running while the main-game loop runs too.
// So we patch functions that can edit same memory to avoid race issue using mutex or other tweaks.
// It would be nice to find a better way, but so far this is the best compromise I found other than forcing mono-core, which I'd prefer not to.


static std::unordered_map<uint32_t, uint32_t> g_threadDicts;
static std::mutex g_threadDictsMutex;


XRTL_CRITICAL_SECTION dirtyListLock;
XRTL_CRITICAL_SECTION csdNodeLock;
void InitPatchesLoadingCriticalSection()
{
	RtlInitializeCriticalSection(&dirtyListLock);
	RtlInitializeCriticalSection(&csdNodeLock);
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

uint32_t GetCurrentDictionaryThread()
{

	auto it = g_threadDicts.find(GuestThread::GetCurrentThreadId());
	if (it != g_threadDicts.end())
	{
		return it->second;
	}

	return 0;
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

	auto thread = GetCurrentDictionaryThread();

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


//CSDPushNewNode
X86_FUNC_IMPL(__imp__sub_2B4440);
X86_FUNC(sub_2B4440)
{
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
}

// just an attempt
X86_FUNC_IMPL(__imp__sub_2B3F00);
X86_FUNC(sub_2B3F00)
{
	RtlEnterCriticalSection(&csdNodeLock);
	__imp__sub_2B3F00(ctx, base);
	RtlLeaveCriticalSection(&csdNodeLock);
}

