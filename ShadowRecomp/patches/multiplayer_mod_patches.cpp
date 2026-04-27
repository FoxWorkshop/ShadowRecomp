#include "global.h"
#include <kernel/function.h>
#include <user/config.h>


X86_FUNC_IMPL(__imp__sub_12700);
X86_FUNC(sub_12700)
{
	auto beforeCTX = ctx;
	__imp__sub_12700(ctx, base);

	if (Config::ExtendedMultiplayer)
	{
		for (uint32_t i = 1; i < g_multiplayerPlayerCount && ctx.eax.u8_lo == 0; i++)
		{
			if (!playerPos[i])
				continue;
			ctx = beforeCTX;
			auto playerPosGuest = g_memory.MapVirtual(playerPos[i]);
			X86_STORE_U32(ctx.esp.u32 + 12, playerPosGuest);
			__imp__sub_12700(ctx, base);
		}
	}
}


//X86_FUNC_IMPL(__imp__sub_226E50);
//// sub_226E50 // Skip all event loading
//bool skipAllEventLoading(DWORD* this_, int* a2, int a3, char a4)
//{
//	auto ctx = GetX86Context();
//	auto base = g_memory.base;
//	__imp__sub_226E50(*ctx, base);
//	ctx->esp.u32 -= 16;
//	return ctx->eax.u8_lo;
//}
//GUEST_FUNCTION_HOOK_THISCALL(sub_226E50, skipAllEventLoading);

// Extended Multiplayer
// 2P, 3P, or 4P Shadow the Hedgehog
// NOTE: Allocations on game boot are required;
// So currently a restart is necessary when toggling on/off

uint32_t g_multiplayerPlayerCount = 4;

bool ExtendedMultiplayerCameraAttachMidAsmHook(X86Register& eax)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
    if (Config::ExtendedMultiplayer)
    {
        return true;
    }
    auto val = X86_LOAD_U8(eax.u32 + 8);
    return (val & 4) != 0;
}

bool ExtendedMultiplayerPlayerSpawnMidAsmHook(X86Register& eax)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
    if (Config::ExtendedMultiplayer)
    {
        return true;
    }
    auto val = X86_LOAD_U8(eax.u32 + 8);
    return (val & 4) != 0;
}

bool ExtendedMultiplayerCameraAllocationLoopMidAsmHook(X86Register& edi)
{
	uint32_t limit = Config::ExtendedMultiplayer ? g_multiplayerPlayerCount : 2;
    return edi.u32 < limit;
}

static bool s_extraSpawnActive = false;
static bool s_extraSpawnDone = false;
static uint32_t s_extraSpawnRemaining = 0;
static uint32_t s_extraSpawnPlayerIndex = 2;

bool ExtendedMultiplayerExtraSpawnMidAsmHook(X86Register& esp)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
    if (s_extraSpawnDone)
    {
        s_extraSpawnDone = false;
        s_extraSpawnActive = false;
        esp.u32 += 3 * (g_multiplayerPlayerCount - 2) * 4;
        return false;
    }
    if (!Config::ExtendedMultiplayer || g_multiplayerPlayerCount <= 2)
    {
        return false;
    }
    if (!s_extraSpawnActive)
    {
        s_extraSpawnActive = true;
        s_extraSpawnRemaining = g_multiplayerPlayerCount - 2;
        s_extraSpawnPlayerIndex = 2;
    }
    esp.u32 -= 4;
    X86_STORE_U32(esp.u32, s_extraSpawnPlayerIndex);
    esp.u32 -= 4;
    X86_STORE_U32(esp.u32, s_extraSpawnPlayerIndex);
    esp.u32 -= 4;
    X86_STORE_U32(esp.u32, 14 + s_extraSpawnPlayerIndex);
    s_extraSpawnRemaining--;
    s_extraSpawnPlayerIndex++;
    return true;
}

bool ExtendedMultiplayerExtraSpawnRedirectMidAsmHook(X86Register& esp)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
    if (s_extraSpawnActive)
    {
        if (s_extraSpawnRemaining == 0)
            s_extraSpawnDone = true;
        return true;
    }
    return false;
}

static uint32_t s_extraPlayerPositionID = 0;

void ExtendedMultiplayerPlayerPositionIDOverrideMidAsmHook(X86Register& reg)
{
    if (s_extraPlayerPositionID != 0)
        reg.u32 = s_extraPlayerPositionID;
}

