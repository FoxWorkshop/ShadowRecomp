#include "video.h"
#include "nv2a.h"
#include "vertexdeclaration.h"
#include "vertexshader.h"

typedef unsigned long       DWORD;
typedef DWORD NV2AMETHOD;
#define CONTIGUOUS_MEMORY_BASE  0x80000000

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


// For now, skip the cache, but handle the pgraph method directly
// Note : Here's where the method gets multiplied by four!
// Note 2 : d is read from local scope, and ni is unused (same in LLE)
// Note 3 : Keep EmuExecutePushBufferRaw skipping all commands not intended for channel 0 (3D)
// Note 4 : Prevent a crash during shutdown when g_NV2A gets deleted
#define CACHE_PUSH(subc, mthd, word, ni) \
	if (subc == 0) { \
		pgraph_handle_method(d, subc, mthd << 2, word); \
	}

typedef union {
	/* https://envytools.readthedocs.io/en/latest/hw/fifo/dma-pusher.html#the-commands-pre-gf100-format

		000 CCCCCCCCCCC 00 SSS MMMMMMMMMMM 00	increasing methods [NV4+]
		000 00000000000 10 000 00000000000 00	return [NV1A+, NV4-style only]
		001 JJJJJJJJJJJ JJ JJJ JJJJJJJJJJJ 00	old jump [NV4+, NV4-style only]
		010 CCCCCCCCCCC 00 SSS MMMMMMMMMMM 00	non-increasing methods [NV10+]
		JJJ JJJJJJJJJJJ JJ JJJ JJJJJJJJJJJ 01	jump [NV1A+, NV4-style only]
		JJJ JJJJJJJJJJJ JJ JJJ JJJJJJJJJJJ 10	call [NV1A+, NV4-style only]

		C = method Count, S = Subchannel, M = first Method, J = Jump address
	*/
	// Entire 32 bit command word, and an overlay for the above use-cases :
	uint32_t            word;                    /*  0 .. 31 */
	struct {
		uint32_t        type : 2;        /*  0 ..  1 */
		// See https://envytools.readthedocs.io/en/latest/hw/fifo/dma-pusher.html#nv4-control-flow-commands
#define COMMAND_TYPE_NONE        0
#define COMMAND_TYPE_JUMP_LONG   1
#define COMMAND_TYPE_CALL        2
		uint32_t        method : 11;       /*  2 .. 12 */
		uint32_t        subchannel : 3;        /* 13 .. 15 */
		uint32_t        flags : 2;        /* 16 .. 17 */
		// See https://envytools.readthedocs.io/en/latest/hw/fifo/dma-pusher.html#nv4-method-submission-commands
#define COMMAND_FLAGS_NONE                         0
#define COMMAND_FLAGS_SLI_CONDITIONAL              1 // (NV40+)
#define COMMAND_FLAGS_RETURN                       2
#define COMMAND_FLAGS_LONG_NON_INCREASING_METHODS  3 // [IB-mode only] 
		uint32_t        method_count : 11;       /* 18 .. 28 */
		uint32_t        instruction : 3;        /* 29 .. 31 */
#define COMMAND_INSTRUCTION_INCREASING_METHODS     0
#define COMMAND_INSTRUCTION_JUMP                   1
#define COMMAND_INSTRUCTION_NON_INCREASING_METHODS 2
	};
#define COMMAND_WORD_MASK_JUMP      0x1FFFFFFC /*  2 .. 31 */
#define COMMAND_WORD_MASK_JUMP_LONG 0xFFFFFFFC /*  2 .. 28 */
} nv_fifo_command;

