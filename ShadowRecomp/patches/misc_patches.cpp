#include <gpu/video.h>
#include <kernel/function.h>
#include <user/config.h>
#include <patches/global.h>
#include <input/padstate.h>
#include "pause_patches.h"


// Patch out logs Artificial Chaos can spew (causes crashes)
X86_FUNC(sub_37EBE6)
{
	ctx.esp.u32 += 4;
	return;
}

//leftover debugprint functions be gone!
X86_FUNC(sub_3DA150)
{
	ctx.esp.u32 += 4;
	return;
}

X86_FUNC(sub_41D78A)
{
	ctx.esp.u32 += 4;
	return;
}

X86_FUNC(sub_41A46B)
{
	ctx.esp.u32 += 4;
	return;
}


X86_FUNC(sub_14710)
{
	ctx.esp.u32 += 4;
	return;
}

X86_FUNC(sub_40634F)
{
	ctx.esp.u32 += 4;
	return;
}

//this one is a hard lock, we will let it run
X86_FUNC(sub_2B1A00)
{
	printf("Not inititalized m_eDealloctor\n");
	while (1)
		;
}

X86_FUNC(sub_41C0D5)
{
	ctx.esp.u32 += 4;
	return;
}

X86_FUNC(sub_41CD65)
{
	ctx.esp.u32 += 4;
	return;
}

bool SkipPartnerIntroMidAsmHook()
{
	return !Config::EnablePartnerIntro;
}

// All Stages Unlocked cheat
// GetStageId(row, column, onlyUnlocked)
X86_FUNC_IMPL(__imp__sub_28D950);
X86_FUNC(sub_28D950)
{
	if (Config::AllStagesUnlocked)
		X86_STORE_U8(ctx.esp.u32 + 0xC, 0);

	__imp__sub_28D950(ctx, base);
}

// GetStageRouteId(row, column, onlyUnlocked)
X86_FUNC_IMPL(__imp__sub_28D9E0);
X86_FUNC(sub_28D9E0)
{
	if (Config::AllStagesUnlocked)
		X86_STORE_U8(ctx.esp.u32 + 0xC, 0);

	__imp__sub_28D9E0(ctx, base);
}

// GetBossStageId(index, onlyUnlocked)
X86_FUNC_IMPL(__imp__sub_28DA70);
X86_FUNC(sub_28DA70)
{
	if (Config::AllStagesUnlocked)
		X86_STORE_U8(ctx.esp.u32 + 0x8, 0);

	__imp__sub_28DA70(ctx, base);
}

// trick the game that we already visited each stage so the stage thumbnail show up on the menu
X86_FUNC_IMPL(__imp__sub_2A0790);
X86_FUNC(sub_2A0790)
{
	__imp__sub_2A0790(ctx, base);

	if (Config::AllStagesUnlocked)
		ctx.eax.u32 = 1;
}

// trick the game that stage is already unlocked 
bool SelectModeUnlockCheckBypassMidAsmHook()
{
	return Config::AllStagesUnlocked;
}

bool EasierUnlockLastStoryMidAsmHook(X86Register& edi)
{
	if (Config::EasyLastStoryUnlock)
	{
		if (edi.u32 >= 5)
		{
			return true;
		}
	}

	return false;
}

//can always skip cutscenes
X86_FUNC_IMPL(__imp__sub_226B00);
X86_FUNC(sub_226B00)
{
	auto thisAddr = ctx.ecx.u32; //thiscall always store first parameter in ecx
	__imp__sub_226B00(ctx, base);
	X86_STORE_U8(thisAddr + 8, 0x0);
}

// same as above
X86_FUNC_IMPL(__imp__sub_1715B0);
X86_FUNC(sub_1715B0)
{
	auto thisAddr = ctx.ecx.u32;
	__imp__sub_1715B0(ctx, base);
	X86_STORE_U8(thisAddr + 24, 0x0);
}

// For testing 2P mod or performance testing; Force TBlock to always load every chunk
bool loadAllChunks(float* a1, float* a2, float* a3)
{
	return 1;
}
// Uncomment below if wanting to test with all chunks loaded
//GUEST_FUNCTION_HOOK_STDCALL(sub_12700, loadAllChunks);


// Level Geo Unrestrict (For Custom Stages)
// Note: This will break some stages, e.g. Egg Dealer (Black Comet) and if you die in a certain area in GUN Fortress.
// This is because Sonic Team left visibility chunks pointing to old files. These are usually ignored due to this file restriction function.
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
// TODO: This corrupts memory / needs to be reworked; Was good enough for a POC
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

