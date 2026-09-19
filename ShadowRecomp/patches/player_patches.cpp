#include <kernel/function.h>
#include <ui/game_window.h>
#include <user/config.h>
#include <app.h>
#include <sdl_events.h>
#include <input/input.h>
#include <input/padstate.h>
#include "global.h"

PlayerShadow* playerPtr[4] = {};
RwV3d* playerPos[4] = {};
bool isAwakeAboutToInvoke = false;
uint32_t g_playerStructAddrs[4] = {};

void TeleportPlayersToP1()
{
	static bool s_f5Held = false;
	bool f5 = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F5] != 0;
	if (f5 && !s_f5Held && playerPos[0])
	{
		float x = playerPos[0]->x;
		float y = playerPos[0]->y;
		float z = playerPos[0]->z;
		for (uint32_t i = 1; i < g_multiplayerPlayerCount; i++)
		{
			if (playerPos[i])
			{
				playerPos[i]->x = x;
				playerPos[i]->y = y;
				playerPos[i]->z = z;
			}
		}
	}
	s_f5Held = f5;
}

//Awake Set Chaos Control / Blast mode
X86_FUNC_IMPL (__imp__sub_8E690);
X86_FUNC (sub_8E690)
{
	isAwakeAboutToInvoke = true;
	auto mode = X86_LOAD_U32(ctx.esp.u32 + 16);
	__imp__sub_8E690(ctx, base);

	input::SetChaosAura(mode == 0 ? ELedAura::LED_AURA_VILLAIN : ELedAura::LED_AURA_HERO);

}

//Awake destructor
X86_FUNC_IMPL(__imp__sub_8E9A0);
X86_FUNC(sub_8E9A0)
{
	__imp__sub_8E9A0(ctx, base);
	App::s_chaosMode = ELedAura::LED_AURA_NEUTRAL;
}

// Player ctor
X86_FUNC_IMPL(__imp__sub_96AD0);
X86_FUNC(sub_96AD0)
{
	auto playerAddr = ctx.ecx.u32;
	__imp__sub_96AD0(ctx, base);
	int slot = -1;
	int freeSlot = -1;
	for (uint32_t i = 0; i < 4; i++)
	{
		// This should never happen (ctor on level init)
		// But just incase (maybe something weird with extended multiplayer / events later)
		if (g_playerStructAddrs[i] == playerAddr)
		{
			slot = i;
			break;
		}

		if (g_playerStructAddrs[i] == 0 && freeSlot == -1)
		{
			freeSlot = i;
		}
	}

	if (slot == -1)
	{
		slot = freeSlot;
	}

	if (slot == -1) // exit early (only track 4 slots)
	{
		return;
	}

	g_playerStructAddrs[slot] = playerAddr;
	playerPtr[slot] = (PlayerShadow*)X86_LOAD_U32(playerAddr);
	playerPos[slot] = (RwV3d*)g_memory.Translate(playerAddr + 4);
	printf("Player Slot %u Pointer %llx\n", slot, (intptr_t)playerPtr[slot]);
	printf("Player Slot %u Pos Ptr %llx\n", slot, (intptr_t)playerPos[slot]);
}

// Player dtor @ sub_97320 (not this func)
// Player ptr ctor created passed to sub_96B50 (this func), use to mark destroyed players
X86_FUNC_IMPL(__imp__sub_96B50);
X86_FUNC(sub_96B50)
{
	auto playerAddr = ctx.ecx.u32;
	__imp__sub_96B50(ctx, base);

	for (uint32_t i = 0; i < 4; i++)
	{
		if (g_playerStructAddrs[i] == playerAddr)
		{
			g_playerStructAddrs[i] = 0;
			playerPtr[i] = nullptr;
			playerPos[i] = nullptr;
			printf("Player Slot %u Destroyed %x\n", i, playerAddr);
			break;
		}
	}
}

//edit this if you want to set a specific start position for the player
bool oncePos = false;
X86_FUNC_IMPL(__imp__sub_90D10);
X86_FUNC(sub_90D10)
{
	auto a2Addr = X86_LOAD_U32(ctx.esp.u32 + 4);
	auto mode = X86_LOAD_U16(a2Addr + 4);
	__imp__sub_90D10(ctx, base);

	TeleportPlayersToP1();

	if (playerPos[0] && !oncePos && mode == 2)
	{
		/**playerPos[0]->x = 6.004902;
		playerPos[0]->y = -494.980072;
		playerPos[0]->z = -411.312012;
		oncePos = true;*/
	}
}

#ifdef Cheats
// cheat infinite ammo
X86_FUNC_IMPL(__imp__sub_9FC00);
X86_FUNC(sub_9FC00)
{
	__imp__sub_9FC00(ctx, base);

	X86_STORE_U32(0x5456fc, 999);
	ctx.eax.u32 = 999;
	auto res = X86_LOAD_U32(0x5456D0);

	// infinite chaos blast
	res += 0x100;
	X86_STORE_U32(0x5456D0, res);
}
#endif

// Originally clones SHADOW_FOOTEFF_R.DFF 3 times, make it clone a 4th and assign to slot 0x68 (bone 10).
// Constructor: 0x8DB90 [aka anonymous_namespace::Jet::Jet]
// NOTE: Destructor/Hide/Show don't need patches because the game expects all 4 slots to be filled but 4th is null in OG
// Destructor: 0x8D6B0
// Show Flame: 0x8CC50
// Hide Flame: 0x8CC80
// Slots: 0x5C (bone 7), 0x60 (bone 8), 0x64 (bone 9), 0x68 (bone 10)
void JetEffectRestoreFourthFlameMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	const uint32_t jet = esi.u32;
	const uint32_t firstFlame = X86_LOAD_U32(jet + 0x5C);
	if (firstFlame != 0 && X86_LOAD_U32(jet + 0x68) == 0)
	{
		// 0x2CB460 - model clone func
		// TODO: Probably re-use this for Awake effect clone too for MP patches
		const uint32_t clone = GuestToHostFunction<uint32_t, CallConvention::Cdecl>(sub_2CB460, firstFlame);
		if (clone != 0)
		{
			X86_STORE_U32(jet + 0x68, clone);
		}
	}
}

// CreateBehavior (all player actions)
//
// Table of all PlayerBehavior @ 0x4EEE58
// 0x6 = JumpDash
// 0x8 = HomingAttack
// TODO: We will extend this further to enable state transitions like SpinDash->Slide etc in the future\
// TODO: Probably move this to ReloadedPatches later, but for now keep it in
X86_FUNC_IMPL(__imp__sub_977D0);
X86_FUNC(sub_977D0)
{
	// esp+8 = BehaviorId
	// ecx+4 = PlayerStruct targetted by Behavior
	if (X86_LOAD_U32(ctx.esp.u32 + 8) == 8) // HomingAttack
	{
		uint32_t player = X86_LOAD_U32(ctx.ecx.u32 + 4);
		for (uint32_t i = 0; i < 4; i++) // Checking player structs 1-4 TODO: Maybe grab the extended multiplayer value later?
		{
			if (g_playerStructAddrs[i] == player)
			{
				// TODO: Account for this if we do Action remapping; Input remapping should be fine to auto handle this
				if (PJSPeripheral::IsHeld(i, PJSPeripheral::BTN_LEFT_TRIG) ||
					PJSPeripheral::IsHeld(i, PJSPeripheral::BTN_WHITE))
				{
					X86_STORE_U32(ctx.esp.u32 + 8, 6); // Change target Behavior to JumpDash
				}
				break;
			}
		}
	}
	__imp__sub_977D0(ctx, base);
}
