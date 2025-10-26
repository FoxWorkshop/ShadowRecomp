#include <stdafx.h>
#include <kernel/function.h>
#include <kernel/io/file_system.h>
#include <cpu/guest_thread.h>
#include <kernel/kernel.h>
#include <iostream>
#include <filesystem>


//trick the game to make it think hardware audio is ready
X86_FUNC_IMPL(__imp__sub_3EAFBA);
X86_FUNC(sub_3EAFBA)
{
	g_mmio[131088] = 0x20;
	__imp__sub_3EAFBA(ctx, base);
}

//same as above
X86_FUNC_IMPL(__imp__sub_3EBC18);
X86_FUNC(sub_3EBC18)
{
	g_mmio[4194604] |= 2;
	*(uint32_t*)&g_mmio[4194608] |= 0x100;
	__imp__sub_3EBC18(ctx, base);
}

//same as above
X86_FUNC_IMPL(__imp__sub_3E8B60);
X86_FUNC(sub_3E8B60)
{
	*(uint32_t*)&g_mmio[131088] = 0x80;
	__imp__sub_3E8B60(ctx, base);
}

uint32_t fakeOffset = 0;
const uint32_t bufferSize = 0x10000;

//IDirectSoundBuffer_SetCurrentPosition
X86_FUNC_IMPL(__imp__sub_3E4258);
X86_FUNC(sub_3E4258)
{
	auto a2 = X86_LOAD_U32(ctx.esp.u32 + 8);
	if (!a2)
	{
		fakeOffset = 0;
	}
	__imp__sub_3E4258(ctx, base);
}

//the most important hack, in order to progress, DirectSoundGetCurrentPosition expect a hardware value that increase consistently
//checking the logs on CXBX, the value is different per session, using a dummy value here seems to work but we will probably need a better way at some point.
//worth to specify that DirectSoundGetCurrentPosition is needed for decoding and playing adx.
X86_FUNC_IMPL(__imp__sub_3E763F);
X86_FUNC(sub_3E763F)
{
	__imp__sub_3E763F(ctx, base);

	if (!ctx.eax.u32)
	{
		fakeOffset += 0x1000;
		ctx.eax.u32 = fakeOffset;
	}
}

