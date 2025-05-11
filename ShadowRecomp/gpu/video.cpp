#include <kernel/function.h>
#include "x86_context.h"


//todo


static void SetVertexShader(int a1)
{

}


static void SetPixelShader(int a1)
{

}

//Replace the fake modes[] array with real values from SDL2, DX12, etc.).
//If we decode sub_3D68A5(), make sure to match its format ID to whatever format we need
//CMiniport_GetDisplayCapabilities() should be stub 

void D3D_EnumAdapterModes(int adapter, unsigned int a2, uint32_t* a3) 
{
    if (!a3)
        return;

    // Simulate format flags
    uint32_t formatFlag;
    switch (a2 & 3) {
    case 0: formatFlag = 30; break;
    case 1: formatFlag = 17; break;
    case 2: formatFlag = 28; break;
    case 3: formatFlag = 18; break;
    }

    // Fake mode table
    struct FakeMode {
        uint16_t width;
        uint16_t height;
        uint32_t fmt; // return value of sub_3D68A5
    };

    static FakeMode modes[] = {
        { 640, 480, 0x15 },   // 0x15 = D3DFMT_X8R8G8B8 maybe
        { 800, 600, 0x15 },
        { 1280, 720, 0x15 },
    };

    unsigned int modeIndex = a2 >> 2;
    if (modeIndex >= std::size(modes))
        return;

    const FakeMode& mode = modes[modeIndex];
    a3[0] = mode.width;
    a3[1] = mode.height;
    a3[2] = 60;           // refresh rate
    a3[3] = 0x50;     // pixel format
    a3[4] = formatFlag;   // format flag based on a2 & 3
}

int D3D_GetAdapterModeCount(int a1)
{
	printf("STUB D3D_GetAdapterModeCount\n");
	return 4 * 1;
}

