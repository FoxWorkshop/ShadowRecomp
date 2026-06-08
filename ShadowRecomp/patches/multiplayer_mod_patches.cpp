#include "global.h"
#include <kernel/function.h>
#include <user/config.h>

// Uncomment the define below to enable
#define EnableMultiplayerMod

#ifdef EnableMultiplayerMod
// Skip 2P Intro Event by patching StageAction::Change - NOTE DANGEROUS!! Should be replaced later
X86_FUNC_IMPL(__imp__sub_16DB80);
X86_FUNC(sub_16DB80)
{
	if (Config::Enable2PLite)
	{
		auto thisAddr = ctx.ecx.u32;
		auto stageAction = (int)GetHostParameter(2, CallConvention::Thiscall, ParameterType::Integer);
		if (stageAction == 0xC) {
			X86_STORE_U8(ctx.esp.u32 + 4, 0x3);
		}
	}
	__imp__sub_16DB80(ctx, base);
}

// Patch the TBlock / Chunk to check P2 if P1 returns false
X86_FUNC_IMPL(__imp__sub_12700);
X86_FUNC(sub_12700)
{
	auto beforeCTX = ctx;
	__imp__sub_12700(ctx, base); // P1

	if (Config::Enable2PLite)
	{
		if (ctx.eax.u8_lo == 0 && playerPos) { // Note: Runs twice now in SP - when we migrate to static playerCount we can change this to exit with < 2
			ctx = beforeCTX; // restore ctx to before original call, so we can call it again with new params
			auto playerPosGuest = g_memory.MapVirtual(playerPos); // NOTE: HACK - playerPos is last Shadow allocated, which is P2 in the case of MP; But we should change this to be a proper grab later
			X86_STORE_U32(ctx.esp.u32 + 12, playerPosGuest);
			__imp__sub_12700(ctx, base); // P2
		}
	}
}


X86_FUNC_IMPL(__imp__sub_226E50);
// sub_226E50 // Skip all event loading - Currently required for 2P Lite
bool skipAllEventLoading(DWORD* this_, int* a2, int a3, char a4)
{
	if (Config::Enable2PLite)
	{
		return 0;
	}
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	__imp__sub_226E50(*ctx, base);
	ctx->esp.u32 -= 16;
	return ctx->eax.u8_lo;
}
GUEST_FUNCTION_HOOK_THISCALL(sub_226E50, skipAllEventLoading);