//this function wait for a xbox hardware signal, we remove the check with this hook.
X86_FUNC(sub_3E65E6)
{
	uint32_t valUint320 = 0;

	// push ebp
	X86_PUSH(ctx, base, ctx.ebp.u32);
	// mov ebp, esp
	ctx.ebp.u32 = ctx.esp.u32;
	// sub esp, 0x1C
	auto temp_3E65E9_U32_OLD = ctx.esp.u32;
	auto temp_3E65E9_U32_SRC = 28;
	ctx.esp.u32 = temp_3E65E9_U32_OLD - temp_3E65E9_U32_SRC;
	ctx.eflags.update_flags_sub_U32(temp_3E65E9_U32_OLD, temp_3E65E9_U32_SRC, ctx.esp.u32);
	// push ebx
	X86_PUSH(ctx, base, ctx.ebx.u32);
	// push esi
	X86_PUSH(ctx, base, ctx.esi.u32);
	// push edi
	X86_PUSH(ctx, base, ctx.edi.u32);
	// mov edi, ecx
	ctx.edi.u32 = ctx.ecx.u32;
	// mov eax, [edi+0x08]
	ctx.eax.u32 = X86_LOAD_U32(ctx.edi.u32 + 8);
	// mov eax, [eax+0x10]
	ctx.eax.u32 = X86_LOAD_U32(ctx.eax.u32 + 16);
	// mov ebx, [eax]
	ctx.ebx.u32 = X86_LOAD_U32(ctx.eax.u32);
	// mov eax, [ebx+0x80C]
	ctx.eax.u32 = X86_LOAD_U32(ctx.ebx.u32 + 2060);
	// add eax, [ebx+0x804]
	auto temp_3E65FF_U32_OLD = ctx.eax.u32;
	auto temp_3E65FF_U32_SRC = X86_LOAD_U32(ctx.ebx.u32 + 2052);
	ctx.eax.u32 = temp_3E65FF_U32_OLD + temp_3E65FF_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3E65FF_U32_OLD, temp_3E65FF_U32_SRC, ctx.eax.u32);
	// mov ecx, [edi+0x20]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.edi.u32 + 32);
	// lea esi, [ebx+eax*4+0x818]
	ctx.esi.u32 = ctx.ebx.u32 + ctx.eax.u32 * 4 + 2072;
	// mov eax, [esi]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esi.u32);
	// dec eax
	auto temp_3E6611_U32 = ctx.eax.u32;
	ctx.eax.u32--;
	ctx.eflags.update_flags_dec_U32(temp_3E6611_U32, ctx.eax.u32);
	// shl eax, 0x05
	auto temp_4089362 = ctx.eax.u32;
	ctx.eax.u32 <<= 5;
	ctx.eflags.update_flags_shl(temp_4089362, 5, ctx.eax.u32);
	// add eax, 0x28
	auto temp_3E6615_U32_OLD = ctx.eax.u32;
	auto temp_3E6615_U32_SRC = 40;
	ctx.eax.u32 = temp_3E6615_U32_OLD + temp_3E6615_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3E6615_U32_OLD, temp_3E6615_U32_SRC, ctx.eax.u32);
	// test ecx, ecx
	ctx.eflags.update_test_flags_U32(ctx.ecx.u32 & ctx.ecx.u32);
	// mov [ebp-0x14], edi
	X86_STORE_U32(ctx.ebp.u32 - 20, ctx.edi.u32);
	// mov [ebp-0x10], eax
	X86_STORE_U32(ctx.ebp.u32 - 16, ctx.eax.u32);
	// jz 0x003E662F
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3E662F;
	// push ecx
	X86_PUSH(ctx, base, ctx.ecx.u32);
	// call 0x003E5B9C
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3E5B9C(ctx, base);
	// and dword ptr [edi+0x20], 0x00
	auto temp_3E6628_U32 = X86_LOAD_U32(ctx.edi.u32 + 32);
	temp_3E6628_U32 &= 0x0;
	X86_STORE_U32(ctx.edi.u32 + 32, temp_3E6628_U32);
	ctx.eflags.update_flags_logic(temp_3E6628_U32);
	// mov eax, [ebp-0x10]
	ctx.eax.u32 = X86_LOAD_U32(ctx.ebp.u32 - 16);
loc_3E662F:
	// push 0x01
	X86_PUSH(ctx, base, 0x1);
	// push eax
	X86_PUSH(ctx, base, ctx.eax.u32);
	// push 0x626F5344
	X86_PUSH(ctx, base, 0x626F5344);
	// call 0x003E5B34
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3E5B34(ctx, base);
	// test eax, eax
	ctx.eflags.update_test_flags_U32(ctx.eax.u32 & ctx.eax.u32);
	// mov [edi+0x20], eax
	X86_STORE_U32(ctx.edi.u32 + 32, ctx.eax.u32);
	// jnz 0x003E664D
	if ((ctx.eflags.value & X86EFlags::ZF) == 0) goto loc_3E664D;
	// mov eax, 0x8007000E
	ctx.eax.u32 = 0x8007000Eu;
	// jmp 0x003E6754
	goto loc_3E6754;
