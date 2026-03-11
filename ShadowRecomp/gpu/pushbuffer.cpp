#include "video.h"
#include "vertexdeclaration.h"
#include "vertexshader.h"
#include <kernel/function.h>

// Xbox originally supports immediate drawing by sending some kind of command queue to the GPU directly, bypassing the need of 
// calling DrawPrimitive functions, although, this system can also be used for non drawing operations.
// For rendering, it is usually done by calling D3DDevice_BeginPush call, a D3DDevice_SetVertexData* call per vertex and one 
// D3DDevice_EndPush API call. Similar to the other Xbox D3D APIs, these APIs "push" commands and data towards the Xbox GPU 
// (called NV2A) to perform drawing. More Info: https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/wiki/Xbox-D3D-drawing-via-NV2A-Kelvin

// In the case of ShadowRecomp, rather than trying to emulate the entire push system command of Xbox, we're going to take shortcuts 
// and manually patch the few functions that need it.


static float ClearQuadVertex[16]; // 4 vertices, Stride 16 (float X Y Z W)
static float RasterRenderQuadVertex[24]; // 4 vertices, Stride 24 (float X Y Z W)

static float SubmitNodeQuadVertex[256]; //arbitrary limitation, should be enough to handle all case scenarios

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

void HLE_draw_inline_array(X_D3DPRIMITIVETYPE primitiveMode, int32_t VertexCount, void* VertexData)
{

	//DWORD vertex data array, 
	// To be used as a replacement for DrawVerticesUP, the caller needs to set the vertex format using IDirect3DDevice8::SetVertexShader before calling BeginPush.
	// All attributes in the vertex format must be padded DWORD multiples, and the vertex attributes must be specified in the canonical FVF ordering
	// (position followed by weight, normal, diffuse, and so on).
	// retrieve vertex shader


	DWORD dwVertexStride = GetStrideFromVertexDeclaration(XSR_HostGetVertexDeclaration((size_t)g_memory.Translate(g_VertexShader_Handle), g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction));
	if (dwVertexStride > 0)
	{
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
		ClearQuadVertex[i] = X86_LOAD_F32(addr + i * sizeof(float));
	}

	//printf("draw quad clear for 640x480 RT composite.\n");
	HLE_draw_inline_array(primitiveMode, 4, ClearQuadVertex);
}

//todo check if this one is correct, I don't know where the game uses that
void RasterRenderQuadMidAsmHook(X86Register& eax)
{

	auto base = g_memory.base;


	intptr_t addr = eax.u32 - (24 * sizeof(DWORD)) - 4;

	X_D3DPRIMITIVETYPE primitiveMode = (X_D3DPRIMITIVETYPE)X86_LOAD_U32(addr - 8);
	// Vertex data starts after: 0x417FC, 8, 0x40601818
	intptr_t vertexDataAddr = addr - (24 * sizeof(DWORD));


	for (int i = 0; i < 24; i++)
	{
		RasterRenderQuadVertex[i] = X86_LOAD_F32(vertexDataAddr + i * sizeof(float));
	}

	HLE_draw_inline_array(primitiveMode, 4, RasterRenderQuadVertex);
}


const int32_t dword_55FDEC = 0x55FDEC;  //used to detect the flag for stride 24 vs 16
// This is used to draw the homing attack or spin dash aura, as well as shooting from guns.
void _XbSubmitNodeMidAsmHook(X86Register& eax)
{

	auto base = g_memory.base;

	//Get stride from flag
	uint32_t v1_addr = X86_LOAD_U32(dword_55FDEC) + 12;
	uint8_t flagStride = X86_LOAD_U8(v1_addr);

	const int32_t stride = (flagStride & 1) ? 6 : 4; // 6 or 4 DWORDs per vertex

	// Get vertex count from dword_55FDEC
	uint32_t dword_55FDEC_ptr = X86_LOAD_U32(dword_55FDEC);
	uint16_t vertexCount = X86_LOAD_U16(dword_55FDEC_ptr);

	if (vertexCount > 256) //if this condition ever pass, we should consisder increasing the size of SubmitNodeQuadVertex array
	{
		printf("WARNING Too many vertices for SubmitNodeQuad detected, capping to 256!!!\n");
		vertexCount = 256;
	}

	// Calculate total vertex data size
	const int32_t totalDwords = vertexCount * stride;

	int32_t vertexDataAddr = eax.u32 - sizeof(DWORD) - (totalDwords * sizeof(DWORD));

	// Primitive type is at vertexDataAddr - 8
	X_D3DPRIMITIVETYPE primitiveType = (X_D3DPRIMITIVETYPE)X86_LOAD_U32(vertexDataAddr - 8);

	for (int i = 0; i < totalDwords; i++)
	{
		SubmitNodeQuadVertex[i] = X86_LOAD_F32(vertexDataAddr + i * sizeof(float));
	}

	HLE_draw_inline_array(primitiveType, vertexCount, SubmitNodeQuadVertex);
}


//this is needed so weighted characters have proper textures / material color, and lighting.
// no push function used here, so we just copy constants.
void _rpSkinXboxSetMaterialColor(int32_t a1addr, int32_t a2)
{
	intptr_t a1 = (intptr_t)g_memory.Translate(a1addr);
	float g_VSConstants[4];
	auto base = g_memory.base;

	//lighting
	constexpr int LIGHT_CONST = 5;
	auto v3 = *((float*)a1 + 3);

	float r = X86_LOAD_F32(0x55B3C0);
	float g = X86_LOAD_F32(0x55B3C4);
	float b = X86_LOAD_F32(0x55B3C8);
	float a = X86_LOAD_F32(0x55B3CC);
	if (v3 == 1.0)
	{
		g_VSConstants[0] = r;
		g_VSConstants[1] = g;
		g_VSConstants[2] = g;
	}
	else
	{
		g_VSConstants[0] = r * v3;
		g_VSConstants[1] = g * v3;
		g_VSConstants[2] = b * v3;
	}

	g_VSConstants[3] = a;

	CopyVertexShaderConstant(LIGHT_CONST, (DWORD*)g_VSConstants, 1);

	//material texture
	constexpr int COLOR_CONST = 191;

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