// 2P Lite Stuff
/*

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
// 23 Stage Slots [0-23]
// 17 Boss Slots [23-39] - Note, skip 39 because Devil Doom has special rules we need to account for
// Final Haunt StageTable GameMode 0x463628
auto stageTableGameMode = 0x462F98;

// GameInit - Temp for sound listener etc, but invokes event and invokes 2P music and UI etc
X86_FUNC_IMPL(__imp__sub_16FEF0);
X86_FUNC(sub_16FEF0)
{
	if (Config::Enable2PLite)
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
	if (Config::Enable2PLite)
	{
		for (int i = 0; i <= 38; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_170850(ctx, base);
}

// Patch CSD start to '0'
X86_FUNC_IMPL(__imp__sub_281C50);
X86_FUNC(sub_281C50)
{
	if (Config::Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 0);
		}
		for (int i = 23; i <= 38; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 1);
		}
	}
	__imp__sub_281C50(ctx, base);
}

// Patch Spawn to '6' | TODO: Mid-asm hook needed for TXD Fix!
X86_FUNC_IMPL(__imp__sub_281FE0);
X86_FUNC(sub_281FE0)
{
	if (Config::Enable2PLite)
	{
		for (int i = 0; i <= 38; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_281FE0(ctx, base);
}

// Patch End to '0'
X86_FUNC_IMPL(__imp__sub_16F7E0);
X86_FUNC(sub_16F7E0)
{
	if (Config::Enable2PLite)
	{
		for (int i = 0; i <= 38; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_16F7E0(ctx, base);
}

// Unpause Split Mode
X86_FUNC_IMPL(__imp__sub_238650);
X86_FUNC(sub_238650)
{
	if (Config::Enable2PLite)
	{
		for (int i = 0; i <= 38; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 6);
		}
	}
	__imp__sub_238650(ctx, base);
}

// On Death
X86_FUNC_IMPL(__imp__sub_2385E0);
X86_FUNC(sub_2385E0)
{
	if (Config::Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 0);
		}
		for (int i = 23; i <= 38; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 1);
		}
	}
	__imp__sub_2385E0(ctx, base);
}

// On Goal Handling
X86_FUNC_IMPL(__imp__sub_16E0D0);
X86_FUNC(sub_16E0D0)
{
	if (Config::Enable2PLite)
	{
		for (int i = 0; i <= 22; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 0);
		}
		for (int i = 23; i <= 38; i++) {
			X86_STORE_U32(stageTableGameMode + (0x50 * i), 1);
		}
	}
	__imp__sub_16E0D0(ctx, base);
}

// TEMP Workaround for On Goal Invoke with Bad GameMode
X86_FUNC_IMPL(__imp__sub_237DA0);
X86_FUNC(sub_237DA0)
{
	if (Config::Enable2PLite)
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

// Level Geo Unrestrict (For Custom Stages)
// Note: This will break some stages, e.g. Egg Dealer (Black Comet) and if you die in a certain area in GUN Fortress.
// This is because Sonic Team left visibility chunks pointing to old files... which are usually ignore due to this file restriction function.
//X86_FUNC_IMPL(__imp__sub_56E00);
//X86_FUNC(sub_56E00)
//{
//	auto isDigit = [](char c) { return c >= '0' && c <= '9'; };
//
//	uint32_t strAddr = X86_LOAD_U32(ctx.esp.u32 + 4);
//	const char* str = strAddr ? (const char*)(base + strAddr) : nullptr;
//
//	if (str)
//	{
//		for (const char* p = str; *p; p++)
//		{
//			if ((p[0] == 's' || p[0] == 'S') &&
//				(p[1] == 't' || p[1] == 'T') &&
//				(p[2] == 'g' || p[2] == 'G') &&
//				isDigit(p[3]) && isDigit(p[4]) &&
//				isDigit(p[5]) && isDigit(p[6]) &&
//				p[7] == '_' &&
//				isDigit(p[8]) && isDigit(p[9]))
//			{
//				ctx.eax.u32 = 1;
//				ctx.esp.u32 += 4;
//				return;
//			}
//		}
//	}
//
//	__imp__sub_56E00(ctx, base);
//}



// Collision BVH Stack Overflow Fix
// Original sub_302E30 inner BVH stack lives on the host stack (~32 entries).
// Large collision meshes create BVH trees deeper than 32 levels, silently
// overflowing into other stack variables and crashing.
// 
// Redirects the BVH stack to a static guest memory buffer using mid-asm hooks.
// [[midasm_hook]] entries at 0x303375 and 0x30392B

// static constexpr uint32_t COLL_BVH_BUF_ADDR = 0x80000000;
//
// void BVHStackRedirectInit(X86Register& ecx, X86Register& esi)
// {
// 	auto base = g_memory.base;
//
// 	float tNear = X86_LOAD_F32(ecx.u32 + 4);
// 	float tFar = X86_LOAD_F32(ecx.u32 + 8);
//
// 	X86_STORE_U32(COLL_BVH_BUF_ADDR + 0, 0x0000FF00);
// 	X86_STORE_U32(COLL_BVH_BUF_ADDR + 4, COLL_BVH_BUF_ADDR);
// 	X86_STORE_F32(COLL_BVH_BUF_ADDR + 8, tNear);
// 	X86_STORE_F32(COLL_BVH_BUF_ADDR + 12, tFar);
//
// 	esi.u32 = COLL_BVH_BUF_ADDR + 16;
// }
//
// void BVHStackRedirectCmp(X86Register& eax)
// {
// 	eax.u32 = COLL_BVH_BUF_ADDR + 4;
// }

#endif