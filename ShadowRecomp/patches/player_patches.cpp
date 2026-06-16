#include <ui/game_window.h>
#include <user/config.h>
#include <app.h>
#include <sdl_events.h>
#include "global.h"

PlayerShadow* playerPtr = nullptr;
RwV3d* playerPos = nullptr;

//Awake Set Chaos Control / Blast mode
X86_FUNC_IMPL(__imp__sub_8E690);
X86_FUNC(sub_8E690)
{
	auto mode = X86_LOAD_U32(ctx.esp.u32 + 16);
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
	playerPtr = (PlayerShadow*)X86_LOAD_U32(playerAddr);
	playerPos = (RwV3d*)g_memory.Translate(playerAddr + 4);
#ifdef _DEBUG
	printf("Last Shadow Pointer %llx\n", (intptr_t)playerPtr);
	printf("Last Shadow Pos Ptr %llx\n", (intptr_t)playerPos);
	printf("Last Shadow Pos x Ptr %llx\n", (intptr_t)&playerPos->x);
	printf("Last Shadow Pos y Ptr %llx\n", (intptr_t)&playerPos->y);
	printf("Last Shadow Pos z Ptr %llx\n", (intptr_t)&playerPos->z);
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


	if (playerPos && !oncePos && mode == 2)
	{
		/**playerPos->x = 6.004902;
		playerPos->y = -494.980072;
		playerPos->z = -411.312012;
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