//force XBOXDeviceSystemOpen to work due to a weird adapter mode check that fail, might be able to stub it entirely?
X86_FUNC(sub_2DD620)
{
	// sub esp, 0x18
	auto temp_2DD620_U32 = ctx.esp.u32;
	ctx.esp.u32 -= 0x18u;
	ctx.eflags.update_flags_sub(temp_2DD620_U32, 0x18u, ctx.esp.u32);
	// push ebx
	X86_PUSH(ctx, base, ctx.ebx.u32);
	// mov ebx, [eax]
	ctx.ebx.u32 = X86_LOAD_U32(ctx.eax.u32);
	// push edi
	X86_PUSH(ctx, base, ctx.edi.u32);
	// xor eax, eax
	ctx.eax.u32 ^= ctx.eax.u32;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
	// mov ecx, 0x27
	ctx.ecx.u32 = 0x27u;
	// mov edi, 0x57DD40
	ctx.edi.u32 = 0x57DD40u;
	// push eax
	X86_PUSH(ctx, base, ctx.eax.u32);
	// rep stosd
	if (ctx.eflags.value & X86EFlags::DF) {
		for (uint32_t i = 0; i < ctx.ecx.u32; ++i) {
			X86_STORE_U32(static_cast<uintptr_t>(ctx.edi.u32) - i * 4, ctx.eax.u32);
		}
		ctx.edi.u32 -= ctx.ecx.u32 * 4;
	}
	else {
		for (uint32_t i = 0; i < ctx.ecx.u32; ++i) {
			X86_STORE_U32(static_cast<uintptr_t>(ctx.edi.u32) + i * 4, ctx.eax.u32);
		}
		ctx.edi.u32 += ctx.ecx.u32 * 4;
	}
	ctx.ecx.u32 = 0;
	// call 0x003DD8A0
	ctx.esp.u32 -= 0x4; //Adjust stack pointer
	sub_3DD8A0(ctx, base);
	// test eax, eax
	ctx.eflags.update_test_flags(ctx.eax.u32 & ctx.eax.u32);
	// mov [0x0057DD40], eax
	X86_STORE_U32(5758272, ctx.eax.u32);
	// jnz 0x002DD64A
	if ((ctx.eflags.value & X86EFlags::ZF) == 0) goto loc_2DD64A;
	// pop edi
	ctx.edi.u32 = X86_POP(ctx, base);
	// pop ebx
	ctx.ebx.u32 = X86_POP(ctx, base);
	// add esp, 0x18
	auto temp_2DD646_U32 = ctx.esp.u32;
	ctx.esp.u32 += 0x18u;
	ctx.eflags.update_flags_add(temp_2DD646_U32, 0x18u, ctx.esp.u32);
	// ret
	ctx.esp.u32 += 0x4;  //Adjust stack pointer
	return;
loc_2DD64A:
	// push esi
	X86_PUSH(ctx, base, ctx.esi.u32);
	// push 0x47D1CC
	X86_PUSH(ctx, base, 0x47D1CC);
	// push 0x57DD8C
	X86_PUSH(ctx, base, 0x57DD8C);
	// call [0x0057DECC]
	ctx.esp.u32 -= 0x4; //Adjust stack pointer
	X86_CALL_INDIRECT_FUNC(*(uint32_t*)(base + 5758668));
	// add esp, 0x08
	auto temp_2DD65B_U32 = ctx.esp.u32;
	ctx.esp.u32 += 0x8u;
	ctx.eflags.update_flags_add(temp_2DD65B_U32, 0x8u, ctx.esp.u32);
	// push 0x00
	X86_PUSH(ctx, base, 0x0);
	// call 0x003DD8B0
	ctx.esp.u32 -= 0x4; //Adjust stack pointer
	sub_3DD8B0(ctx, base);
	// test ebx, ebx
	ctx.eflags.update_test_flags(ctx.ebx.u32 & ctx.ebx.u32);
	// mov [0x0057DD78], eax
	X86_STORE_U32(5758328, ctx.eax.u32);
	// mov dword ptr [0x0057DD84], 0x00
	X86_STORE_U32(5758340, 0x0u);
	// jz 0x002DD738
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_2DD738;
	// mov ecx, 0x0A
	ctx.ecx.u32 = 0xAu;
	// mov esi, ebx
	ctx.esi.u32 = ctx.ebx.u32;
	// mov edi, 0x57DD50
	ctx.edi.u32 = 0x57DD50u;
	// rep movsd
	for (; ctx.ecx.u32 > 0; ctx.ecx.u32--, ctx.esi.u32 += 4, ctx.edi.u32 += 4) {
		X86_STORE_U32(ctx.edi.u32, X86_LOAD_U32(ctx.esi.u32));
	}
	// mov eax, [0x0057DD78]
	ctx.eax.u32 = X86_LOAD_U32(5758328);
	// xor esi, esi
	ctx.esi.u32 ^= ctx.esi.u32;
	ctx.eflags.update_flags_logic(ctx.esi.u32);
	// test eax, eax
	ctx.eflags.update_test_flags(ctx.eax.u32 & ctx.eax.u32);
	// jle 0x002DD725
	if (((ctx.eflags.value & X86EFlags::ZF) != 0) || (((ctx.eflags.value & X86EFlags::SF) != 0) != ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_2DD725;
	// lea esp, [esp]
	ctx.esp.u32 = ctx.esp.u32;
loc_2DD6A0:
	// lea ecx, [esp+0x10]
	ctx.ecx.u32 = ctx.esp.u32 + 16;
	// push ecx
	X86_PUSH(ctx, base, ctx.ecx.u32);
	// push esi
	X86_PUSH(ctx, base, ctx.esi.u32);
	// push 0x00
	X86_PUSH(ctx, base, 0x0);
	// call 0x003DD950
	ctx.esp.u32 -= 0x4; //Adjust stack pointer
	sub_3DD950(ctx, base);
	// mov edx, [esp+0x10]
	ctx.edx.u32 = X86_LOAD_U32(ctx.esp.u32 + 16);
	// cmp edx, [ebx]
	ctx.eflags.update_compare_flags_U32(ctx.edx.u32, X86_LOAD_U32(ctx.ebx.u32));
	// jnz 0x002DD6FA
	// mov eax, [esp+0x14]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esp.u32 + 20);
	// cmp eax, [ebx+0x04]
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, X86_LOAD_U32(ctx.ebx.u32 + 4));
	// jnz 0x002DD6FA
	// mov eax, [ebx+0x08]
	ctx.eax.u32 = X86_LOAD_U32(ctx.ebx.u32 + 8);
	// call 0x002DD550
	ctx.esp.u32 -= 0x4; //Adjust stack pointer
	sub_2DD550(ctx, base);
	// mov ecx, eax
	ctx.ecx.u32 = ctx.eax.u32;
	// mov eax, [esp+0x20]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esp.u32 + 32);
	// call 0x002DD550
	ctx.esp.u32 -= 0x4; //Adjust stack pointer
	sub_2DD550(ctx, base);
	// cmp eax, ecx
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, ctx.ecx.u32);
	// mov ecx, [ebx+0x1C]
loc_2DD717:
	// cmp esi, [0x0057DD78]
	ctx.eflags.update_compare_flags_U32(ctx.esi.u32, X86_LOAD_U32(5758328));
	// mov [0x0057DD84], esi
	X86_STORE_U32(5758340, ctx.esi.u32);
	// jl 0x002DD745
	if ((((ctx.eflags.value & X86EFlags::SF) != 0) != ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_2DD745;
loc_2DD725:
	// pop esi
	ctx.esi.u32 = X86_POP(ctx, base);
	// pop edi
	ctx.edi.u32 = X86_POP(ctx, base);
	// mov dword ptr [0x0057DD40], 0x00
	X86_STORE_U32(5758272, 0x0u);
	// xor eax, eax
	ctx.eax.u32 ^= ctx.eax.u32;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
	// pop ebx
	ctx.ebx.u32 = X86_POP(ctx, base);
	// add esp, 0x18
	auto temp_2DD734_U32 = ctx.esp.u32;
	ctx.esp.u32 += 0x18u;
	ctx.eflags.update_flags_add(temp_2DD734_U32, 0x18u, ctx.esp.u32);
	// ret
	ctx.esp.u32 += 0x4;  //Adjust stack pointer
	return;
loc_2DD738:
	// push 0x57DD50
	X86_PUSH(ctx, base, 0x57DD50);
	// call 0x002DD5C0
	ctx.esp.u32 -= 0x4; //Adjust stack pointer
	sub_2DD5C0(ctx, base);
	// add esp, 0x04
	auto temp_2DD742_U32 = ctx.esp.u32;
	ctx.esp.u32 += 0x4u;
	ctx.eflags.update_flags_add(temp_2DD742_U32, 0x4u, ctx.esp.u32);
loc_2DD745:
	// mov eax, [0x0057DD58]
	ctx.eax.u32 = X86_LOAD_U32(5758296);
	// call 0x002DD500
	ctx.esp.u32 -= 0x4; //Adjust stack pointer
	sub_2DD500(ctx, base);
	// mov [0x0057DD7C], eax
	X86_STORE_U32(5758332, ctx.eax.u32);
	// mov eax, [0x0057DD68]
	ctx.eax.u32 = X86_LOAD_U32(5758312);
	// cmp eax, 0x2A
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, 0x2Au);
	// jz 0x002DD777
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_2DD777;
	// cmp eax, 0x2B
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, 0x2Bu);
	// jz 0x002DD777
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_2DD777;
	// cmp eax, 0x2E
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, 0x2Eu);
	// jz 0x002DD777
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_2DD777;
	// cmp eax, 0x2F
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, 0x2Fu);
	// mov dword ptr [0x0057DD88], 0x10
	X86_STORE_U32(5758344, 0x10u);
	// jnz 0x002DD781
	if ((ctx.eflags.value & X86EFlags::ZF) == 0) goto loc_2DD781;