void EmuExecutePushBufferRaw(void* pPushData, uint32_t uSizeInBytes)
{
	assert(pPushData);
	assert(uSizeInBytes >= 4);



	// DMA Pusher state -- see https://envytools.readthedocs.io/en/latest/hw/fifo/dma-pusher.html#pusher-state

	auto d = m_nv2a_state.get();
	d->pgraph.regs[NV_PGRAPH_CTX_CONTROL] |= NV_PGRAPH_CTX_CONTROL_CHID; // avoid assert in pgraph_handle_method()

	uintptr_t* dma_limit; // pushbuffer size limit
	uintptr_t* dma_put; // pushbuffer current end address
	uintptr_t* dma_get; //pushbuffer current read address
	struct {
		NV2AMETHOD mthd; // Current method
		uint32_t subc; // :3 = Current subchannel
		uint32_t mcnt; // :24 = Current method count
		bool ni; // Current command's NI (non-increasing) flag
	} dma_state;

	static uintptr_t dcount_shadow = 0; // [NV5:] Number of already-processed methods in cmd]
	static bool subr_active = false; // Subroutine active
	static uintptr_t* subr_return = nullptr; // Subroutine return address
	// static bool big_endian = false; // Pushbuffer endian switch

	// DMA troubleshooting values -- see https://envytools.readthedocs.io/en/latest/hw/fifo/dma-pusher.html#errors
	static uintptr_t* dma_get_jmp_shadow = nullptr; // value of dma_get before the last jump
	static uintptr_t rsvd_shadow = 0; // the first word of last-read command
	static uintptr_t data_shadow = 0; // the last-read data word

	// Overlay, to ease decoding the PFIFO command word
	union {
		uint32_t word;
		nv_fifo_command command;
	};

	// Initialize working variables
	dma_limit = (uintptr_t*)((uintptr_t)pPushData + uSizeInBytes); // TODO : If this an absolute addresss?
	dma_put = (uintptr_t*)((uintptr_t)pPushData + uSizeInBytes);
	dma_get = (uintptr_t*)pPushData;
	dma_state = {};

	// NV-4-style PFIFO DMA command stream pusher
	// See https://envytools.readthedocs.io/en/latest/hw/fifo/dma-pusher.html#the-pusher-pseudocode-pre-gf100
	while (dma_get != dma_put) {
		// Check if loop reaches end of pushbuffer
		if (dma_get >= dma_limit)
		{
			printf("Last pushbuffer instruction exceeds END of Data\n");
			// TODO : throw DMA_PUSHER(MEM_FAULT);
			return; // For now, don't even attempt to run through
		}

		// Read a DWORD from the current push buffer pointer
		word = *dma_get++;
		/* now, see if we're in the middle of a command */
		if (dma_state.mcnt) 
		{
			/* data word of methods command */
			data_shadow = word;

			CACHE_PUSH(dma_state.subc, dma_state.mthd, word, dma_state.ni);
			if (!dma_state.ni) {
				dma_state.mthd++;
			}

			dma_state.mcnt--;
			dcount_shadow++;
			continue; // while
		}

		/* no command active - this is the first word of a new one */
		rsvd_shadow = word;
		// Check and handle command type, then instruction, then flags
		switch (command.type) {
		case COMMAND_TYPE_NONE:
			break; // fall through
		case COMMAND_TYPE_JUMP_LONG:
			printf("Pushbuffer COMMAND_TYPE_JUMP_LONG\n");
			dma_get_jmp_shadow = dma_get;
			dma_get = (uintptr_t*)(CONTIGUOUS_MEMORY_BASE | (word & COMMAND_WORD_MASK_JUMP_LONG));
			//NV2A uses COMMAND_TYPE_JUMP_LONG as return for COMMAND_TYPE_CALL. we clear the subr_active here to indicate we have returned from COMMAND_TYPE_CALL.
			subr_active = false;
			continue; // while
		case COMMAND_TYPE_CALL: // Note : NV2A return is said not to work?
			if (subr_active) {
				printf("Pushbuffer COMMAND_TYPE_CALL while another call was active!\n");
				// TODO : throw DMA_PUSHER(CALL_SUBR_ACTIVE);
				// For now, don't even attempt to run through, this should never happened, if it happened, the pgraph handler will go crazy.

			}
			else {
				printf("Pushbuffer COMMAND_TYPE_CALL\n");
			}

			subr_return = dma_get;
			subr_active = true;
			dma_get = (uintptr_t*)(CONTIGUOUS_MEMORY_BASE | (word & COMMAND_WORD_MASK_JUMP_LONG));
			continue; // while
		default:
			printf("Pushbuffer COMMAND_TYPE unknown\n");
			// TODO : throw DMA_PUSHER(INVALID_CMD);
			return; // For now, don't even attempt to run through
		} // switch type

		switch (command.instruction) {
		case COMMAND_INSTRUCTION_INCREASING_METHODS:
			dma_state.ni = false;
			break;
		case COMMAND_INSTRUCTION_JUMP:
			printf("Pushbuffer COMMAND_INSTRUCTION_JUMP\n");
			dma_get_jmp_shadow = dma_get;
			dma_get = (uintptr_t*)(CONTIGUOUS_MEMORY_BASE | (word & COMMAND_WORD_MASK_JUMP));
			continue; // while
		case COMMAND_INSTRUCTION_NON_INCREASING_METHODS:
			dma_state.ni = true;
			break;
		default:
			printf("Pushbuffer COMMAND_INSTRUCTION unknown\n");
			// TODO : throw DMA_PUSHER(INVALID_CMD);
			return; // For now, don't even attempt to run through
		} // switch instruction

		switch (command.flags) {
		case COMMAND_FLAGS_NONE: // Decode push buffer method & size (inverse of D3DPUSH_ENCODE)
			dma_state.mthd = command.method;
			dma_state.subc = command.subchannel;
			dma_state.mcnt = command.method_count;
			break;
		case COMMAND_FLAGS_RETURN: // Note : NV2A return is said not to work?
			if (word != 0x00020000) {
				printf("Pushbuffer COMMAND_FLAGS_RETURN with additional bits?!\n");
				return; // For now, don't even attempt to run through
			}
			else {
				printf("Pushbuffer COMMAND_FLAGS_RETURN\n");
			}

			if (!subr_active) {
				printf("Pushbuffer COMMAND_FLAGS_RETURN while another call was active!\n");
				// TODO : throw DMA_PUSHER(RET_SUBR_INACTIVE);
				return; // For now, don't even attempt to run through
			}

			dma_get = subr_return;
			subr_active = false;
			continue; // while
		default:
			if (command.flags == COMMAND_FLAGS_SLI_CONDITIONAL) {
				printf("Pushbuffer COMMAND_FLAGS_SLI_CONDITIONAL (NV40+) not available on NV2A\n");;
			}
			else if (command.flags == COMMAND_FLAGS_LONG_NON_INCREASING_METHODS) {
				printf("Pushbuffer COMMAND_FLAGS_LONG_NON_INCREASING_METHODS [IB-mode only] not available on NV2A\n");
				/// No need to do: dma_state.mthd = command.method; dma_state.ni = true;
				/// dma_state.mcnt = *dma_get++ & 0x00FFFFFF; // Long NI method command count is read from low 24 bits of next word
				/// dma_get += dma_state.mcnt; // To be safe, skip method data
			}
			else {
				printf("Pushbuffer COMMAND_FLAGS unknown\n");
			}

			/// dma_get += command.method_count; // To be safe, skip method data
			/// continue;
			// TODO : throw DMA_PUSHER(INVALID_CMD);
			return; // For now, don't even attempt to run through
		} // switch flags

		dcount_shadow = 0;
	}
}