// Input locking PAD2 for forcing 2P and Enabling Partner Control
X86_FUNC_IMPL(__imp__sub_28AF30);
X86_FUNC(sub_28AF30)
{
	auto thisAddr = ctx.ecx.u32;
	__imp__sub_28AF30(ctx, base);
	X86_STORE_U32(0x5356E8, 1); // PAD2 locked to XInput PORT 2
}

std::string texNameArray[] =
{
	"E630A_c1",
	"memory01",
	"memory02",
	"memory03",
	"memory04",
	"memory05",
	"memory06",
	"memory07",
};

bool foundTex = false;
//RwSetTextureName debugging left over, can be useful to track a texture pointer and find where it's used
X86_FUNC_IMPL(__imp__sub_2D82E0);
X86_FUNC(sub_2D82E0)
{
	char* str = (char*)GetHostParameter(2, CallConvention::Cdecl, ParameterType::Pointer);
	std::string s = str;

	for (int i = 0; i < 8; i++)
	{
		if (s.find(texNameArray[i]) != std::string::npos)
		{
			foundTex = true;
			break;
		}
	}

	__imp__sub_2D82E0(ctx, base);
}

static bool shouldSkipMessage = false;
X86_FUNC_IMPL(__imp__sub_1E5A0);
X86_FUNC(sub_1E5A0)
{
	if (shouldSkipMessage)
	{
		ctx.esp.u32 += 12 + 4;
		shouldSkipMessage = false;
		return;
	}

	__imp__sub_1E5A0(ctx, base);
}


void DisableMessageMidAsmHook()
{
	shouldSkipMessage = Config::Subtitles.Value == 0;
}

// Fix the lock-on weapon laser draw hiding dropshadows
// KageTask (draws the SDW_ALL)
// Patch by forcing the render state to draw
X86_FUNC_IMPL(__imp__sub_BF050);
X86_FUNC(sub_BF050)
{
	uint32_t ecx = ctx.ecx.u32;
	uint32_t setRenderState = X86_LOAD_U32(0x57DE20);
	// KageTask just inherits last state, so when the laser draws the alpha applies to SDW_ALL too
	if (setRenderState != 0)
	{
		struct { uint32_t state, value; } states[] =
		{
			{ 0x9, 2 },
			{ 0xA, 5 },
			{ 0xB, 6 },
			{ 0xC, 0 },
		}; // Thank you Donut (RWSTATE)

		for (auto& s : states)
		{
			ctx.esp.u32 -= 8;
			X86_STORE_U32(ctx.esp.u32 + 0, s.state);
			X86_STORE_U32(ctx.esp.u32 + 4, s.value);
			ctx.esp.u32 -= 4;
			X86_CALL_FUNC(g_memory.FindFunction(setRenderState));
			ctx.esp.u32 += 8;
		}
	}
	ctx.ecx.u32 = ecx;
	__imp__sub_BF050(ctx, base);
}

// helper function to find the current asset loaded from a .one file.
char* TOneFileCheckFileName(TOneFile* this_, int32_t index)
{
	int v2 = this_->assetStreamData;
	char* result; // eax

	int resV2 = 0;

	if (v2)
	{
		auto base = g_memory.base;
		resV2 = X86_LOAD_U32(v2 + 16);
	}


	if ((this_->field_2C || this_->fname[0]) && index >= 2 && (index < resV2 + 2))
		result = (char*)g_memory.Translate(56 * index + v2 + 52);
	else
		result = 0;

	return result;
}

// Expert Select (port of Reloaded v1.2 version)
// TODO: Do we want this in base, or as Reloaded only?
// TODO: Note we should probably switch to Zetti's version before release
// This one can apply to Story Mode too
void ExpertSelectMidAsmHook(X86Register& edx)
{
	if (PJSPeripheral::IsHeld(0, PJSPeripheral::BTN_X))
	{
		auto base = g_memory.base;
		X86_STORE_U8(edx.u32 + 0x59, 1);
	}
}

// Fix EggPierrot wrong asset
// sub_142ED0 picks which .ONE to load
// Intercept and check for the typo (501 -> 511)
bool EggPierrotStageIdFixMidAsmHook(X86Register& eax)
{
	return eax.u32 == 511;
}