loc_2DD777:
	// mov dword ptr [0x0057DD88], 0x20
	X86_STORE_U32(5758344, 0x20u);
loc_2DD781:
	// stmxcsr [esp+0x0C]
	//MISSING INSTRUCTION HERE TODO ADD
	// mov eax, [esp+0x0C]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esp.u32 + 12);
	// or eax, 0x8000
	ctx.eax.u32 |= 0x8000;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
	// pop esi
	ctx.esi.u32 = X86_POP(ctx, base);
	// mov [esp+0x08], eax
	X86_STORE_U32(ctx.esp.u32 + 8, ctx.eax.u32);
	// ldmxcsr [esp+0x08]
	//MISSING INSTRUCTION HERE TODO ADD
	// pop edi
	ctx.edi.u32 = X86_POP(ctx, base);
	// mov eax, 0x01
	ctx.eax.u32 = 0x1u;
	// pop ebx
	ctx.ebx.u32 = X86_POP(ctx, base);
	// add esp, 0x18
	auto temp_2DD7A0_U32 = ctx.esp.u32;
	ctx.esp.u32 += 0x18u;
	ctx.eflags.update_flags_add(temp_2DD7A0_U32, 0x18u, ctx.esp.u32);
	// ret
	ctx.esp.u32 += 0x4;  //Adjust stack pointer
	return;
}

uint8_t g_DummyDeviceMemory[0x24A0] = {};
uint8_t* D3D_g_pDevice = nullptr;

