#include <gpu/video.h>
#include <kernel/function.h>
#include <user/config.h>
#include <patches/global.h>


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

void SelectModeUnlockCheckBypassMidAsmHook(X86Register& eax)
{
	if (Config::AllStagesUnlocked)
	{
		eax.u32 = 1;
	}
}

bool EasierUnlockLastStoryMidAsmHook(X86Register & edi)
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