// Fix the Xbox exclusive 'more transparent/alpha' subtitle bug
// sub_28D1E0 called by the SaveMessage is reused for the in-game Restart/Quit prompts
// GC/PS2 don't have those prompts, so they never have the bug.
// The last drawn element's alpha gets applied to the main brush as a result
static void ResetMessageBrushColor()
{
	auto base = g_memory.base;
	auto messageManager = X86_LOAD_U32(0x544F3C); // MessageManager
	if (messageManager == 0)
	{
		return;
	}

	auto messageDisp = X86_LOAD_U32(messageManager + 0x10); // TMessageDisp
	if (messageDisp == 0)
	{
		return;
	}

	auto brush = X86_LOAD_U32(messageDisp + 0x58); // Color
	if (brush == 0)
	{
		return;
	}

	// Match default subtitle color 0xFEFEFEFE from TMessageDisp::InitBrush
	for (auto corner : { brush + 0x00, brush + 0x30 })
	{
		X86_STORE_F32(corner + 0x0, 254.0f);
		X86_STORE_F32(corner + 0x4, 254.0f);
		X86_STORE_F32(corner + 0x8, 254.0f);
		X86_STORE_F32(corner + 0xC, 254.0f);
	}
	for (auto delta : { brush + 0x18, brush + 0x48 })
	{
		X86_STORE_F32(delta + 0x0, 0.0f);
		X86_STORE_F32(delta + 0x4, 0.0f);
		X86_STORE_F32(delta + 0x8, 0.0f);
		X86_STORE_F32(delta + 0xC, 0.0f);
	}
	X86_STORE_U32(brush + 0x60, 0xFEFEFEFE);
	X86_STORE_U32(brush + 0x64, X86_LOAD_U32(brush + 0x64) & ~1u);
}

// MessageTaskExec
X86_FUNC_IMPL(__imp__sub_59430);
X86_FUNC(sub_59430)
{
	ResetMessageBrushColor();
	ToggleMessageTaskDraw(true);
	__imp__sub_59430(ctx, base);
	ToggleMessageTaskDraw(false);
}

// ElecCoreProgram::Exec
// Fix the mission counter increment being skipped
// 0x44 = destroyed
// 0x2E4 = hp (total HP @ 0x46B86C)
X86_FUNC_IMPL(__imp__sub_1F89B0);
X86_FUNC(sub_1F89B0)
{
	auto elecCore = ctx.ecx.u32;
	if (X86_LOAD_U32(elecCore + 0x44) == 1 && X86_LOAD_F32(elecCore + 0x2E4) >= X86_LOAD_F32(0x46B86C))
	{
		auto missionMgr = X86_LOAD_U32(0x541FA4); // MissionManagerTask
		if (missionMgr != 0)
		{
			auto mission = X86_LOAD_U32(missionMgr + 8);
			auto handle = X86_LOAD_U32(missionMgr + 0xC);
			if (mission != 0 && handle != 0 && X86_LOAD_U32(mission + 0xC) != 1) // already completed check
			{
				// Set the counter BEFORE clearing the mission (OG has this too, but its in the wrong branch)
				X86_STORE_U8(mission + 0x14, 1);
				auto vtable = X86_LOAD_U32(mission);
				auto addCount = X86_LOAD_U32(vtable + 0x1C); // CountMission::AddCount
				ctx.esp.u32 -= 8;
				X86_STORE_U32(ctx.esp.u32 + 0, 1); // count
				X86_STORE_U32(ctx.esp.u32 + 4, 0); // affinity (dark)
				ctx.ecx.u32 = mission;
				ctx.esp.u32 -= 4;
				X86_CALL_INDIRECT_FUNC(addCount);
			}
		}
	}
	ctx.ecx.u32 = elecCore;
	__imp__sub_1F89B0(ctx, base);
}
// Fix the missing ScoreCommand if vacuum picks up fire (branch is skipped since owner check fails)
// Fire VacuumCommand handler @ 0x1AA820
// Fire attack callback @ 0x19B550
X86_FUNC_IMPL(__imp__sub_5C4D0); // ScoreCommand
void FireVacuumCallbackMidAsmHook(X86Register& ebp, X86Register& edi)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;

	// ebp = GadgetCharInfo (object being sucked)
	// ebp+0x50 = GadgetID, 0x1A = Fire
	// ebp+0x64 = CharCommandUnit, handler instance is 0x8 inside CharCommandUnit
	// edi = Vacuum weapon instance (0xC = player who is holding it)
	if (ebp.u32 == 0 || X86_LOAD_U32(ebp.u32 + 0x50) != 0x1A)
	{
		// skip if not fire
		return;
	}

	auto cmdUnit = X86_LOAD_U32(ebp.u32 + 0x64);
	if (cmdUnit == 0) // command null check (matching OG)
	{
		return;
	}

	auto node = X86_LOAD_U32(cmdUnit + 8);
	if (node == 0 || X86_LOAD_U32(node) != 0x1AA820) { // check for Fire/PoisonGas handler
		return;
	}

	// 0x466F6C = Fire & PoisonGas vtable (shared, but PoisonGas has no score attribute)
	// fire+0x50 = state
	auto fire = X86_LOAD_U32(node + 4);
	if (X86_LOAD_U32(fire) != 0x466F6C || X86_LOAD_U32(fire + 0x50) != 1)
	{
		return;
	}

	// safety check if the vac command drops
	if (X86_LOAD_U32(edi.u32 + 0xC) == 0)
	{
		return;
	}

	ctx->esp.u32 -= 4;
	X86_STORE_U32(ctx->esp.u32, 0x1B); // score callback entry
	ctx->esp.u32 -= 4;
	X86_STORE_U32(ctx->esp.u32, edi.u32 + 0xC); // player instance
	ctx->esp.u32 -= 4;
	__imp__sub_5C4D0(*ctx, base);
	ctx->esp.u32 += 8;
}

