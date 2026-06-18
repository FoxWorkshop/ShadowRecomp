#include <ui/game_window.h>
#include <user/config.h>
#include <app.h>
#include <sdl_events.h>
#include "global.h"

PlayerShadow* playerPtr[4] = {};
RwV3d* playerPos[4] = {};
bool isAwakeAboutToInvoke = false;
static uint32_t s_playerSlotIndex = 0;
static uint32_t s_knownThisPtrs[4] = {};
static uint32_t s_knownCount = 0;

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
X86_FUNC_IMPL(__imp__sub_8E690);
X86_FUNC(sub_8E690)
{
	auto mode = X86_LOAD_U32(ctx.esp.u32 + 16);
	isAwakeAboutToInvoke = true;
	__imp__sub_8E690(ctx, base);
	App::s_chaosMode = mode == 0 ? ELedAura::LED_AURA_VILLAIN : ELedAura::LED_AURA_HERO;
	SDL_User_ChaosMode(App::s_chaosMode);
}

//Awake destructor
X86_FUNC_IMPL(__imp__sub_8E9A0);
X86_FUNC(sub_8E9A0)
{
	__imp__sub_8E9A0(ctx, base);
	App::s_chaosMode = ELedAura::LED_AURA_NEUTRAL;
	SDL_User_ChaosMode(App::s_chaosMode);
}

X86_FUNC_IMPL(__imp__sub_96AD0);
X86_FUNC(sub_96AD0)
{
	auto playerAddr = ctx.ecx.u32;
	__imp__sub_96AD0(ctx, base);

	uint32_t slot = UINT32_MAX;

	for (uint32_t i = 0; i < s_knownCount; i++)
	{
		if (s_knownThisPtrs[i] == playerAddr)
		{
			slot = i;
			break;
		}
	}

	if (slot == UINT32_MAX)
	{
		bool foundAny = false;
		for (uint32_t i = 0; i < s_knownCount; i++)
		{
			if (s_knownThisPtrs[i] != 0)
			{
				foundAny = true;
				break;
			}
		}

		if (foundAny && s_knownCount >= g_multiplayerPlayerCount)
		{
			memset(s_knownThisPtrs, 0, sizeof(s_knownThisPtrs));
			for (uint32_t i = 0; i < 4; i++)
			{
				playerPtr[i] = nullptr;
				playerPos[i] = nullptr;
			}
			s_knownCount = 0;
		}

		slot = s_knownCount++;
	}

	s_playerSlotIndex = (slot + 1) % g_multiplayerPlayerCount;
	s_knownThisPtrs[slot] = playerAddr;

	playerPtr[slot] = (PlayerShadow*)X86_LOAD_U32(playerAddr);
	playerPos[slot] = (RwV3d*)g_memory.Translate(playerAddr + 4);
#ifdef _DEBUG
	printf("Player Slot %u Pointer %llx\n", slot, (intptr_t)playerPtr[slot]);
	printf("Player Slot %u Pos Ptr %llx\n", slot, (intptr_t)playerPos[slot]);
	printf("Last Shadow Pos x Ptr %llx\n", (intptr_t)&playerPos[slot]->x);
	printf("Last Shadow Pos y Ptr %llx\n", (intptr_t)&playerPos[slot]->y);
	printf("Last Shadow Pos z Ptr %llx\n", (intptr_t)&playerPos[slot]->z);
#endif
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