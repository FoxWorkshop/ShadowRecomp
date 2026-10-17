#include <gpu/video.h>
#include <kernel/function.h>
#include <user/config.h>
#include <patches/global.h>
#include "pause_patches.h"


// Patch out logs Artificial Chaos can spew (causes crashes)
X86_FUNC(sub_37EBE6)
{
	ctx.esp.u32 += 4;
}

//leftover debugprint functions be gone!
X86_FUNC(sub_3DA150)
{
	ctx.esp.u32 += 4;
}

X86_FUNC(sub_41D78A)
{
	ctx.esp.u32 += 4;
}

X86_FUNC(sub_41A46B)
{
	ctx.esp.u32 += 4;
}

X86_FUNC(sub_14710)
{
	ctx.esp.u32 += 4;
}

X86_FUNC(sub_40634F)
{
	ctx.esp.u32 += 4;
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
}

X86_FUNC(sub_41CD65)
{
	ctx.esp.u32 += 4;
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

//RwSetTextureName debugging left over, can be useful to track a texture pointer and find where it's used
bool foundTex = false;
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

// Egg Dealer roulette UI white flash fix
// This happens on GC too, rarely the intro cutscene picks sprite 6 (there are only 5)
X86_FUNC_IMPL(__imp__sub_114650);
X86_FUNC(sub_114650)
{
	const uint32_t angleAddr = ctx.esp.u32 + 0x8;
	const float scale = X86_LOAD_F32(0x4F96D0);
	const float slotFrames = X86_LOAD_F32(0x4F9988) * scale;
	const float frame = X86_LOAD_F32(angleAddr) * scale;
	const float slotFrame = std::round(frame / slotFrames) * slotFrames;

	// float precision moment. Cheating by skipping an extra 0.01f
	if (slotFrame - frame >= 0.0f && slotFrame - frame < 0.05f)
	{
		X86_STORE_F32(angleAddr, (slotFrame + 0.01f) / scale);
	}
	__imp__sub_114650(ctx, base);
}

// START PAD assignment patches
// Force PAD1/PAD2 to always be PORT1 and PORT2, no matter what.
// Also fixes Partner Control since Xbox resets PAD1 to unmapped on stage start normally
// 'Any controller' is PORT value of 5
// 0x5356E4 = PAD1
// 0x5356E8 = PAD2

// PAD constructor (game init)
void PADManagerInitMidAsmHook()
{
	auto base = g_memory.base;
	X86_STORE_U32(0x5356E4, 0);
	X86_STORE_U32(0x5356E8, 1);
}

// Advertise (first blue message on main menu)
void PADAdvertiseMidAsmHook()
{
	auto base = g_memory.base;
	X86_STORE_U32(0x5356E4, 0);
	X86_STORE_U32(0x5356E8, 1);
}

// BATTLE MODE char select (If P2 backs out it sets P1 back to 5)
void PADBattleModeP2BackOutMidAsmHook()
{
	auto base = g_memory.base;
	X86_STORE_U32(0x5356E4, 0);
}

// BATTLE MODE stage select (sets both if either player backs out)
void PADBattleStageSelMidAsmHook()
{
	auto base = g_memory.base;
	X86_STORE_U32(0x5356E4, 0);
	X86_STORE_U32(0x5356E8, 1);
}
// END PAD assignment patches

// .fog load (called by the TFogSet ctor)
// fog data @ 0x54975C
// fog exec @ sub_2265C0
X86_FUNC_IMPL(__imp__sub_226370);
X86_FUNC(sub_226370)
{
	if (!Config::Fog.Value)
	{
		ctx.esp.u32 += 4;
		return;
	}
	__imp__sub_226370(ctx, base);
}

// Originally on GameCube, the synopsis shows a skip button at the bottom right of the screen.
// For some reason a flag to hide it was enabled on the Xbox version
// We can disable the flag to restore it.
void UnhideSynopsisSkipPrompt(uint32_t cast, const std::string& path)
{
	auto lower = path;
	std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return (char)std::tolower(c); });

	// If more stuff get find out, a full array could be made, but good enough for now.
	if (lower != "csdfiles/synopsis.xncp/button/skip_btn" && lower != "csdfiles/synopsis.xncp/button/skip_btn/skip_txt")
		return;

	auto base = g_memory.base;
	auto castInfo = X86_LOAD_U32(cast + 0x30);

	if (castInfo)
	{
		X86_STORE_U32(castInfo, 0x0); // disable hidden flag
	}
}
