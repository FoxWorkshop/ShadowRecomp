#include <kernel/function.h>
#include <kernel/kernel.h>

// When the loading thread is running, two threads are updating the dirty link list (dword_57DEBC) which can create a race issue 
// and corrupt some random RwFrames which will eventually crash the game.
// we can fix that by adding mutex to prevent the race.
// would be nice to find a better way, but so far this is the best compromise I found


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
			EnterDirtyListCriticalSection();
			valid = true;
		}
	}

	__imp__sub_2E1300(ctx, base);

	if (valid)
		LeaveDirtyListCriticalSection();
}

//RwFrameSyncDirty
X86_FUNC_IMPL(__imp__sub_2EC720);
X86_FUNC(sub_2EC720)
{
	EnterDirtyListCriticalSection();
	__imp__sub_2EC720(ctx, base);
	LeaveDirtyListCriticalSection();
}

