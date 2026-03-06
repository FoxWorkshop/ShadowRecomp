#include <kernel/function.h>
#include <kernel/kernel.h>

// When the loading thread is running, two threads are updating the dirty link list (dword_57DEBC) which can create a race issue 
// and corrupt some random RwFrames which will eventually crash the game.
// we can fix that by adding mutex to prevent the race.
// would be nice to find a better way, but so far this is the best compromise I found


//patch for loading thread race issue
XRTL_CRITICAL_SECTION dirtyListLock;
XRTL_CRITICAL_SECTION dword_559E94Lock;
void InitPatchesLoadingCriticalSection()
{
	RtlInitializeCriticalSection(&dirtyListLock);
	RtlInitializeCriticalSection(&dword_559E94Lock);
}


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

//test to do check if it work
X86_FUNC_IMPL(__imp__sub_2B39A0);
X86_FUNC(sub_2B39A0)
{
	RtlEnterCriticalSection(&dword_559E94Lock);
	__imp__sub_2B39A0(ctx, base);
	RtlLeaveCriticalSection(&dword_559E94Lock);
}

X86_FUNC_IMPL(__imp__sub_2B4440);
X86_FUNC(sub_2B4440)
{
	RtlEnterCriticalSection(&dword_559E94Lock);
	__imp__sub_2B4440(ctx, base);
	RtlLeaveCriticalSection(&dword_559E94Lock);
}

X86_FUNC_IMPL(__imp__sub_2B3A70);
X86_FUNC(sub_2B3A70)
{
	RtlEnterCriticalSection(&dword_559E94Lock);
	__imp__sub_2B3A70(ctx, base);
	RtlLeaveCriticalSection(&dword_559E94Lock);
}

//maybe sub_2B42C0 with dword_559E90

