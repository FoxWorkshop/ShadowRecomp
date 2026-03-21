#include <gpu/video.h>
#include <kernel/function.h>





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



//todo implement as an option
void SkipPartnerIntroMidAsmHook(X86Register& eax)
{
	eax.u32 = 1;
}

// TODO: This is temporary, replace with a proper SaveData patch
void SelectModeUnlockCheckBypassMidAsmHook(X86Register& eax)
{
	eax.u32 = 1;
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