#include <gpu/video.h>
#include <kernel/function.h>
#include <user/config.h>

extern bool s_ShouldPatchSubtitles;
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
	"gamemenu",
	"001_command_icons",
	"pause_main",
	"000_squarebuttons",
	"000_VHArrow_xbox",
};

bool foundTex = false;
//RwSetTextureName debugging left over, can be useful to track a texture pointer and find where it's used
X86_FUNC_IMPL(__imp__sub_2D82E0);
X86_FUNC(sub_2D82E0)
{
	char* str = (char*)GetHostParameter(2, CallConvention::Cdecl, ParameterType::Pointer);
	std::string s = str;
	for (int i = 0; i < 5; i++)
	{
		if (s.find(texNameArray[i]) != std::string::npos)
		{
			printf("tex %s ", texNameArray[i].c_str());
			foundTex = true;
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

// MessageTaskExec
X86_FUNC_IMPL(__imp__sub_59430);
X86_FUNC(sub_59430)
{
	s_ShouldPatchSubtitles = Config::UIAlignmentMode.Value == EUIAlignmentMode::Centre;
	__imp__sub_59430(ctx, base);
	s_ShouldPatchSubtitles = false;
}

void DisableMessageMidAsmHook()
{
	shouldSkipMessage = Config::Subtitles.Value == 0;
}