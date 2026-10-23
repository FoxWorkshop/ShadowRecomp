#include <kernel/function.h>
#include <app.h>
#include <sdl_events.h>
#include <input/input.h>
#include "global.h"

uint32_t g_playerStructAddrs[4] = {};
bool isAwakeAboutToInvoke = false;

RwV3d* GetPlayerPos(uint32_t slot)
{
	return g_playerStructAddrs[slot] != 0 ? (RwV3d*)g_memory.Translate(g_playerStructAddrs[slot] + 4) : nullptr;
}

// clear stage Shift+F5
// TODO: remove before release
void TriggerGoalAction()
{
	static bool f5Held = false;
	auto base = g_memory.base;
	bool f5 = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F5] != 0;
	if (f5 && !f5Held && (SDL_GetModState() & KMOD_SHIFT))
	{
		X86_STORE_U32(0x5496C4, 6);
	}
	f5Held = f5;
}

//Awake Set Chaos Control / Blast mode
X86_FUNC_IMPL(__imp__sub_8E690);
X86_FUNC(sub_8E690)
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
	printf("Player Slot %u Struct %x Ptr %llx\n", slot, playerAddr, (intptr_t)GetPlayerPos(slot));
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

	TriggerGoalAction();

	RwV3d* p1Pos = GetPlayerPos(0);
	if (p1Pos && !oncePos && mode == 2)
	{
		/**p1Pos->x = 6.004902;
		p1Pos->y = -494.980072;
		p1Pos->z = -411.312012;
		oncePos = true;*/
	}
}

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
