#include "global.h"
#include <kernel/function.h>

const bool Enable2PLite = false;
// Patch the TBlock / Chunk to check P2 if P1 returns false
X86_FUNC_IMPL(__imp__sub_12700);
X86_FUNC(sub_12700)
{
	auto beforeCTX = ctx;
	__imp__sub_12700(ctx, base); // P1
	if (Enable2PLite)
	{
		if (ctx.eax.u8_lo == 0 && playerPos) { // Note: Runs twice now in SP - when we migrate to static playerCount we can change this to exit with < 2
			ctx = beforeCTX; // restore ctx to before original call, so we can call it again with new params
			auto playerPosGuest = g_memory.MapVirtual(playerPos); // NOTE: HACK - playerPos is last Shadow allocated, which is P2 in the case of MP; But we should change this to be a proper grab later
			X86_STORE_U32(ctx.esp.u32 + 12, playerPosGuest);
			__imp__sub_12700(ctx, base); // P2
		}
	}
}

//sub_226E50 // Skip all event loading
bool skipAllEventLoading(DWORD* this_, int *a2, int a3, char a4)
{
	return 0;
}
GUEST_FUNCTION_HOOK_THISCALL(sub_226E50, skipAllEventLoading);


// 2P Lite Stuff
/*

// 0x463088 = lethal highway gamemode (for testing)

TODO: Probably store a global to get stageTable offset per load, that way we can access it anywhere without worrying about it, or the mode.
As a bonus, we can get the 0/1 (boss or normal stage) value and use that as the 'restore' reference

-  sub_16FEF0 (PARENT) /  sub_258550+1AD // SKIP FOR NOW; Requires mid-asm hooks - StageManager Constructor sound listener, obj render dist for cam2, event play, music invoke, commandWindow, etc
-  sub_170850+6CD // stageInitialize							- 6
-  sub_164FF0+1FD // Camera allocation / assignment				- 6
-  sub_2A8D90+207 // Vibration									- 6

-  sub_281C50+106 // CSD (PauseMenu NCP file to load)			- 0
-  sub_281C50+186 // CSD (??)									- 0
-  sub_281FE0+346 // CSD (PauseMenu & Mission TXD file to load) - 0
-  sub_281FE0+3C6 // Spawn Players								- 6
-  sub_16F7E0+252												- 0


16E0D0+74
2385E0+B3
237DA0+1654
237DA0+1687
226D20+18B
237DA0+156E (every result frame) -> expects 2P_UI,SoundListener/RenderDist Flag to be '0' to operate GoalClear
*/

// Westopolis StageTable GameMode 0x462F98
// 0x50 per table instance, 0-22 = sets 0100 through 0700 [no bosses]
// Final Haunt StageTable GameMode 0x463628
auto stageTableGameMode = 0x462F98;

// GameInit - Temp for sound listener etc, but invokes event and invokes 2P music and UI etc
X86_FUNC_IMPL(__imp__sub_16FEF0);
X86_FUNC(sub_16FEF0)
{
	if (Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_16FEF0(ctx, base);
}

// Patch StageInitialize to Assume '6' game mode (probably breaks for bosses?)
X86_FUNC_IMPL(__imp__sub_170850);
X86_FUNC(sub_170850)
{
	if (Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_170850(ctx, base);
}

// Patch CSD start to '0'
X86_FUNC_IMPL(__imp__sub_281C50);
X86_FUNC(sub_281C50)
{
	if (Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 0);
		}
	}
	__imp__sub_281C50(ctx, base);
}

// Patch Spawn to '6' | TODO: Mid-asm hook needed for TXD Fix!
X86_FUNC_IMPL(__imp__sub_281FE0);
X86_FUNC(sub_281FE0)
{
	if (Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_281FE0(ctx, base);
}

// Patch End to '0'
X86_FUNC_IMPL(__imp__sub_16F7E0);
X86_FUNC(sub_16F7E0)
{
	if (Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_16F7E0(ctx, base);
}

// Unpause Split Mode
X86_FUNC_IMPL(__imp__sub_238650);
X86_FUNC(sub_238650)
{
	if (Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_238650(ctx, base);
}

// On Death
X86_FUNC_IMPL(__imp__sub_2385E0);
X86_FUNC(sub_2385E0)
{
	if (Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 0);
		}
	}
	__imp__sub_2385E0(ctx, base);
}

// On Goal Handling
X86_FUNC_IMPL(__imp__sub_16E0D0);
X86_FUNC(sub_16E0D0)
{
	if (Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 0);
		}
	}
	__imp__sub_16E0D0(ctx, base);
}

// TEMP Workaround for On Goal Invoke with Bad GameMode
X86_FUNC_IMPL(__imp__sub_237DA0);
X86_FUNC(sub_237DA0)
{
	if (Enable2PLite)
	{
		X86_STORE_U8(0x54DDE2, 0); // Disable 2P_UI, SoundListener/RenderDist Flag for GoalClear
	}
	__imp__sub_237DA0(ctx, base);
}

// Unpause->Set Split Mode
// Need to handle splitscreen setting, or just perma override it. Probably better to handle it instead
// sub_5AAF0 + F2B
// sub_238650 + 23D

// End 2P Lite Stuff