// Katana Bullet frame update step (hooked func)
// sub_27C2A0 is called right after with the collision check
// Widen the collision sweep to match the visual effect
// Bullet @ esi
// BulletType @ esi+0x4C
// Type 5 is Katana01, Type 6 is Katana02
void KatanaBulletSweepMidAsmHook(X86Register& esi)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;

	auto bulletType = X86_LOAD_U32(esi.u32 + 0x4C);
	if (bulletType != 5 && bulletType != 6)
	{
		return;
	}

	// out buffer @ esp
	// old pos @ esp+0x8
	// next frame pos @ esp+0xC
	auto outPtr = X86_LOAD_U32(ctx->esp.u32);
	auto posA = X86_LOAD_U32(ctx->esp.u32 + 0x08);
	auto posB = X86_LOAD_U32(ctx->esp.u32 + 0x0C);

	float ax = X86_LOAD_F32(posA + 0x0);
	float az = X86_LOAD_F32(posA + 0x8);
	float bx = X86_LOAD_F32(posB + 0x0);
	float by = X86_LOAD_F32(posB + 0x4);
	float bz = X86_LOAD_F32(posB + 0x8);

	// sideways direction of the wave travel (ignoring y)
	float px = -(bz - az);
	float pz = (bx - ax);
	float len = sqrtf(px * px + pz * pz);
	if (len < 1e-4f)
	{
		px = 1.0f;
		pz = 0.0f;
	}
	else
	{
		px /= len;
		pz /= len;
	}

	// katana level 1 = 9 (18 full width)
	// katana level 2 = 22 (44 full width)
	float halfWidth = bulletType == 6 ? 22.0f : 9.0f;

	// update the expanded range in out buffer
	X86_STORE_F32(outPtr + 0x00, ax - px * halfWidth);
	X86_STORE_F32(outPtr + 0x04, X86_LOAD_F32(posA + 0x4));
	X86_STORE_F32(outPtr + 0x08, az - pz * halfWidth);
	X86_STORE_F32(outPtr + 0x18, bx + px * halfWidth);
	X86_STORE_F32(outPtr + 0x1C, by);
	X86_STORE_F32(outPtr + 0x20, bz + pz * halfWidth);

	// update sweep with new size
	X86_STORE_U32(ctx->esp.u32 + 0x08, outPtr);
	X86_STORE_U32(ctx->esp.u32 + 0x0C, outPtr + 0x18);
}

// Remove the shared 'clear bit' for egg breaker breaking the glow effect
void EggBreakerDigitizeEffectMidAsmHook() {}


// Fix Mipmapped textures breaking on 3D models
// After a level TXD loads, the game sets max anisotropy 4 on every tex with mip levels.
// However, the anisot plugin was never attached on the Xbox version so nothing work and the game end binding a null texture. 
// We can patch this by dropping the write and let the host handle the rest.
X86_FUNC(sub_309F90)
{
	ctx.eax.u32 = X86_LOAD_U32(ctx.esp.u32 + 4);
	ctx.AddEsp(1);
}


// EffectDrawGlobalLensFlareTask ctor never frees the data at +0x34,
// therefore, if the task dies early, the destructor will use garbage data. 
// Clear it after the ctor to be safe.

X86_FUNC_IMPL(__imp__sub_C0DC0);
X86_FUNC(sub_C0DC0)
{
	const uint32_t task = ctx.ecx.u32;

	__imp__sub_C0DC0(ctx, base);

	X86_STORE_U32(task + 0x34, 0);
}