int Direct3D_CreateDevice_r(int a1, int a2, int a3, char a4, uint32_t a5, uint32_t* a6)
{
	printf("\nCREATE D3D DEVICE!\n");
	auto ctx = GetX86Context();
	auto base = g_memory.base;

	// mov eax, [0x003E31B4]
	ctx->eax.u32 = X86_LOAD_U32(4075956);
	// test eax, eax
	ctx->eflags.update_test_flags(ctx->eax.u32 & ctx->eax.u32);

	if ((ctx->eflags.value & X86EFlags::ZF) == 0)
		goto loc_3DDAC3;
	// mov dword ptr [0x003E31B4], 0x80000
	X86_STORE_U32(4075956, 0x80000u);
loc_3DDAC3:
	// mov eax, [0x003E31B0]
	ctx->eax.u32 = X86_LOAD_U32(4075952);
	// test eax, eax
	ctx->eflags.update_test_flags(ctx->eax.u32 & ctx->eax.u32);
	// jnz 0x003DDAD6
	if ((ctx->eflags.value & X86EFlags::ZF) != 0)
		X86_STORE_U32(4075952, 0x8000u);


	D3D_g_pDevice = (uint8_t*)g_userHeap.AllocPhysical(0x24A0, 4096);
	//*(uint32_t*)(D3D_g_pDevice + 0x1E5) = g_memory.MapVirtual(g_userHeap.AllocPhysical(0x4000, 4096));
	//*(uint32_t*)(D3D_g_pDevice + 0x1E5 + 4) = g_memory.MapVirtual(g_userHeap.AllocPhysical(0x4000, 4096));
	auto content = g_memory.MapVirtual(D3D_g_pDevice);
	X86_STORE_U32(0x3E0A18, content);
	X86_STORE_U32(0x3E0A20, content);


	*a6 = content;
	return 0; //success
}

//todo
int D3DResource_Release(uint32_t a1)
{
	
	return 0;
}

constexpr size_t FakeBufferSize = 0x4000;
uint8_t FakeD3DBuffer[FakeBufferSize];
uint32_t D3D_MakeRequestedSpace_()
{	
	auto ctx = GetX86Context();
	auto base = g_memory.base;

	auto addr = g_memory.MapVirtual(g_userHeap.AllocPhysical(0x4000, 4096));

	X86_STORE_U32(0x3E0A24, addr);
	return addr;
}

uint32_t D3DDevice_SetRenderState_VertexBlend(int a1)
{
	return 0;
}

uint32_t D3DDevice_SetRenderState_FogColor(int a1)
{
	return 0;
}

uint32_t D3DDevice_SetRenderStateSTUB(int a1)
{
	return 0;
}

uint32_t D3DDevice_SetRenderStateSTUBEsp(int a1)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 4;
	return 0;
}

uint32_t D3DDevice_SetRenderState_FillMode(void* this_, int a1)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 4;
	return 0;
}

uint32_t sub_3D2FF0_r(int a1, int a2)
{

	return 0;
}

int D3D_CommonSetMultiSampleModeAndScale(void* a1, int a2)
{
	return 0;
}

unsigned int D3DDevice_SetTextureStageStateNotInline(int a1, int a2, unsigned int a3)
{
	return 0;
}

//todo
int D3DDevice_SetTexture(void* this_, int a2, int a3)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 8;
	return 0;
}

int D3DTexture_GetSurfaceLevel_r(void* this_, int a2, int a3)
{
	return 0;
}

unsigned int D3DDevice_SetRenderTarget(int a1, int a2)
{
	return 1;
}

unsigned int D3DDevice_SetViewport(DWORD* a1)
{
	return 1;
}

unsigned int D3DDevice_Clear(int a1, int a2, int a3, int a4, float a5, int a6)
{
	return 1;
}

int D3DDevice_SetTransform(int a1, float* a2)
{
	return 0;
}

//not really DX related, but is a part of "RenderStart" class
int sub_157570_r(int a1)
{
	return 0;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3DDAB0, Direct3D_CreateDevice_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD8B0, D3D_GetAdapterModeCount);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD950, D3D_EnumAdapterModes);

GUEST_FUNCTION_HOOK_STDCALL(sub_3D2280, SetVertexShader);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DC590, SetPixelShader);

