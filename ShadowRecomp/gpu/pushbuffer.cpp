#include "video.h"
#include "nv2a.h"
#include "vertexdeclaration.h"
#include "vertexshader.h"

// Xbox originally supports immediate drawing by sending some kind of command queue to the GPU directly, bypassing the need of 
// calling DrawPrimitive functions. It is done by calling D3DDevice_BeginPush call, a D3DDevice_SetVertexData* call per vertex and one 
// D3DDevice_EndPush API call. Similar to the other Xbox D3D APIs, these APIs "push" commands and data towards the Xbox GPU 
// (called NV2A) to perform drawing. More Info: https://github.com/Cxbx-Reloaded/Cxbx-Reloaded/wiki/Xbox-D3D-drawing-via-NV2A-Kelvin

// This leads to inlined draw code so we need to handle that
// In the case of ShadowRecomp, rather than trying to emulate the entire push system command, we're going to take shortcuts and manually patch the few draw calls that we need.


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



void pgraph_handle_method(NV2AState* d,
	unsigned int subchannel,
	unsigned int method,
	uint32_t parameter)
{
	unsigned int i;
	unsigned int slot;

	return;

	PGRAPHState* pg = &d->pgraph;


	bool channel_valid =
		d->pgraph.regs[NV_PGRAPH_CTX_CONTROL] & NV_PGRAPH_CTX_CONTROL_CHID;
	assert(channel_valid);

	unsigned channel_id = GET_MASK(pg->regs[NV_PGRAPH_CTX_USER], NV_PGRAPH_CTX_USER_CHID);

	assert(subchannel < 8);


	if (method == NV_SET_OBJECT)
	{
		assert(parameter < d->pramin.ramin_size);
		uint8_t* obj_ptr = d->pramin.ramin_ptr + parameter;

		uint32_t ctx_1 = ldl_le_p((uint32_t*)obj_ptr);
		uint32_t ctx_2 = ldl_le_p((uint32_t*)(obj_ptr + 4));
		uint32_t ctx_3 = ldl_le_p((uint32_t*)(obj_ptr + 8));
		uint32_t ctx_4 = ldl_le_p((uint32_t*)(obj_ptr + 12));
		uint32_t ctx_5 = parameter;

		pg->regs[NV_PGRAPH_CTX_CACHE1 + subchannel * 4] = ctx_1;
		pg->regs[NV_PGRAPH_CTX_CACHE2 + subchannel * 4] = ctx_2;
		pg->regs[NV_PGRAPH_CTX_CACHE3 + subchannel * 4] = ctx_3;
		pg->regs[NV_PGRAPH_CTX_CACHE4 + subchannel * 4] = ctx_4;
		pg->regs[NV_PGRAPH_CTX_CACHE5 + subchannel * 4] = ctx_5;
	}

	// is this right?
	pg->regs[NV_PGRAPH_CTX_SWITCH1] = pg->regs[NV_PGRAPH_CTX_CACHE1 + subchannel * 4];
	pg->regs[NV_PGRAPH_CTX_SWITCH2] = pg->regs[NV_PGRAPH_CTX_CACHE2 + subchannel * 4];
	pg->regs[NV_PGRAPH_CTX_SWITCH3] = pg->regs[NV_PGRAPH_CTX_CACHE3 + subchannel * 4];
	pg->regs[NV_PGRAPH_CTX_SWITCH4] = pg->regs[NV_PGRAPH_CTX_CACHE4 + subchannel * 4];
	pg->regs[NV_PGRAPH_CTX_SWITCH5] = pg->regs[NV_PGRAPH_CTX_CACHE5 + subchannel * 4];

	uint32_t graphics_class = GET_MASK(pg->regs[NV_PGRAPH_CTX_SWITCH1],
		NV_PGRAPH_CTX_SWITCH1_GRCLASS);


	if (subchannel != 0) {
		// catches context switching issues on xbox d3d
		assert(graphics_class != 0x97);
	}

	switch (graphics_class)
	{
	case NV_KELVIN_PRIMITIVE: {

		switch (method)
		{
		case NV097_INLINE_ARRAY:
			assert(pg->inline_array_length < NV2A_MAX_BATCH_LENGTH);
			pg->inline_array[pg->inline_array_length++] = parameter;
			break;
		case NV097_SET_BEGIN_END:
		{
			uint32_t control_0 = pg->regs[NV_PGRAPH_CONTROL_0];
			uint32_t control_1 = pg->regs[NV_PGRAPH_CONTROL_1];

			bool depth_test = control_0
				& NV_PGRAPH_CONTROL_0_ZENABLE;
			bool stencil_test = control_1
				& NV_PGRAPH_CONTROL_1_STENCIL_TEST_ENABLE;

			if (parameter == NV097_SET_BEGIN_END_OP_END)
			{

				if (pg->draw_arrays_length) {


					assert(pg->inline_buffer_length == 0);
					assert(pg->inline_array_length == 0);
					assert(pg->inline_elements_length == 0);

					printf("UNIMPLEMENTED: NV097_SET_BEGIN_END with draw arrays\n");
					assert(false);
				}
				else if (pg->inline_buffer_length) {


					assert(pg->draw_arrays_length == 0);
					assert(pg->inline_array_length == 0);
					assert(pg->inline_elements_length == 0);

					printf("UNIMPLEMENTED: NV097_SET_BEGIN_END - DRAW INLINE BUFFER\n");
					assert(false);
				}
				else if (pg->inline_array_length) {


					assert(pg->draw_arrays_length == 0);
					assert(pg->inline_buffer_length == 0);
					assert(pg->inline_elements_length == 0);


					HLE_draw_inline_array(d);

				}
				else if (pg->inline_elements_length) {


					assert(pg->draw_arrays_length == 0);
					assert(pg->inline_buffer_length == 0);
					assert(pg->inline_array_length == 0);

					HLE_draw_inline_elements(d);
				}
				else {
					printf("EMPTY NV097_SET_BEGIN_END\n");
					assert(false);
				}
			}
			else {

				pg->primitive_mode = parameter;
				pg->inline_elements_length = 0;
				pg->inline_array_length = 0;
				pg->inline_buffer_length = 0;
				pg->draw_arrays_length = 0;
				pg->draw_arrays_max_count = 0;

				//	printf("SET SURFACE DIRTY UNIMPLEMENTED\n");

				break;
			}
		}
		break;
		case NV097_ARRAY_ELEMENT16:
			assert(pg->inline_elements_length < NV2A_MAX_BATCH_LENGTH);
			pg->inline_elements[pg->inline_elements_length++] = parameter & 0xFFFF;
			pg->inline_elements[pg->inline_elements_length++] = parameter >> 16;
			break;
		case NV097_ARRAY_ELEMENT32:
			assert(pg->inline_elements_length < NV2A_MAX_BATCH_LENGTH);
			pg->inline_elements[pg->inline_elements_length++] = parameter;
			break;
		default:
			printf("PGRAPH: unhandled class 0x%02X method 0x%03X subchannel %u parameter 0x%08X\n",
				graphics_class, method, subchannel, parameter);
			assert(false);
			break;
		}

		break; //end of switch method

	}
	default:
		printf("PGRAPH: unhandled class 0x%02X method 0x%03X subchannel %u parameter 0x%08X\n",
			graphics_class, method, subchannel, parameter);
		assert(false);
		break; //end of switch graphics_class
	}

}

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
		DrawPrimitiveUP(primitiveMode, VertexCount, VertexData, dwVertexStride);
	}

}


void ClearWithQuadsMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;

	//since the pointer of push buffer moved until its end, we have to go back to the beginning to extract vertex data
	intptr_t addr = eax.u32 - (16 * sizeof(DWORD)) - 4; //16 (size of the stride) * number of vertex - offset

	memset(ClearQuadVertex, 0, sizeof(ClearQuadVertex));

	X_D3DPRIMITIVETYPE primitiveMode = (X_D3DPRIMITIVETYPE)X86_LOAD_U32(addr - 8);
	for (int i = 0; i < 16; i++)
	{
		ClearQuadVertex[i] = X86_LOAD_F32(addr + i * 4);
	}

	HLE_draw_inline_array(primitiveMode, 4, ClearQuadVertex);
}