bool ExtendedMultiplayerPlayerPositionLoopMidAsmHook()
{
    if (s_extraPlayerPositionID == 0)
        s_extraPlayerPositionID = 2;
    else
        s_extraPlayerPositionID++;

    if (s_extraPlayerPositionID <= g_multiplayerPlayerCount - 1)
        return true;

    s_extraPlayerPositionID = 0;
    return false;
}

static uint32_t s_assignPadPlayerId = 0;

void AssignPadPlayerIdSaveMidAsmHook(X86Register& ecx)
{
    s_assignPadPlayerId = ecx.u32;
}

void AssignPadPlayerIdOverrideMidAsmHook(X86Register& edx)
{
    edx.u32 = s_assignPadPlayerId;
}

bool PADBoundsCheckMidAsmHook(X86Register& eax)
{
    if (Config::ExtendedMultiplayer && g_multiplayerPlayerCount >= 4 && eax.u32 >= 4)
        return false;
    return true;
}

bool ViewportDivMidAsmHook(X86Register& reg)
{
    if (Config::ExtendedMultiplayer && g_multiplayerPlayerCount > 2)
    {
        reg.u32 = (uint32_t)((int32_t)reg.u32 / (int32_t)g_multiplayerPlayerCount);
        return true;
    }
    return false;
}

void ViewportExtendedPlayerCalcOverrideMidAsmHook()
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
    if (!Config::ExtendedMultiplayer || g_multiplayerPlayerCount <= 2)
        return;

    uint32_t screenWidth = X86_LOAD_U32(0x4E5D88);
    uint32_t screenHeight = X86_LOAD_U32(0x4E5D8C);
    uint32_t dimA = X86_LOAD_U32(0x541A5C);
    uint32_t dimB = X86_LOAD_U32(0x541A60);

    if (g_multiplayerPlayerCount == 4)
    {
        uint32_t w = screenWidth / 2;
        uint32_t h = screenHeight / 2;
        uint32_t dimAdiv = dimA / 2;
        uint32_t dimBdiv = dimB / 2;

        X86_STORE_U32(0x4E5DB0, 0);
        X86_STORE_U32(0x4E5DB4, 0);
        X86_STORE_U32(0x4E5DB8, w);
        X86_STORE_U32(0x4E5DBC, h);
        X86_STORE_U32(0x4E5DC0, 0);
        X86_STORE_U32(0x4E5DC4, 0);
        X86_STORE_U32(0x4E5DC8, dimAdiv);
        X86_STORE_U32(0x4E5DCC, dimBdiv);

        X86_STORE_U32(0x4E5DE0, w);
        X86_STORE_U32(0x4E5DE4, 0);
        X86_STORE_U32(0x4E5DE8, w);
        X86_STORE_U32(0x4E5DEC, h);
        X86_STORE_U32(0x4E5DF0, 0);
        X86_STORE_U32(0x4E5DF4, 0);
        X86_STORE_U32(0x4E5DF8, dimAdiv);
        X86_STORE_U32(0x4E5DFC, dimBdiv);

        X86_STORE_U32(0x4E5E10, 0);
        X86_STORE_U32(0x4E5E14, h);
        X86_STORE_U32(0x4E5E18, w);
        X86_STORE_U32(0x4E5E1C, h);
        X86_STORE_U32(0x4E5E20, 0);
        X86_STORE_U32(0x4E5E24, 0);
        X86_STORE_U32(0x4E5E28, dimAdiv);
        X86_STORE_U32(0x4E5E2C, dimBdiv);

        X86_STORE_U32(0x4E5E40, w);
        X86_STORE_U32(0x4E5E44, h);
        X86_STORE_U32(0x4E5E48, w);
        X86_STORE_U32(0x4E5E4C, h);
        X86_STORE_U32(0x4E5E50, 0);
        X86_STORE_U32(0x4E5E54, 0);
        X86_STORE_U32(0x4E5E58, dimAdiv);
        X86_STORE_U32(0x4E5E5C, dimBdiv);
    }
    else
    {
        uint32_t w = screenWidth / g_multiplayerPlayerCount;
        uint32_t x = 2 * w;
        uint32_t dimAdiv = dimA / g_multiplayerPlayerCount;

        X86_STORE_U32(0x4E5E10, x);
        X86_STORE_U32(0x4E5E18, w);
        X86_STORE_U32(0x4E5E1C, screenHeight);
        X86_STORE_U32(0x4E5E2C, dimB);
        X86_STORE_U32(0x4E5E44, screenHeight);
        X86_STORE_U32(0x4E5E48, w);
        X86_STORE_U32(0x4E5E4C, screenHeight);
        X86_STORE_U32(0x4E5E54, dimB);
        X86_STORE_U32(0x4E5E58, dimAdiv);
        X86_STORE_U32(0x4E5E5C, dimB);
    }
} 

