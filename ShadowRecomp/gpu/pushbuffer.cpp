#include "video.h"
#include "nv2a.h"
#include "vertexdeclaration.h"
#include "vertexshader.h"
#include <kernel/function.h>

// Xbox originally supports immediate drawing by sending some kind of command queue to the GPU directly, bypassing the need of 
// calling DrawPrimitive functions. It is done by calling D3DDevice_BeginPush call, a D3DDevice_SetVertexData* call per vertex and one 
// D3DDevice_EndPush API call. Similar to the other Xbox D3D APIs, these APIs "push" commands and data towards the Xbox GPU 
// (called NV2A) to perform drawing. More Info: https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/wiki/Xbox-D3D-drawing-via-NV2A-Kelvin

// In the case of ShadowRecomp, rather than trying to emulate the entire push system command, we're going to take shortcuts 
// and manually patch the few draw calls that we need.


typedef unsigned long       DWORD;
typedef DWORD NV2AMETHOD;
#define CONTIGUOUS_MEMORY_BASE  0x80000000


static float ClearQuadVertex[16]; // 4 vertices, Stride 16 (float X Y Z W)

DWORD GetStrideFromVertexDeclaration(HostVertexDeclaration* pVertexDeclaration)
{
	DWORD Stride = 0;

	if (pVertexDeclaration)
	{

		if (pVertexDeclaration->NumberOfVertexStreams == 1) {
			// Note : This assumes that the only stream in use will be stream zero :
			Stride = pVertexDeclaration->VertexStreams[0].HostVertexStride;
		}
		else {
			printf("Non-FVF Vertex Shaders with multiple streams not supported for PushBuffer emulation!\n");
		}
	}
	else {
		printf("Missing Vertex Declaration not supported for PushBuffer emulation!\n");
	}

	return Stride;
}

void HLE_draw_inline_array(NV2AState* d)
{
	PGRAPHState* pg = &d->pgraph;

	//DWORD vertex data array, 
	// To be used as a replacement for DrawVerticesUP, the caller needs to set the vertex format using IDirect3DDevice8::SetVertexShader before calling BeginPush.
	// All attributes in the vertex format must be padded DWORD multiples, and the vertex attributes must be specified in the canonical FVF ordering
	// (position followed by weight, normal, diffuse, and so on).
	// retrieve vertex shader

	bool isFixed = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;

	DWORD dwVertexStride = GetStrideFromVertexDeclaration(HostGetVertexDeclaration((size_t)g_memory.Translate(g_VertexShader_Handle), isFixed));
	if (dwVertexStride > 0)
	{
		uint32_t VertexCount = (pg->inline_array_length * sizeof(DWORD)) / dwVertexStride;

		DrawPrimitiveUP((X_D3DPRIMITIVETYPE)pg->primitive_mode, VertexCount, pg->inline_array, dwVertexStride);
	}
}

void HLE_draw_inline_elements(NV2AState* d)
{
	PGRAPHState* pg = &d->pgraph;
	unsigned int uiIndexCount = pg->inline_elements_length;

	DrawIndexedPrimitive((X_D3DPRIMITIVETYPE)pg->primitive_mode,
		uiIndexCount,
		(int16_t*)d->pgraph.inline_elements);

}


static inline uint32_t ldl_le_p(const void* p)
{
	return *(uint32_t*)p;
}


extern GuestTexture* RTTexRef;
void QuickDumpTexture(GuestTexture* texture, const char* name);
void HLE_draw_inline_array(X_D3DPRIMITIVETYPE primitiveMode, int32_t VertexCount, void* VertexData)
{

	//DWORD vertex data array, 
	// To be used as a replacement for DrawVerticesUP, the caller needs to set the vertex format using IDirect3DDevice8::SetVertexShader before calling BeginPush.
	// All attributes in the vertex format must be padded DWORD multiples, and the vertex attributes must be specified in the canonical FVF ordering
	// (position followed by weight, normal, diffuse, and so on).
	// retrieve vertex shader


	DWORD dwVertexStride = GetStrideFromVertexDeclaration(HostGetVertexDeclaration((size_t)g_memory.Translate(g_VertexShader_Handle), g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction));
	if (dwVertexStride > 0)
	{
	//	printf("now drawing fullscreen quad.\n");
		DrawPrimitiveUP(primitiveMode, VertexCount, VertexData, dwVertexStride);
	}
}

void ClearWithQuadsMidAsmHook(X86Register& eax)
{

	auto base = g_memory.base;
	//since the pointer of push buffer moved until its end, we have to go back to the beginning to extract vertex data
	intptr_t addr = eax.u32 - (16 * sizeof(DWORD)) - 4; //16 (size of the stride) * number of vertex - offset

	X_D3DPRIMITIVETYPE primitiveMode = (X_D3DPRIMITIVETYPE)X86_LOAD_U32(addr - 8);
	for (int i = 0; i < 16; i++)
	{
		ClearQuadVertex[i] = X86_LOAD_F32(addr + i * 4);
	}

	//printf("draw quad clear for 640x480 RT composite.\n");
	HLE_draw_inline_array(primitiveMode, 4, ClearQuadVertex);
}


void _rpSkinXboxSetMaterialColor(int32_t a1addr, int32_t a2)
{
	constexpr int COLOR_CONST = 191;

	float g_VSConstants[4];

	intptr_t a1 = (intptr_t)g_memory.Translate(a1addr);

	if (!a2 || *(intptr_t*)(a1 + 4) == -1)
	{
		float c = *(float*)(a1 + 20);
		g_VSConstants[0] = c;
		g_VSConstants[1] = c;
		g_VSConstants[2] = c;
		g_VSConstants[3] = 1.0f;
	
	}
	else
	{
		uint8_t* color = (uint8_t*)(a1 + 4);
		float alpha = *(float*)(a1 + 20) * (1.0f / 255.0f);

		g_VSConstants[0] = color[0] * alpha;
		g_VSConstants[1] = color[1] * alpha;
		g_VSConstants[2] = color[2] * alpha;
		g_VSConstants[3] = color[3] * (1.0f / 255.0f);
	}

	CopyVertexShaderConstant(COLOR_CONST, (DWORD*)g_VSConstants, 1);
}

GUEST_FUNCTION_HOOK(sub_2C51C0, _rpSkinXboxSetMaterialColor);