loc_3E664D:
	// push [esi+0x04]
	X86_PUSH(ctx, base, X86_LOAD_U32(ctx.esi.u32 + 4));
	// mov ecx, [edi+0x08]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.edi.u32 + 8);
	// call 0x003EAD6D
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3EAD6D(ctx, base);
	// test eax, eax
	ctx.eflags.update_test_flags_U32(ctx.eax.u32 & ctx.eax.u32);
	// jl 0x003E6754
	if ((((ctx.eflags.value & X86EFlags::SF) != 0) != ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_3E6754;
	// lea eax, [ebx+0x818]
	ctx.eax.u32 = ctx.ebx.u32 + 2072;
	// mov [ebp-0x04], eax
	X86_STORE_U32(ctx.ebp.u32 - 4, ctx.eax.u32);
	// mov eax, [ebp-0x10]
	ctx.eax.u32 = X86_LOAD_U32(ctx.ebp.u32 - 16);
	// add eax, esi
	auto temp_3E666C_U32_OLD = ctx.eax.u32;
	auto temp_3E666C_U32_SRC = ctx.esi.u32;
	ctx.eax.u32 = temp_3E666C_U32_OLD + temp_3E666C_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3E666C_U32_OLD, temp_3E666C_U32_SRC, ctx.eax.u32);
	// mov [ebp-0x08], eax
	X86_STORE_U32(ctx.ebp.u32 - 8, ctx.eax.u32);
	// lea eax, [ebp-0x1C]
	ctx.eax.u32 = ctx.ebp.u32 - 28;
	// push eax
	X86_PUSH(ctx, base, ctx.eax.u32);
	// call 0x003EAEBE
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3EAEBE(ctx, base);
	// mov eax, [esi]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esi.u32);
	// push 0x00
	X86_PUSH(ctx, base, 0x0);
	// push [ebp-0x08]
	X86_PUSH(ctx, base, X86_LOAD_U32(ctx.ebp.u32 - 8));
	// shl eax, 0x03
	auto temp_4089473 = ctx.eax.u32;
	ctx.eax.u32 <<= 3;
	ctx.eflags.update_flags_shl(temp_4089473, 3, ctx.eax.u32);
	// push eax
	X86_PUSH(ctx, base, ctx.eax.u32);
	// push [ebp-0x08]
	X86_PUSH(ctx, base, X86_LOAD_U32(ctx.ebp.u32 - 8));
	// lea eax, [ebp-0x1C]
	ctx.eax.u32 = ctx.ebp.u32 - 28;
	// push eax
	X86_PUSH(ctx, base, ctx.eax.u32);
	// call 0x003EAEEB
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3EAEEB(ctx, base);
	// test eax, eax
	ctx.eflags.update_test_flags_U32(ctx.eax.u32 & ctx.eax.u32);
	// jle 0x003E66A1
	if (((ctx.eflags.value & X86EFlags::ZF) != 0) || (((ctx.eflags.value & X86EFlags::SF) != 0) != ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_3E66A1;
	// and eax, 0xFFFF
	ctx.eax.u32 &= 0xffff;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
	// or eax, 0x80070000
	ctx.eax.u32 |= 0x80070000;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
	// test eax, eax
	ctx.eflags.update_test_flags_U32(ctx.eax.u32 & ctx.eax.u32);
loc_3E66A1:
	// jl 0x003E6754
	if ((((ctx.eflags.value & X86EFlags::SF) != 0) != ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_3E6754;
	// and dword ptr [ebp-0x0C], 0x00
	auto temp_3E66A7_U32 = X86_LOAD_U32(ctx.ebp.u32 - 12);
	temp_3E66A7_U32 &= 0x0;
	X86_STORE_U32(ctx.ebp.u32 - 12, temp_3E66A7_U32);
	ctx.eflags.update_flags_logic(temp_3E66A7_U32);
	// cmp dword ptr [esi], 0x00
	ctx.eflags.update_compare_flags_U32(X86_LOAD_U32(ctx.esi.u32), static_cast<uint32_t>(0x0u));
	// jbe 0x003E672F
	if (((ctx.eflags.value & X86EFlags::CF) != 0) || ((ctx.eflags.value & X86EFlags::ZF) != 0)) goto loc_3E672F;
	// mov eax, [ebp-0x08]
	ctx.eax.u32 = X86_LOAD_U32(ctx.ebp.u32 - 8);
	// mov [ebp-0x08], eax
	X86_STORE_U32(ctx.ebp.u32 - 8, ctx.eax.u32);
	// lea edi, [esi+0x08]
	ctx.edi.u32 = ctx.esi.u32 + 8;
loc_3E66B9:
	// push 0x00
	X86_PUSH(ctx, base, 0x0);
	// push [ebp-0x04]
	X86_PUSH(ctx, base, X86_LOAD_U32(ctx.ebp.u32 - 4));
	// push [edi+0x04]
	X86_PUSH(ctx, base, X86_LOAD_U32(ctx.edi.u32 + 4));
	// push [ebp-0x04]
	X86_PUSH(ctx, base, X86_LOAD_U32(ctx.ebp.u32 - 4));
	// push [ebp-0x08]
	X86_PUSH(ctx, base, X86_LOAD_U32(ctx.ebp.u32 - 8));
	// call 0x003EAEEB
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3EAEEB(ctx, base);
	// test eax, eax
	ctx.eflags.update_test_flags_U32(ctx.eax.u32 & ctx.eax.u32);
	// jle 0x003E66DC
	if (((ctx.eflags.value & X86EFlags::ZF) != 0) || (((ctx.eflags.value & X86EFlags::SF) != 0) != ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_3E66DC;
	// and eax, 0xFFFF
	ctx.eax.u32 &= 0xffff;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
	// or eax, 0x80070000
	ctx.eax.u32 |= 0x80070000;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
	// test eax, eax
	ctx.eflags.update_test_flags_U32(ctx.eax.u32 & ctx.eax.u32);
loc_3E66DC:
	// jl 0x003E6754
	if ((((ctx.eflags.value & X86EFlags::SF) != 0) != ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_3E6754;
	// mov eax, [edi+0x04]
	ctx.eax.u32 = X86_LOAD_U32(ctx.edi.u32 + 4);
	// add [ebp-0x04], eax
	auto temp_3E66E1_U32_OLD = X86_LOAD_U32(ctx.ebp.u32 - 4);
	auto temp_3E66E1_U32_SRC = ctx.eax.u32;
	auto temp_3E66E1_U32_RES = temp_3E66E1_U32_OLD + temp_3E66E1_U32_SRC;
	X86_STORE_U32(ctx.ebp.u32 - 4, temp_3E66E1_U32_RES);
	ctx.eflags.update_flags_add_U32(temp_3E66E1_U32_OLD, temp_3E66E1_U32_SRC, temp_3E66E1_U32_RES);
	// mov eax, [ebp-0x14]
	ctx.eax.u32 = X86_LOAD_U32(ctx.ebp.u32 - 20);
	// mov ecx, [eax+0x0C]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.eax.u32 + 12);
	// mov ecx, [ecx+0x04]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.ecx.u32 + 4);
	// add dword ptr [edi+0x10], 0xFE836000
	auto temp_3E66ED_U32_OLD = X86_LOAD_U32(ctx.edi.u32 + 16);
	auto temp_3E66ED_U32_SRC = 18446744073684606976;
	auto temp_3E66ED_U32_RES = temp_3E66ED_U32_OLD + temp_3E66ED_U32_SRC;
	X86_STORE_U32(ctx.edi.u32 + 16, temp_3E66ED_U32_RES);
	ctx.eflags.update_flags_add_U32(temp_3E66ED_U32_OLD, temp_3E66ED_U32_SRC, temp_3E66ED_U32_RES);
	// add dword ptr [ebp-0x08], 0x08
	auto temp_3E66F4_U32_OLD = X86_LOAD_U32(ctx.ebp.u32 - 8);
	auto temp_3E66F4_U32_SRC = 8;
	auto temp_3E66F4_U32_RES = temp_3E66F4_U32_OLD + temp_3E66F4_U32_SRC;
	X86_STORE_U32(ctx.ebp.u32 - 8, temp_3E66F4_U32_RES);
	ctx.eflags.update_flags_add_U32(temp_3E66F4_U32_OLD, temp_3E66F4_U32_SRC, temp_3E66F4_U32_RES);
	// sub ecx, 0x17C6818
	auto temp_3E66F8_U32_OLD = ctx.ecx.u32;
	auto temp_3E66F8_U32_SRC = 24930328;
	ctx.ecx.u32 = temp_3E66F8_U32_OLD - temp_3E66F8_U32_SRC;
	ctx.eflags.update_flags_sub_U32(temp_3E66F8_U32_OLD, temp_3E66F8_U32_SRC, ctx.ecx.u32);
	// add [edi], ecx
	auto temp_3E66FE_U32_OLD = X86_LOAD_U32(ctx.edi.u32);
	auto temp_3E66FE_U32_SRC = ctx.ecx.u32;
	auto temp_3E66FE_U32_RES = temp_3E66FE_U32_OLD + temp_3E66FE_U32_SRC;
	X86_STORE_U32(ctx.edi.u32, temp_3E66FE_U32_RES);
	ctx.eflags.update_flags_add_U32(temp_3E66FE_U32_OLD, temp_3E66FE_U32_SRC, temp_3E66FE_U32_RES);
	// mov ecx, 0xFFA0BE7A
	ctx.ecx.u32 = 0xFFA0BE7Au;
	// sub ecx, [ebx+0x804]
	auto temp_3E6705_U32_OLD = ctx.ecx.u32;
	auto temp_3E6705_U32_SRC = X86_LOAD_U32(ctx.ebx.u32 + 2052);
	ctx.ecx.u32 = temp_3E6705_U32_OLD - temp_3E6705_U32_SRC;
	ctx.eflags.update_flags_sub_U32(temp_3E6705_U32_OLD, temp_3E6705_U32_SRC, ctx.ecx.u32);
	// add edi, 0x20
	auto temp_3E670B_U32_OLD = ctx.edi.u32;
	auto temp_3E670B_U32_SRC = 32;
	ctx.edi.u32 = temp_3E670B_U32_OLD + temp_3E670B_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3E670B_U32_OLD, temp_3E670B_U32_SRC, ctx.edi.u32);
	// shl ecx, 0x02
	auto temp_4089614 = ctx.ecx.u32;
	ctx.ecx.u32 <<= 2;
	ctx.eflags.update_flags_shl(temp_4089614, 2, ctx.ecx.u32);
	// add [edi-0x18], ecx
	auto temp_3E6711_U32_OLD = X86_LOAD_U32(ctx.edi.u32 - 24);
	auto temp_3E6711_U32_SRC = ctx.ecx.u32;
	auto temp_3E6711_U32_RES = temp_3E6711_U32_OLD + temp_3E6711_U32_SRC;
	X86_STORE_U32(ctx.edi.u32 - 24, temp_3E6711_U32_RES);
	ctx.eflags.update_flags_add_U32(temp_3E6711_U32_OLD, temp_3E6711_U32_SRC, temp_3E6711_U32_RES);
	// mov eax, [eax+0x08]
	ctx.eax.u32 = X86_LOAD_U32(ctx.eax.u32 + 8);
	// mov eax, [eax+0x14]
	ctx.eax.u32 = X86_LOAD_U32(ctx.eax.u32 + 20);
	// sub eax, 0xC000
	auto temp_3E671A_U32_OLD = ctx.eax.u32;
	auto temp_3E671A_U32_SRC = 49152;
	ctx.eax.u32 = temp_3E671A_U32_OLD - temp_3E671A_U32_SRC;
	ctx.eflags.update_flags_sub_U32(temp_3E671A_U32_OLD, temp_3E671A_U32_SRC, ctx.eax.u32);
	// add [edi-0x08], eax
	auto temp_3E671F_U32_OLD = X86_LOAD_U32(ctx.edi.u32 - 8);
	auto temp_3E671F_U32_SRC = ctx.eax.u32;
	auto temp_3E671F_U32_RES = temp_3E671F_U32_OLD + temp_3E671F_U32_SRC;
	X86_STORE_U32(ctx.edi.u32 - 8, temp_3E671F_U32_RES);
	ctx.eflags.update_flags_add_U32(temp_3E671F_U32_OLD, temp_3E671F_U32_SRC, temp_3E671F_U32_RES);
	// inc [ebp-0x0C]
	auto temp_3E6722_U32_OLD = X86_LOAD_U32(ctx.ebp.u32 - 12);
	auto temp_3E6722_U32_RES = temp_3E6722_U32_OLD;
	temp_3E6722_U32_RES++;
	X86_STORE_U32(ctx.ebp.u32 - 12, temp_3E6722_U32_RES);
	ctx.eflags.update_flags_inc_U32(temp_3E6722_U32_OLD, temp_3E6722_U32_RES);
	// mov eax, [ebp-0x0C]
	ctx.eax.u32 = X86_LOAD_U32(ctx.ebp.u32 - 12);
	// cmp eax, [esi]
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, X86_LOAD_U32(ctx.esi.u32));
	// jb 0x003E66B9
	if ((ctx.eflags.value & X86EFlags::CF) != 0) goto loc_3E66B9;
	// mov edi, [ebp-0x14]
	ctx.edi.u32 = X86_LOAD_U32(ctx.ebp.u32 - 20);
loc_3E672F:
	// mov ecx, [ebp-0x10]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.ebp.u32 - 16);
	// mov edi, [edi+0x20]
	ctx.edi.u32 = X86_LOAD_U32(ctx.edi.u32 + 32);
	// mov eax, ecx
	ctx.eax.u32 = ctx.ecx.u32;
	// shr ecx, 0x02
	auto temp_4089655 = ctx.ecx.u32;
	ctx.ecx.u32 >>= 2;
	ctx.eflags.update_flags_shr(temp_4089655, 2, ctx.ecx.u32);
	// rep movsd
	for (; ctx.ecx.u32 > 0; ctx.ecx.u32--, ctx.esi.u32 += 4, ctx.edi.u32 += 4) {
		X86_STORE_U32(ctx.edi.u32, X86_LOAD_U32(ctx.esi.u32));
	}
	// mov ecx, eax
	ctx.ecx.u32 = ctx.eax.u32;
	// push 0x03
	//X86_PUSH(ctx, base, 0x3);
	X86_PUSH(ctx, base, 0x0); //ADDED: force 0
	// pop eax
	ctx.eax.u32 = X86_POP(ctx, base);
	// and ecx, eax
	ctx.ecx.u32 &= ctx.eax.u32;
	ctx.eflags.update_flags_logic(ctx.ecx.u32);
	// add ebx, 0x810
	auto temp_3E6743_U32_OLD = ctx.ebx.u32;
	auto temp_3E6743_U32_SRC = 2064;
	ctx.ebx.u32 = temp_3E6743_U32_OLD + temp_3E6743_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3E6743_U32_OLD, temp_3E6743_U32_SRC, ctx.ebx.u32);
	// rep movsb
	int direction_4089673 = (ctx.eflags.value & X86EFlags::DF) ? -1 : 1;
	for (; ctx.ecx.u32 > 0; ctx.ecx.u32--) {
		X86_STORE_U8(ctx.edi.u32, X86_LOAD_U8(ctx.esi.u32));
		ctx.esi.u32 += direction_4089673;
		ctx.edi.u32 += direction_4089673;
	}
	// mov [ebx], eax
	X86_STORE_U32(ctx.ebx.u32, ctx.eax.u32);
loc_3E674D:
	// cmp dword ptr [ebx], 0x00
	ctx.eflags.update_compare_flags_U32(X86_LOAD_U32(ctx.ebx.u32), static_cast<uint32_t>(0x0u));
	// jnz 0x003E674D
	//if ((ctx.eflags.value & X86EFlags::ZF) == 0) goto loc_3E674D; //ADDED REMOVED JNZ CHECK
	// xor eax, eax
	ctx.eax.u32 ^= ctx.eax.u32;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
loc_3E6754:
	// pop edi
	ctx.edi.u32 = X86_POP(ctx, base);
	// pop esi
	ctx.esi.u32 = X86_POP(ctx, base);
	// pop ebx
	ctx.ebx.u32 = X86_POP(ctx, base);
	// leave
	ctx.esp.u32 = ctx.ebp.u32;
	ctx.ebp.u32 = X86_POP(ctx, base);
	// ret 0x04
	ctx.esp.u32 += 0x4;  //Adjust esp for return value (0xC2 0x4)
	ctx.esp.u32 += 0x4;  //Adjust esp for return instruction
	return;
}

//todo delete when implement audio properly
HRESULT IDirectSoundBuffer_GetStatus(int32_t buffer, DWORD* pdwStatus)
{

	*pdwStatus = 0x00010000;


	return S_OK;
}



GUEST_FUNCTION_HOOK_STDCALL(sub_3E421C, IDirectSoundBuffer_GetStatus);



int32_t DirectSoundCreate_r(int32_t* pcGuidDevice, int32_t* ppDS, int32_t* pUnkOuter)
{
	return 0;
}

int32_t IDirectSound_DownloadEffectsImage(int a1, DWORD* a2, int a3, int a4, DWORD* a5)
{
	return 0;
}


GUEST_FUNCTION_HOOK_STDCALL(sub_3E5A27, DirectSoundCreate_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3E4105, IDirectSound_DownloadEffectsImage);