static uint32_t s_renderDivExtraPass = 0;
static uint32_t s_renderDivSavedThis = 0;

void RenderDivSaveThisMidAsmHook(X86Register& edi)
{
    s_renderDivSavedThis = edi.u32;
}

bool RenderDivExtraPassMidAsmHook(X86Register& edi)
{
    if (Config::ExtendedMultiplayer && g_multiplayerPlayerCount > 2)
    {
        if (s_renderDivExtraPass < g_multiplayerPlayerCount - 2)
        {
            s_renderDivExtraPass++;
            edi.u32 = s_renderDivSavedThis;
            return true;
        }
        s_renderDivExtraPass = 0;
    }
    return false;
}

bool RenderDivCameraLookupMidAsmHook(X86Register& eax)
{
    if (s_renderDivExtraPass > 0)
    {
        eax.u32 = s_renderDivExtraPass + 1;
        return true;
    }
    return false;
}

bool RenderDivPlayerOverrideMidAsmHook(X86Register& esp)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
    if (Config::ExtendedMultiplayer && g_multiplayerPlayerCount > 2 && s_renderDivExtraPass > 0)
    {
        esp.u32 -= 4;
        X86_STORE_U32(esp.u32, s_renderDivExtraPass + 2);
        return true;
    }
    return false;
}

bool RenderDivCameraTargetOverrideMidAsmHook()
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
    if (Config::ExtendedMultiplayer && g_multiplayerPlayerCount > 2 && s_renderDivExtraPass > 0)
    {
        X86_STORE_U32(0x54DD9C, s_renderDivExtraPass + 2);
        return true;
    }
    return false;
}

static uint32_t s_cameraAttachPlayerIndex = 0;

bool CameraAttachSetupMidAsmHook(X86Register& ebp)
{
    auto ctx = GetX86Context();
    auto base = g_memory.base;
    if (Config::ExtendedMultiplayer)
    {
        X86_STORE_U32(ebp.u32, g_multiplayerPlayerCount);
        if (s_cameraAttachPlayerIndex == 0)
            s_cameraAttachPlayerIndex = 1;
        return true;
    }
    return false;
}

bool CameraAttachPlayerIdMidAsmHook(X86Register& esp)
{
    auto ctx = GetX86Context();
    auto base = g_memory.base;
    if (!Config::ExtendedMultiplayer)
        return false;
    esp.u32 -= 4;
    X86_STORE_U32(esp.u32, s_cameraAttachPlayerIndex);
    s_cameraAttachPlayerIndex++;
    return true;
}

bool CameraAttachLoopMidAsmHook()
{
    if (!Config::ExtendedMultiplayer)
        return false;
    if (s_cameraAttachPlayerIndex <= g_multiplayerPlayerCount - 1)
        return true;
    s_cameraAttachPlayerIndex = 0;
    return false;
}

bool PlayerPostManagerLoopMidAsmHook(X86Register& eax)
{
    if (Config::ExtendedMultiplayer)
        return eax.u32 < g_multiplayerPlayerCount;
    return eax.u32 < 2;
}

bool ItemIconManagerLoopPushCountMidAsmHook(X86Register& esp)
{
    auto ctx = GetX86Context();
    auto base = g_memory.base;
    if (Config::ExtendedMultiplayer)
    {
        esp.u32 -= 4;
        X86_STORE_U32(esp.u32, g_multiplayerPlayerCount);
        return true;
    }
    return false;
}

bool ItemIconManagerLoopCountMidAsmHook(X86Register& eax)
{
    if (Config::ExtendedMultiplayer)
        return eax.u32 < g_multiplayerPlayerCount;
    return eax.u32 < 2;
}

bool SplitscreenTypeInitializeUnpauseMidAsmHook(X86Register& eax)
{
    if (Config::ExtendedMultiplayer)
    {
        return true;
    }
    return (eax.u8_lo & 0x2) == 0;
}

void AddToScoreNullMidAsmHook() {}

// END Extended Multiplayer

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