GUEST_FUNCTION_HOOK_STDCALL(sub_3D5F10, D3DResource_Release);
GUEST_FUNCTION_HOOK(sub_3D4C00, D3D_MakeRequestedSpace_);

//--------------set renderstate stuff--------------
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2C40, D3DDevice_SetRenderState_VertexBlend);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D27A0, D3DDevice_SetRenderState_FogColor);

//special case that crash by using FS segment, should be resolved once we implement proper rendering and skip FS
GUEST_FUNCTION_HOOK_THISCALL(sub_3D2B10, D3DDevice_SetRenderState_FillMode);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2B60, D3DDevice_SetRenderStateSTUB);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2BC0, D3DDevice_SetRenderStateSTUB);

//idk what this does
GUEST_FUNCTION_HOOK_THISCALL(sub_3D2190, D3DDevice_SetRenderStateSTUB); //no esp adjust
GUEST_FUNCTION_HOOK_FASTCALL(sub_3D2FF0, sub_3D2FF0_r); //bunch of floating point

GUEST_FUNCTION_HOOK_STDCALL(sub_3D3A60, D3DDevice_SetRenderStateSTUB); //multisamplemask
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2760, D3DDevice_SetRenderStateSTUB); //shadowfunc
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2940, D3DDevice_SetRenderStateSTUB); //linewidth
GUEST_FUNCTION_HOOK_STDCALL(sub_3D29B0, D3DDevice_SetRenderStateSTUB); //Dxt1NoiseEnable
GUEST_FUNCTION_HOOK_STDCALL(sub_3D3810, D3DDevice_SetRenderStateSTUB); //YuvEnable
GUEST_FUNCTION_HOOK_STDCALL(sub_3D3840, D3DDevice_SetRenderStateSTUB); //OcclusionCullEgnable
GUEST_FUNCTION_HOOK_THISCALL(sub_3D38B0, D3DDevice_SetRenderStateSTUBEsp); //StencilCullEnable
//--------------end of renderstate stuff--------------

DWORD* D3D_CommonSetDebugRegisters()
{
	return 0;
}

GUEST_FUNCTION_HOOK(sub_3D3240, D3D_CommonSetDebugRegisters); //CommonSetDebugRegisters
GUEST_FUNCTION_HOOK_FASTCALL(sub_3D3310, D3D_CommonSetMultiSampleModeAndScale); 

//texture stuff
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2E90, D3DDevice_SetTextureStageStateNotInline); 
GUEST_FUNCTION_HOOK_THISCALL(sub_3D0CD0, D3DDevice_SetTexture); //esp sub AND retn adjust esp lol



GUEST_FUNCTION_STUB(sub_37DF88); //might not be graphic related? Ton of missing float instruction seem that the game can work without this

int D3DTexture_GetSurfaceLevel2_r(int a1, int a2)
{
	return 0;
}

DWORD* D3DDevice_SetRenderState_ZEnable(int a1)
{
	return 0;
}

DWORD* D3DDevice_SetRenderState_CullMode(void* this_, int a2)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 4;
	return 0;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3DD7C0, D3DTexture_GetSurfaceLevel2_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0040, D3DDevice_SetRenderTarget);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0680, D3DDevice_SetViewport);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D6BF0, D3DDevice_Clear);
GUEST_FUNCTION_HOOK_STDCALL(sub_157570, sub_157570_r); //remove when graphics is implemented
GUEST_FUNCTION_HOOK_STDCALL(sub_3CFCD0, D3DDevice_SetTransform);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D3670, D3DDevice_SetRenderState_ZEnable);

GUEST_FUNCTION_HOOK_THISCALL(sub_3D27F0, D3DDevice_SetRenderState_CullMode);

unsigned int D3D_LazySetPointParams(int a1)
{
	return 0;
}

int RW_SetTextureStageStates(int a1, int a2)
{
	return 0;
}

unsigned int D3DDevice_DrawVerticesUP(int a1, unsigned int a2, int a3, int a4)
{
	return 0;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3D86F0, D3D_LazySetPointParams);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DB990, RW_SetTextureStageStates);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DBFE0, D3DDevice_DrawVerticesUP);

unsigned int D3D_SetFence(void* this_, char a2)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 4;
	return 0;
}

int D3DDevice_Swap(void* this_, int a1)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 4;
	return 1;
}

GUEST_FUNCTION_HOOK_THISCALL(sub_3D4890, D3D_SetFence);
GUEST_FUNCTION_HOOK_THISCALL(sub_3DD560, D3DDevice_Swap);