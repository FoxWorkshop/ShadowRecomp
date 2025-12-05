#include "video.h"

#include "imgui/imgui_common.h"
#include "imgui/imgui_snapshot.h"
#include "imgui/imgui_font_builder.h"

#include "vertexdeclaration.h"
#include "vertexshader.h"
#include "nv2a.h"
#include "swizzle.h"
#include "FixedFunction.h"
#include "renderstate.h"
#include <app.h>
#include <kernel/function.h>
#include <kernel/heap.h>
#include <ui/game_window.h>
#include <user/config.h>
#include "x86_context.h"
#include <sdl_listener.h>
#include <memory>
#include <shader/shader_cache.h>
#include <ui/imgui_utils.h>
#include <xxHashMap.h>
#include <cpu/guest_thread.h>


const uint32_t GuestDevicePtrAddr = 0x3E0A18;
const uint32_t GuestDeviceMemAddr = 0x3E0A20;
const uint32_t PresentMemAddr = 0x55F718;
const uint32_t AutoDepthStencilFormat = 0x57DD68;
const uint32_t RwD3D8RasterExtOffsetAddr = 0x57DD20;
const uint32_t g_DeferredTextureStateAddr = 0x3E0580;

bool LogVSConstants = false;

#define DepthStencilFormat RenderFormat::D32_FLOAT_S8_UINT

#if defined(ASYNC_PSO_DEBUG) || defined(PSO_CACHING)
#include <magic_enum/magic_enum.hpp>
#endif

#ifdef SHADOW_RECOMP_D3D12
#include "shader/FixedFunctionVertexShader.hlsl.dxil.h"
#include "shader/FixedFunctionPixelShader.hlsl.dxil.h"
#include "shader/PassthroughVertexShader.hlsl.dxil.h"

#include "shader/copy_vs.hlsl.dxil.h"
#include "shader/copy_color_ps.hlsl.dxil.h"
#include "shader/copy_depth_ps.hlsl.dxil.h"
#include "shader/enhanced_motion_blur_ps.hlsl.dxil.h"
#include "shader/gamma_correction_ps.hlsl.dxil.h"
#include "shader/gaussian_blur_3x3.hlsl.dxil.h"
#include "shader/gaussian_blur_5x5.hlsl.dxil.h"
#include "shader/gaussian_blur_7x7.hlsl.dxil.h"
#include "shader/gaussian_blur_9x9.hlsl.dxil.h"
#include "shader/imgui_ps.hlsl.dxil.h"
#include "shader/imgui_vs.hlsl.dxil.h"
#include "shader/csd_filter_ps.hlsl.dxil.h"
#include "shader/resolve_msaa_color_2x.hlsl.dxil.h"
#include "shader/resolve_msaa_color_4x.hlsl.dxil.h"
#include "shader/resolve_msaa_color_8x.hlsl.dxil.h"
#include "shader/resolve_msaa_depth_2x.hlsl.dxil.h"
#include "shader/resolve_msaa_depth_4x.hlsl.dxil.h"
#include "shader/resolve_msaa_depth_8x.hlsl.dxil.h"

#include "shader/recomp0.hlsl.dxil.h"
#include "shader/recomp1.hlsl.dxil.h"
#include "shader/recomp2.hlsl.dxil.h"
#include "shader/recomp3.hlsl.dxil.h"
#endif

#include "shader/FixedFunctionVertexShader.hlsl.spirv.h"
#include "shader/PassthroughVertexShader.hlsl.spirv.h"
#include "shader/FixedFunctionPixelShader.hlsl.spirv.h"

#include "shader/copy_vs.hlsl.spirv.h"
#include "shader/copy_color_ps.hlsl.spirv.h"
#include "shader/copy_depth_ps.hlsl.spirv.h"
#include "shader/enhanced_motion_blur_ps.hlsl.spirv.h"
#include "shader/gamma_correction_ps.hlsl.spirv.h"
#include "shader/csd_filter_ps.hlsl.spirv.h"
#include "shader/gaussian_blur_3x3.hlsl.spirv.h"
#include "shader/gaussian_blur_5x5.hlsl.spirv.h"
#include "shader/gaussian_blur_7x7.hlsl.spirv.h"
#include "shader/gaussian_blur_9x9.hlsl.spirv.h"
#include "shader/imgui_ps.hlsl.spirv.h"
#include "shader/imgui_vs.hlsl.spirv.h"
#include "shader/resolve_msaa_color_2x.hlsl.spirv.h"
#include "shader/resolve_msaa_color_4x.hlsl.spirv.h"
#include "shader/resolve_msaa_color_8x.hlsl.spirv.h"
#include "shader/resolve_msaa_depth_2x.hlsl.spirv.h"
#include "shader/resolve_msaa_depth_4x.hlsl.spirv.h"
#include "shader/resolve_msaa_depth_8x.hlsl.spirv.h"

#include "shader/recomp0.hlsl.spirv.h"
#include "shader/recomp1.hlsl.spirv.h"
#include "shader/recomp2.hlsl.spirv.h"
#include "shader/recomp3.hlsl.spirv.h"


namespace plume
{
#ifdef SHADOW_RECOMP_D3D12
	extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
#endif
#ifdef SDL_VULKAN_ENABLED
	extern std::unique_ptr<RenderInterface> CreateVulkanInterface(RenderWindow sdlWindow);
#else
	extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
#endif

	static std::unique_ptr<RenderInterface> CreateVulkanInterfaceWrapper() {
#ifdef SDL_VULKAN_ENABLED
		return CreateVulkanInterface(GameWindow::s_renderWindow);
#else
		return CreateVulkanInterface();
#endif
	}
}



#pragma pack(push, 1)
struct PipelineState
{
	GuestShader* vertexShader = nullptr;
	GuestShader* pixelShader = nullptr;
	//bool isFixedFunctionVS = false;
	bool isFixedFunctionPS = false;
	VertexShaderMode vertexShaderType;
	GuestVertexDeclaration* vertexDeclaration = nullptr;
	bool zEnable = true;
	bool zWriteEnable = true;
	bool stencilEnable = false;
	RenderBlend srcBlend = RenderBlend::ONE;
	RenderBlend destBlend = RenderBlend::ZERO;
	RenderCullMode cullMode = RenderCullMode::NONE;
	RenderComparisonFunction zFunc = RenderComparisonFunction::LESS;
	RenderComparisonFunction stencilFunc = RenderComparisonFunction::ALWAYS;
	RenderStencilOp stencilFail = RenderStencilOp::KEEP; //todo
	RenderStencilOp stencilZFail = RenderStencilOp::KEEP;
	RenderStencilOp stencilPass = RenderStencilOp::KEEP;
	uint32_t stencilMask = 0xFFFFFFFF;
	uint32_t stencilWriteMask = 0xFFFFFFFF;
	uint32_t stencilRef = 0;
	RenderComparisonFunction stencilFuncCCW = RenderComparisonFunction::ALWAYS;
	bool alphaBlendEnable = false;
	RenderBlendOperation blendOp = RenderBlendOperation::ADD;
	float slopeScaledDepthBias = 0.0f;
	int32_t depthBias = 0;
	RenderBlend srcBlendAlpha = RenderBlend::ONE;
	RenderBlend destBlendAlpha = RenderBlend::ZERO;
	RenderBlendOperation blendOpAlpha = RenderBlendOperation::ADD;
	uint32_t colorWriteEnable = uint32_t(RenderColorWriteEnable::ALL);
	RenderPrimitiveTopology primitiveTopology = RenderPrimitiveTopology::TRIANGLE_LIST;
	uint8_t vertexStrides[16]{};
	RenderFormat renderTargetFormat{};
	RenderFormat depthStencilFormat{};
	RenderSampleCounts sampleCount = RenderSampleCount::COUNT_1;
	bool enableAlphaToCoverage = false;
	uint32_t specConstants = 0;
};
#pragma pack(pop)

struct UploadAllocation
{
	const RenderBuffer* buffer;
	uint64_t offset;
	uint8_t* memory;
	uint64_t deviceAddress;
};

static_assert(sizeof(uint32_t) == 4, "expect 4-byte uint");

struct TextureCombinerState {
	uint32_t color_op = 1;
	uint32_t color_arg0 = D3DTA_TEXTURE;
	uint32_t color_arg1 = D3DTA_TEXTURE;
	uint32_t color_arg2 = D3DTA_CURRENT;
	uint32_t alpha_op = 1;
	uint32_t alpha_arg0 = D3DTA_TEXTURE;
	uint32_t alpha_arg1 = D3DTA_TEXTURE;
	uint32_t alpha_arg2 = D3DTA_CURRENT;
	uint32_t texcoord_index = 0;
};

struct SharedConstants
{
	uint32_t texture2DIndices[TexSlotMax];
	uint32_t texture3DIndices[TexSlotMax];
	uint32_t textureCubeIndices[TexSlotMax];
	uint32_t samplerIndices[TexSlotMax];

	//TextureCombinerState combiners[16]; 

	uint32_t color_op[TexSlotMax];
	uint32_t color_arg0[TexSlotMax];
	uint32_t color_arg1[TexSlotMax];
	uint32_t color_arg2[TexSlotMax];

	uint32_t alpha_op[TexSlotMax];
	uint32_t alpha_arg0[TexSlotMax];
	uint32_t alpha_arg1[TexSlotMax];
	uint32_t alpha_arg2[TexSlotMax];
	uint32_t result_arg[TexSlotMax];
	//uint32_t texcoord_index[TexSlotMax];

	uint32_t TextureSampleType[TexSlotMax];


	// TEXTURETRANSFORMFLAGS handled by the VS
	uint32_t BUMPENVMAT00[TexSlotMax];
	uint32_t BUMPENVMAT01[TexSlotMax];
	uint32_t BUMPENVMAT11[TexSlotMax];
	uint32_t BUMPENVMAT10[TexSlotMax];
	uint32_t BUMPENVLSCALE[TexSlotMax];
	uint32_t BUMPENVLOFFSET[TexSlotMax];
	uint32_t TEXCOORDINDEX[TexSlotMax]; 	//handled by the VS

	// BORDERCOLOR set on sampler

	float TextureFactor[4]; //RGBA
	uint32_t SpecularEnable;
	uint32_t FogEnable;
	uint32_t FogTableMode;
	uint32_t FogDensity;
	uint32_t FogStart;
	uint32_t FogEnd;
	float FogColor[3];

	uint32_t alphaTestEnable;
	uint32_t alphaFunc; // D3DCMP_* comparison function
	float alphaRef; // Reference value (0.0 - 1.0)

	uint32_t booleans;
	uint32_t swappedTexcoords;
	float halfPixelOffsetX;
	float halfPixelOffsetY;
	float alphaThreshold;
};




//const UINT ringBufferSize = 4 * 1024 * 1024; // 4 MB ring buffer
const UINT ringBufferSize = 0x10000;
static GuestBuffer* g_ringBuffer;
static GuestSurface* g_renderTarget;
static GuestSurface* g_depthStencil;
static RenderFramebuffer* g_framebuffer;
static RenderViewport g_viewport(0.0f, 0.0f, 1280.0f, 720.0f);
static PipelineState g_pipelineState;
static int32_t g_depthBias;
static float g_slopeScaledDepthBias;
int32_t g_RenderUpscaleFactor = 1;

GuestSurface* GetRenderTarget()
{
	return g_renderTarget;
}

//constants are variable send to the GPU
static uint32_t g_vertexShaderConstants[HOST_D3DVS_CONSTREG_FULL * 4];
static uint32_t g_pixelShaderConstants[0x380];
static SharedConstants g_sharedConstants; //TextureStage operation data, indice etc. Used with texture descriptor
//static uint64_t g_vertexConstantsDirtyFlags = 0;
bool g_vertexConstantsDirtyFlags = false;
std::array<D3DMATRIX, X_D3DTS_MAX> g_Transforms;

uint32_t g_renderState[255];

struct VBData
{
	XD3DResource* guestVBData;
	GuestBuffer* buffer;
	int size;

};

std::vector<VBData> g_VBList;


uint32_t vsh_constants[HOST_D3DVS_CONSTREG_FULL][4]; //xbox registers (192) + host registers (slot of 4 because one register = 4 bytes)
bool vsh_constants_dirty[HOST_D3DVS_CONSTREG_FULL];
constexpr uint32_t kRegsPerBlock = 4;
constexpr uint32_t kFloatsPerReg = 4;
constexpr uint32_t kBytesPerReg = 16;


static GuestTexture* g_textures[TexSlotMax];
static RenderSamplerDesc g_samplerDescs[TexSlotMax];
static TextureCombinerState g_sharedSamplerDescs[TexSlotMax];
static bool g_scissorTestEnable = false;
static RenderRect g_scissorRect;
//static GuestBuffer* g_vertexBuffers[16];

typedef struct _X_STREAMINPUT
{
	GuestBuffer* VertexBuffer;
	UINT                Stride;

} X_STREAMINPUT;

struct VBS
{
	GuestBuffer* bufferPtr;
	int32_t size;

};

std::vector<VBS> VBlist; //todo delete

X_STREAMINPUT g_Xbox_SetStreamSource[X_VSH_MAX_STREAMS];

static RenderVertexBufferView g_vertexBufferViews[16];
static RenderInputSlot g_inputSlots[16];
static RenderIndexBufferView g_indexBufferView({}, 0, RenderFormat::R16_UINT);


int32_t* g_texture_stage_states = nullptr;
int32_t g_dirty_texture_stage_states[4 * X_D3DTSS_MAX]; //128 because 4 stages * 32 texture stage state possible

struct DirtyStates
{
	bool renderTargetAndDepthStencil;
	bool viewport;
	bool pipelineState;
	bool depthBias;
	bool sharedConstants;
	bool scissorRect;
	bool vertexShaderConstants;
	uint8_t vertexStreamFirst;
	uint8_t vertexStreamLast;
	bool indices;
	bool pixelShaderConstants;
	bool textureStageState;
	bool vertexBufferLock;

	DirtyStates(bool value)
		: renderTargetAndDepthStencil(value)
		, viewport(value)
		, pipelineState(value)
		, depthBias(value)
		, sharedConstants(value)
		, scissorRect(value)
		, vertexShaderConstants(value)
		, vertexStreamFirst(value ? 0 : 255)
		, vertexStreamLast(value ? 15 : 0)
		, indices(value)
		, pixelShaderConstants(value)
		, vertexBufferLock(value)
	{
	}
};

static DirtyStates g_dirtyStates(true);

template<typename T>
static void SetDirtyValue(bool& dirtyState, T& dest, const T& src)
{
	if (dest != src)
	{
		dest = src;
		dirtyState = true;
	}
}



static constexpr size_t PROFILER_VALUE_COUNT = 256;
static size_t g_profilerValueIndex;

struct Profiler
{
	std::atomic<double> value;
	double values[PROFILER_VALUE_COUNT];
	std::chrono::steady_clock::time_point start;

	void Begin()
	{
		start = std::chrono::steady_clock::now();
	}

	void End()
	{
		value = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
	}

	void Set(double v)
	{
		value = v;
	}

	void Reset()
	{
		End();
		Begin();
	}

	double UpdateAndReturnAverage()
	{
		values[g_profilerValueIndex] = value;
		return std::accumulate(values, values + PROFILER_VALUE_COUNT, 0.0) / PROFILER_VALUE_COUNT;
	}
};

static double g_applicationValues[PROFILER_VALUE_COUNT];
static Profiler g_gpuFrameProfiler;
static Profiler g_presentProfiler;
static Profiler g_updateDirectorProfiler;
static Profiler g_renderDirectorProfiler;
static Profiler g_frameFenceProfiler;
static Profiler g_presentWaitProfiler;
static Profiler g_swapChainAcquireProfiler;

static bool g_profilerVisible;
static bool g_profilerWasToggled;

#ifdef SHADOW_RECOMP_D3D12
static bool g_vulkan = false;
#else
static constexpr bool g_vulkan = true;
#endif

static bool g_hardwareResolve = true;
static bool g_hardwareDepthResolve = true;

static std::unique_ptr<RenderInterface> g_interface;
static std::unique_ptr<RenderDevice> g_device;


static RenderDeviceCapabilities g_capabilities;

static constexpr size_t NUM_FRAMES = 2;
static constexpr size_t NUM_QUERIES = 2;

static uint32_t g_frame = 0;
static uint32_t g_nextFrame = 1;

static std::unique_ptr<RenderCommandQueue> g_queue;
static std::unique_ptr<RenderCommandList> g_commandLists[NUM_FRAMES];
static std::unique_ptr<RenderCommandFence> g_commandFences[NUM_FRAMES];
static std::unique_ptr<RenderQueryPool> g_queryPools[NUM_FRAMES];
static bool g_commandListStates[NUM_FRAMES];

static Mutex g_copyMutex;
static std::unique_ptr<RenderCommandQueue> g_copyQueue;
static std::unique_ptr<RenderCommandList> g_copyCommandList;
static std::unique_ptr<RenderCommandFence> g_copyCommandFence;


static std::unique_ptr<RenderSwapChain> g_swapChain;
static bool g_swapChainValid;

static constexpr RenderFormat BACKBUFFER_FORMAT = RenderFormat::B8G8R8A8_UNORM;

static std::unique_ptr<RenderCommandSemaphore> g_acquireSemaphores[NUM_FRAMES];
static std::unique_ptr<RenderCommandSemaphore> g_renderSemaphores[NUM_FRAMES];
static uint32_t g_backBufferIndex;
static std::unique_ptr<GuestSurface> g_backBufferHolder;
static GuestSurface* g_backBuffer;

static std::unique_ptr<RenderTexture> g_intermediaryBackBufferTexture;
static uint32_t g_intermediaryBackBufferTextureWidth;
static uint32_t g_intermediaryBackBufferTextureHeight;
static uint32_t g_intermediaryBackBufferTextureDescriptorIndex;

struct std::unique_ptr<RenderDescriptorSet> g_textureDescriptorSet;
struct std::unique_ptr<RenderDescriptorSet> g_samplerDescriptorSet;

static xxHashMap<std::pair<uint32_t, std::unique_ptr<RenderSampler>>> g_samplerStates;

static xxHashMap<GuestBuffer*> g_vertexBufferCache;
static xxHashMap<GuestBuffer*> g_indexBufferCache;

struct UploadBuffer
{
	static constexpr size_t SIZE = 16 * 1024 * 1024;

	std::unique_ptr<RenderBuffer> buffer;
	uint8_t* memory = nullptr;
	uint64_t deviceAddress = 0;
};

struct UploadAllocator
{
	std::vector<UploadBuffer> buffers;
	uint32_t index = 0;
	uint32_t offset = 0;

	UploadAllocation allocate(uint32_t size, uint32_t alignment)
	{
		assert(size <= UploadBuffer::SIZE);

		offset = (offset + alignment - 1) & ~(alignment - 1);

		if (offset + size > UploadBuffer::SIZE)
		{
			++index;
			offset = 0;
		}

		if (buffers.size() <= index)
			buffers.resize(index + 1);

		auto& buffer = buffers[index];
		if (buffer.buffer == nullptr)
		{
			buffer.buffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(UploadBuffer::SIZE, RenderBufferFlag::CONSTANT | RenderBufferFlag::VERTEX | RenderBufferFlag::INDEX | RenderBufferFlag::DEVICE_ADDRESSABLE));
			buffer.memory = reinterpret_cast<uint8_t*>(buffer.buffer->map());
			buffer.deviceAddress = buffer.buffer->getDeviceAddress();
		}

		auto ref = buffer.buffer->at(offset);
		offset += size;

		return { ref.ref, ref.offset, buffer.memory + ref.offset, buffer.deviceAddress + ref.offset };
	}

	template<bool TByteSwap, typename T>
	UploadAllocation allocate(const T* memory, uint32_t size, uint32_t alignment)
	{
		auto result = allocate(size, alignment);

		if constexpr (TByteSwap)
		{
			auto destination = reinterpret_cast<T*>(result.memory);

			for (size_t i = 0; i < size; i += sizeof(T))
			{
				*destination = *memory;
				++destination;
				++memory;
			}
		}
		else
		{
			memcpy(result.memory, memory, size);
		}

		return result;
	}

	void reset()
	{
		index = 0;
		offset = 0;
	}
};

static UploadAllocator g_uploadAllocators[NUM_FRAMES];

struct IntermediaryUploadAllocator
{
	static constexpr size_t SIZE = 16 * 1024 * 1024;

	std::vector<std::unique_ptr<uint8_t[]>> buffers;
	uint32_t index = 0;
	uint32_t offset = 0;

	uint8_t* allocate(uint32_t size)
	{
		assert(size <= SIZE);

		if (offset + size > SIZE)
		{
			++index;
			offset = 0;
		}

		if (buffers.size() <= index)
			buffers.resize(index + 1);

		auto& buffer = buffers[index];
		if (buffer == nullptr)
			buffer = std::make_unique_for_overwrite<uint8_t[]>(SIZE);

		auto result = buffer.get() + offset;
		offset += ((size + 0xF) & ~0xF);

		return result;
	}

	uint8_t* allocate(const void* memory, uint32_t size)
	{
		auto result = allocate(size);
		memcpy(result, memory, size);
		return result;
	}

	void reset()
	{
		index = 0;
		offset = 0;
	}
};

static IntermediaryUploadAllocator g_intermediaryUploadAllocator;

static std::vector<GuestResource*> g_tempResources[NUM_FRAMES];
static std::vector<std::unique_ptr<RenderBuffer>> g_tempBuffers[NUM_FRAMES];


float GetRenderTargetHeight()
{
	if (g_renderTarget->height)
		return g_renderTarget->height;

	printf("ERROR no render target found\n");
	return 0.0f;
}

static std::thread::id g_presentThreadId = std::this_thread::get_id();
static std::atomic<bool> g_readyForCommands;

static ankerl::unordered_dense::map<RenderTexture*, RenderTextureLayout> g_barrierMap;

static void AddBarrier(GuestBaseTexture* texture, RenderTextureLayout layout)
{
	if (texture != nullptr && texture->layout != layout)
	{
		g_barrierMap[texture->texture] = layout;
		texture->layout = layout;
	}
}

static std::vector<RenderTextureBarrier> g_barriers;

static void FlushBarriers()
{
	if (!g_barrierMap.empty())
	{
		for (auto& [texture, layout] : g_barrierMap)
			g_barriers.emplace_back(texture, layout);

		g_commandLists[g_frame]->barriers(RenderBarrierStage::GRAPHICS | RenderBarrierStage::COPY, g_barriers);

		g_barrierMap.clear();
		g_barriers.clear();
	}
}

static std::unique_ptr<uint8_t[]> g_shaderCache;

static std::unique_ptr<RenderPipeline> g_gammaCorrectionPipeline;

enum
{
	TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D,
	TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D,
	TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE,
	TEXTURE_DESCRIPTOR_NULL_COUNT
};

struct TextureDescriptorAllocator
{
	Mutex mutex;
	uint32_t capacity = TEXTURE_DESCRIPTOR_NULL_COUNT;
	std::vector<uint32_t> freed;

	uint32_t allocate()
	{
		std::lock_guard lock(mutex);

		uint32_t value;
		if (!freed.empty())
		{
			value = freed.back();
			freed.pop_back();
		}
		else
		{
			value = capacity;
			++capacity;
		}

		return value;
	}

	void free(uint32_t value)
	{
		assert(value != NULL);
		std::lock_guard lock(mutex);
		freed.push_back(value);
	}
};

static std::unique_ptr<RenderTexture> g_blankTextures[TEXTURE_DESCRIPTOR_NULL_COUNT];
static std::unique_ptr<RenderTextureView> g_blankTextureViews[TEXTURE_DESCRIPTOR_NULL_COUNT];

static TextureDescriptorAllocator g_textureDescriptorAllocator;

static std::unique_ptr<RenderPipelineLayout> g_pipelineLayout;
static xxHashMap<std::unique_ptr<RenderPipeline>> g_pipelines;

GuestSurface* g_XboxBackBuffer = nullptr;
GuestSurface* g_XboxDefaultDepthStencilSurface = nullptr;
//todo figure out this
static void DestructTempResources()
{
	bool valid = false;
	for (auto resource : g_tempResources[g_frame])
	{

		if (resource->type == ResourceType::VertexBuffer)
		{
			valid = true;
			const auto buffer = reinterpret_cast<GuestBuffer*>(resource);

			if (buffer->mappedMemory != nullptr)
				g_userHeap.Free(buffer->mappedMemory);

			static int count;
			buffer->~GuestBuffer();
		}

		/**switch (resource->type)
		{
		case ResourceType::Texture:
		case ResourceType::VolumeTexture:
		{
			/**const auto texture = reinterpret_cast<GuestTexture*>(resource);

			if (texture->mappedMemory != nullptr)
				g_userHeap.Free(texture->mappedMemory);

			g_textureDescriptorAllocator.free(texture->descriptorIndex);

			//if (texture->patchedTexture != nullptr)
				//g_textureDescriptorAllocator.free(texture->patchedTexture->descriptorIndex);

			texture->~GuestTexture();
			break;
		}

		case ResourceType::VertexBuffer:
		case ResourceType::IndexBuffer:
		{
			const auto buffer = reinterpret_cast<GuestBuffer*>(resource);

			if (buffer->mappedMemory != nullptr)
				g_userHeap.Free(buffer->mappedMemory);

			buffer->~GuestBuffer();
			break;
		}

		case ResourceType::RenderTarget:
		case ResourceType::DepthStencil:
		{
			valid = false;
			/**const auto surface = reinterpret_cast<GuestSurface*>(resource);

			if (g_XboxBackBuffer != surface && g_XboxDefaultDepthStencilSurface != surface) //do not destroy main render target (todo check if it's correct)
			{
				if (surface->descriptorIndex != NULL)
					g_textureDescriptorAllocator.free(surface->descriptorIndex);

				surface->~GuestSurface();
			}

			break;
		}

		case ResourceType::VertexDeclaration:
			reinterpret_cast<GuestVertexDeclaration*>(resource)->~GuestVertexDeclaration();
			break;

		case ResourceType::VertexShader:
		case ResourceType::PixelShader:
		{
			reinterpret_cast<GuestShader*>(resource)->~GuestShader();
			break;
		}
		}*/

		if (valid)
			g_userHeap.Free(resource);

		valid = false;
	}

	//if (valid)
	{
		g_tempResources[g_frame].clear();
		g_tempBuffers[g_frame].clear();
	}
}

static ankerl::unordered_dense::set<GuestSurface*> g_pendingSurfaceCopies;
static ankerl::unordered_dense::set<GuestSurface*> g_pendingMsaaResolves;

enum class RenderCommandType
{
	SetRenderState,
	DestructResource,
	UnlockTextureRect,
	UnlockBuffer16,
	UnlockBuffer32,
	DrawImGui,
	ExecuteCommandList,
	BeginCommandList,
	StretchRect,
	SetRenderTarget,
	SetDepthStencilSurface,
	ExecutePendingCopyRectsCommands,
	Clear,
	SetViewport,
	SetTexture,
	//SetScissorRect,
	SetTextureStageState,
	SetBooleans,
	SetVertexShaderConstants,
	SetPixelShaderConstants,
	AddPipeline,
	DrawPrimitive,
	DrawIndexedVertices,
	DrawVerticesUP,
	SetVertexDeclaration,
	SetVertexShader,
	SetStreamSource,
	SetIndices,
	SetPixelShader,
	SetTransform,

};

struct RenderCommand
{
	RenderCommandType type;

	union
	{
		struct
		{
			GuestRenderState type;
			uint32_t value;
		} setRenderState;

		struct
		{
			GuestResource* resource;
		} destructResource;

		struct
		{
			GuestTexture* texture;
		} unlockTextureRect;

		struct
		{
			GuestBuffer* indexBuffer;
		} unlockBuffer;

		struct
		{
			GuestBuffer* indexBuffer;
		} setIndices;

		struct
		{
			GuestSurface* renderTarget;
		} setRenderTarget;

		struct
		{
			GuestSurface* depthStencil;
		} setDepthStencilSurface;


		struct
		{
			uint32_t flags;
			float color[4];
			float z;
		} clear;


		struct
		{
			float x;
			float y;
			float width;
			float height;
			float minDepth;
			float maxDepth;
		} setViewport;

		struct
		{
			uint32_t index;
			GuestTexture* texture;
		} setTexture;

		struct
		{
			uint32_t primitiveType;
			uint32_t startVertex;
			uint32_t primitiveCount;
		} drawPrimitive;

		struct
		{
			uint32_t primitiveType;
			int32_t baseVertexIndex;
			uint32_t startIndex;
			uint32_t primCount;
			int16_t* pIndexData;
			GuestBuffer* indexBuffer;
			//GuestBuffer* vertexBuffer;
		} drawIndexedPrimitive;

		struct
		{
			uint32_t primitiveType;
			uint32_t primitiveCount;
			uint8_t* vertexStreamZeroData;
			uint32_t vertexStreamZeroSize;
			uint32_t vertexStreamZeroStride;
		} drawPrimitiveUP;


		struct
		{
			GuestVertexDeclaration* vertexDeclaration;
		} setVertexDeclaration;

		struct
		{
			GuestShader* shader;
			uint32_t handle;
			VertexShaderMode type;
		} setVertexShader;

		struct
		{
			uint32_t StreamNumber;
			GuestBuffer* vertexBuffer;
			uint32_t stride;
		} setStreamSource;


		struct
		{
			GuestShader* shaderHandle;
			uint32_t handle;
			bool isFixed;
		} setPixelShader;


		struct
		{
			X_D3DTRANSFORMSTATETYPE state;
			D3DMATRIX matrix;
		} setTransform;

		struct
		{
			uint32_t D3DTSS_ADDRESSU;
			uint32_t D3DTSS_ADDRESSV;
			uint32_t D3DTSS_ADDRESSW;
			uint32_t D3DTSS_MAGFILTER;
			uint32_t D3DTSS_MINFILTER;
			uint32_t D3DTSS_MIPFILTER;
			uint32_t D3DTSS_BORDERCOLOR;
			//shared for pixel shaders
			uint32_t D3DTSS_COLOROP;
			uint32_t D3DTSS_COLORARG0;
			uint32_t D3DTSS_COLORARG1;
			uint32_t D3DTSS_COLORARG2;
			uint32_t D3DTSS_ALPHAOP;
			uint32_t D3DTSS_ALPHAARG0;
			uint32_t D3DTSS_ALPHAARG1;
			uint32_t D3DTSS_ALPHAARG2;
			uint32_t D3DTSS_RESULTARG;
			uint32_t D3DTSS_BUMPENVMAT00;
			uint32_t D3DTSS_BUMPENVMAT01;
			uint32_t D3DTSS_BUMPENVMAT11;
			uint32_t D3DTSS_BUMPENVMAT10;
			uint32_t D3DTSS_BUMPENVLSCALE;
			uint32_t D3DTSS_BUMPENVLOFFSET;
			uint32_t D3DTSS_TEXCOORDINDEX;
			uint32_t index;
		} SetTextureStageState;

		struct
		{
			uint8_t* memory;
			uint32_t index;
			uint32_t size;
		} setVertexShaderConstants;

		struct
		{
			uint8_t* memory;
			uint32_t index;
			uint32_t size;
		} setPixelShaderConstants;
	};


};

static RenderBlend ConvertBlendMode(uint32_t blendMode)
{
	switch (blendMode)
	{
	case D3DBLEND_ZERO:
		return RenderBlend::ZERO;
	case D3DBLEND_ONE:
		return RenderBlend::ONE;
	case D3DBLEND_SRCCOLOR:
		return RenderBlend::SRC_COLOR;
	case D3DBLEND_INVSRCCOLOR:
		return RenderBlend::INV_SRC_COLOR;
	case D3DBLEND_SRCALPHA:
		return RenderBlend::SRC_ALPHA;
	case D3DBLEND_INVSRCALPHA:
		return RenderBlend::INV_SRC_ALPHA;
	case D3DBLEND_DESTCOLOR:
		return RenderBlend::DEST_COLOR;
	case D3DBLEND_INVDESTCOLOR:
		return RenderBlend::INV_DEST_COLOR;
	case D3DBLEND_DESTALPHA:
		return RenderBlend::DEST_ALPHA;
	case D3DBLEND_INVDESTALPHA:
		return RenderBlend::INV_DEST_ALPHA;
	default:
		assert(false && "Invalid blend mode");
		return RenderBlend::ZERO;
	}
}

static RenderBlendOperation ConvertBlendOp(uint32_t blendOp)
{
	switch (blendOp)
	{
	case D3DBLENDOP_ADD:
		return RenderBlendOperation::ADD;
	case D3DBLENDOP_SUBTRACT:
		return RenderBlendOperation::SUBTRACT;
	case D3DBLENDOP_REVSUBTRACT:
		return RenderBlendOperation::REV_SUBTRACT;
	case D3DBLENDOP_MIN:
		return RenderBlendOperation::MIN;
	case D3DBLENDOP_MAX:
		return RenderBlendOperation::MAX;
	default:
		assert(false && "Unknown blend operation");
		return RenderBlendOperation::ADD;
	}
}

static RenderComparisonFunction ConvertD3DCMP(uint32_t value)
{
	RenderComparisonFunction comparisonFunc;

	switch (value)
	{
	case D3DCMP_NEVER:
		comparisonFunc = RenderComparisonFunction::NEVER;
		break;
	case D3DCMP_LESS:
		comparisonFunc = RenderComparisonFunction::LESS;
		break;
	case D3DCMP_EQUAL:
		comparisonFunc = RenderComparisonFunction::EQUAL;
		break;
	case D3DCMP_LESSEQUAL:
		comparisonFunc = RenderComparisonFunction::LESS_EQUAL;
		break;
	case D3DCMP_GREATER:
		comparisonFunc = RenderComparisonFunction::GREATER;
		break;
	case D3DCMP_NOTEQUAL:
		comparisonFunc = RenderComparisonFunction::NOT_EQUAL;
		break;
	case D3DCMP_GREATEREQUAL:
		comparisonFunc = RenderComparisonFunction::GREATER_EQUAL;
		break;
	case D3DCMP_ALWAYS:
		comparisonFunc = RenderComparisonFunction::ALWAYS;
		break;
	default:
		assert(false && "Unknown comparison function");
		comparisonFunc = RenderComparisonFunction::NEVER;
		break;
	}

	return comparisonFunc;
}


static RenderStencilOp ConvertStencilOp(uint32_t stencilOp)
{
	switch (stencilOp)
	{
	case D3DSTENCILOP_KEEP:
		return RenderStencilOp::KEEP;
	case D3DSTENCILOP_ZERO:
		return RenderStencilOp::ZERO;
	case D3DSTENCILOP_REPLACE:
		return RenderStencilOp::REPLACE;
	case D3DSTENCILOP_INCRSAT:
		return RenderStencilOp::INCREMENT_AND_CLAMP;
	case D3DSTENCILOP_DECRSAT:
		return RenderStencilOp::DECREMENT_AND_CLAMP;
	case D3DSTENCILOP_INVERT:
		return RenderStencilOp::INVERT;
	case D3DSTENCILOP_INCR:
		return RenderStencilOp::INCREMENT_AND_WRAP;
	case D3DSTENCILOP_DECR:
		return RenderStencilOp::DECREMENT_AND_WRAP;
	default:
		assert(false && "Unknown stencil op");
		return RenderStencilOp::KEEP;
	}
}


//todo implement AlphaToCoverage once MSAA is a thing, until then it's worthless.
#define SPEC_CONSTANT_ALPHA_TEST        (1 << 2)
#define SPEC_CONSTANT_ALPHA_TO_COVERAGE (1 << 3)
static void SetAlphaTestMode(bool enable)
{
	uint32_t specConstants = 0;
	bool enableAlphaToCoverage = false;

	if (enable)
	{
		enableAlphaToCoverage = false; // Config::TransparencyAntiAliasing&& g_renderTarget != nullptr && g_renderTarget->sampleCount != RenderSampleCount::COUNT_1;

		/**if (enableAlphaToCoverage)
			specConstants = SPEC_CONSTANT_ALPHA_TO_COVERAGE;
		else*/

		specConstants = SPEC_CONSTANT_ALPHA_TEST;
	}

	specConstants |= (g_pipelineState.specConstants & ~(SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE));

	SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.alphaTestEnable, uint32_t(enable != 0));
	//SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.enableAlphaToCoverage, enableAlphaToCoverage);
	//SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);
}

static moodycamel::BlockingConcurrentQueue<RenderCommand> g_renderQueue;


//template<GuestRenderState TType>
//specify the "config" of the rendering, essentially prepare the GPU to how it should render. (alpha, blending, culling etc.)
static void SetRenderState(X_D3DRENDERSTATETYPE state, uint32_t value)
{

	RenderCommand cmd;
	cmd.type = RenderCommandType::SetRenderState;
	cmd.setRenderState.type = state;
	cmd.setRenderState.value = value;
	g_renderQueue.enqueue(cmd);
}


static void D3DDevice_SetRenderState_Simple(int32_t Method, int32_t Value)
{
	//Fetch the RenderState conversion info for the given input
	int32_t XboxRenderStateIndex = -1;
	int32_t PCValue = -1;
	for (int32_t i = X_D3DRS_PS_FIRST; i <= X_D3DRS_LAST; i++)
	{
		if (GetDxbxRenderStateInfo(i).M == PUSH_METHOD(Method))
		{
			XboxRenderStateIndex = i;
			break;
		}
	}

	if (XboxRenderStateIndex == -1) {
		printf("RenderState_Simple could not be found in RenderState table Method %x, Value %x\n", Method, Value);
		return;
	}

	SetRenderState((X_D3DRENDERSTATETYPE)XboxRenderStateIndex, Value);
}


X86_FUNC_IMPL(__imp__sub_3D2450);
int32_t D3DDevice_SetRenderStateNotInline(X_D3DRENDERSTATETYPE State, int32_t Value)
{
	if (State >= X_D3DRS_FOGENABLE)
	{
		SetRenderState(State, Value);
	}
	else //let original code run, this will call the Simple version which is handled as well.
	{
		auto ctx = GetX86Context();
		auto base = g_memory.base;
		__imp__sub_3D2450(*ctx, base);
		ctx->esp.u32 -= 12; //readjust esp since the hook will re apply it.

	}

	return 0;
}

static void SetRenderStateUnimplemented(GuestDevice* device, uint32_t value)
{

}

static void ProcSetRenderState(const RenderCommand& cmd)
{
	uint32_t value = cmd.setRenderState.value;

	switch (cmd.setRenderState.type)
	{
	case X_D3DRS_PSTEXTUREMODES:
	{

		break;
	}
	case X_D3DRS_VERTEXBLEND:
	{

		break;
	}
	case X_D3DRS_FOGCOLOR:
	{

		break;
	}
	case X_D3DRS_FILLMODE:
	{

		break;
	}
	case X_D3DRS_BACKFILLMODE:
	{

		break;
	}
	case X_D3DRS_TWOSIDEDLIGHTING:
	{

		break;
	}
	case X_D3DRS_NORMALIZENORMALS:
	{

		break;
	}
	case X_D3DRS_ZENABLE:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable, value != 0);
		g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
		break;
	}
	case X_D3DRS_STENCILENABLE:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilEnable, value != 0);
		g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
		break;
	}
	case X_D3DRS_STENCILFAIL:
	{

		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zEnable, value != 0);
		break;
	}
	case X_D3DRS_CULLMODE:
	{
		RenderCullMode cullMode;

		switch (value) {
		case X_D3DCULL_NONE:
			cullMode = RenderCullMode::NONE;
			break;
		case X_D3DCULL_CW:
			cullMode = RenderCullMode::FRONT;
			break;
		case X_D3DCULL_CCW:
			cullMode = RenderCullMode::BACK;
			break;
		default:
			assert(false && "Invalid cull mode");
			cullMode = RenderCullMode::NONE;
			break;
		}

		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.cullMode, cullMode);
		break;
	}
	case X_D3DRS_FRONTFACE:
	{

		break;
	}
	case X_D3DRS_TEXTUREFACTOR:
	{
		//SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.TextureFactor, cullMode);
		break;
	}
	case X_D3DRS_ZBIAS:
	{
		break;
	}
	case X_D3DRS_LOGICOP:
	{
		break;
	}
	case X_D3DRS_EDGEANTIALIAS:
	{
		break;
	}
	case X_D3DRS_MULTISAMPLEANTIALIAS:
	{
		break;
	}
	case X_D3DRS_MULTISAMPLEMASK:
	{
		break;
	}
	case X_D3DRS_MULTISAMPLEMODE:
	{
		break;
	}
	case X_D3DRS_MULTISAMPLERENDERTARGETMODE:
	{

		break;
	}
	case X_D3DRS_SHADOWFUNC:
	{

		break;
	}
	case X_D3DRS_LINEWIDTH:
	{

		break;
	}
	case X_D3DRS_SAMPLEALPHA:
	{

		break;
	}
	case X_D3DRS_DXT1NOISEENABLE:
	{
		//SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, value != 0);
		break;
	}
	case X_D3DRS_YUVENABLE:
	{
		//SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, value != 0);
		break;
	}
	case X_D3DRS_OCCLUSIONCULLENABLE:
	{
		//SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, value != 0);
		break;
	}
	case X_D3DRS_STENCILCULLENABLE:
	{
		//SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, value != 0);
		break;
	}

	case X_D3DRS_SRCBLEND:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlend, ConvertBlendMode(value));
		break;
	}
	case X_D3DRS_DESTBLEND:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlend, ConvertBlendMode(value));
		break;
	}
	case X_D3DRS_ZFUNC:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zFunc, ConvertD3DCMP(value));
		break;
	}
	case X_D3DRS_STENCILFUNC:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFunc, ConvertD3DCMP(value));
		break;
	}
	case D3DRS_STENCILFAIL:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFail, ConvertStencilOp(value));
		break;
	}
	case D3DRS_STENCILZFAIL:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilZFail, ConvertStencilOp(value));
		break;
	}
	case D3DRS_STENCILPASS:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilPass, ConvertStencilOp(value));
		break;
	}
	case X_D3DRS_ZWRITEENABLE:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.zWriteEnable, value != 0);
		break;
	}
	case X_D3DRS_ALPHAFUNC:
	{
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.alphaFunc, (uint32_t)ConvertD3DCMP(value));
		break;
	}
	case X_D3DRS_ALPHATESTENABLE:
	{
		SetAlphaTestMode(value != 0);
		break;
	}
	case X_D3DRS_ALPHAREF:
	{
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.alphaRef, float(value) / 255.0f);
		break;
	}
	case X_D3DRS_ALPHABLENDENABLE:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.alphaBlendEnable, value != 0);
		break;
	}
	case X_D3DRS_BLENDOP:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.blendOp, ConvertBlendOp(value));
		break;
	}
	case X_D3DRS_COLORWRITEENABLE:
	{
		DWORD OrigValue = value;
		value = 0;

		if (OrigValue & (1L << 16))
			value |= D3DCOLORWRITEENABLE_RED;


		if (OrigValue & (1L << 8))
			value |= D3DCOLORWRITEENABLE_GREEN;


		if (OrigValue & (1L << 0))
			value |= D3DCOLORWRITEENABLE_BLUE;


		if (OrigValue & (1L << 24))
			value |= D3DCOLORWRITEENABLE_ALPHA;


		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.colorWriteEnable, value);
		g_dirtyStates.renderTargetAndDepthStencil |= g_dirtyStates.pipelineState;
		break;
	}
	//fog is handled in shader constant directly
	case X_D3DRS_FOGTABLEMODE:
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogTableMode, value);
		break;
	case X_D3DRS_FOGDENSITY:
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogDensity, value);
		break;
	case X_D3DRS_FOGSTART:
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogStart, value);
		break;
	case X_D3DRS_FOGEND:
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogEnd, value);
		break;
	case X_D3DRS_FOGENABLE:
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogEnable, value);
		break;
	case X_D3DRS_SPECULARENABLE:
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.SpecularEnable, value);
		break;
	}
	/**case D3DRS_SCISSORTESTENABLE:
	{
		SetDirtyValue(g_dirtyStates.scissorRect, g_scissorTestEnable, value != 0);
		break;
	}
	case X_D3DRS_SLOPESCALEDEPTHBIAS:
	{
		if (g_capabilities.dynamicDepthBias)
			SetDirtyValue(g_dirtyStates.depthBias, g_slopeScaledDepthBias, *reinterpret_cast<float*>(&value));
		else
			SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.slopeScaledDepthBias, *reinterpret_cast<float*>(&value));

		break;
	}
	case X_D3DRS_DEPTHBIAS:
	{
		if (g_capabilities.dynamicDepthBias)
			SetDirtyValue(g_dirtyStates.depthBias, g_depthBias, int32_t(*reinterpret_cast<float*>(&value) * (1 << 24)));
		else
			SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias, int32_t(*reinterpret_cast<float*>(&value) * (1 << 24)));

		break;
	}*/

	g_renderState[cmd.setRenderState.type] = value;
}

static std::unique_ptr<RenderShader> g_copyShader;

static std::unique_ptr<RenderShader> g_copyColorShader;
static ankerl::unordered_dense::map<RenderFormat, std::unique_ptr<RenderPipeline>> g_copyColorPipelines;
static std::unique_ptr<RenderPipeline> g_copyDepthPipeline;

static std::unique_ptr<RenderShader> g_resolveMsaaColorShaders[3];
static ankerl::unordered_dense::map<RenderFormat, std::array<std::unique_ptr<RenderPipeline>, 3>> g_resolveMsaaColorPipelines;
static std::unique_ptr<RenderPipeline> g_resolveMsaaDepthPipelines[3];

static std::unique_ptr<RenderPipeline> g_renderTest;

static std::unique_ptr<RenderShader> FixedFuncVS = nullptr;
static std::unique_ptr<RenderShader> PassthroughVS = nullptr; //this is for vertex shader that don't have a program, similar to fixed, but vertex are already transformed.
static std::unique_ptr<RenderShader> FixedFuncPS = nullptr;

enum
{
	GAUSSIAN_BLUR_3X3,
	GAUSSIAN_BLUR_5X5,
	GAUSSIAN_BLUR_7X7,
	GAUSSIAN_BLUR_9X9,
	GAUSSIAN_BLUR_COUNT
};


static std::unique_ptr<GuestShader> g_gaussianBlurShaders[GAUSSIAN_BLUR_COUNT];

static std::unique_ptr<GuestShader> g_csdFilterShader;
static GuestShader* g_csdShader;

static std::unique_ptr<GuestShader> g_enhancedMotionBlurShader;


#ifdef SHADOW_RECOMP_D3D12

#define CREATE_SHADER(NAME) \
    g_device->createShader( \
        g_vulkan ? g_##NAME##_spirv : g_##NAME##_dxil, \
        g_vulkan ? sizeof(g_##NAME##_spirv) : sizeof(g_##NAME##_dxil), \
        "main", \
        g_vulkan ? RenderShaderFormat::SPIRV : RenderShaderFormat::DXIL)

#else

#define CREATE_SHADER(NAME) \
    g_device->createShader(g_##NAME##_spirv, sizeof(g_##NAME##_spirv), "main", RenderShaderFormat::SPIRV)

#endif

#ifdef _WIN32
static bool DetectWine()
{
	HMODULE dllHandle = GetModuleHandle("ntdll.dll");
	return dllHandle != nullptr && GetProcAddress(dllHandle, "wine_get_version") != nullptr;
}
#endif

static constexpr size_t TEXTURE_DESCRIPTOR_SIZE = 65536;
static constexpr size_t SAMPLER_DESCRIPTOR_SIZE = 1024;

static std::unique_ptr<GuestTexture> g_imFontTexture;
static std::unique_ptr<RenderPipelineLayout> g_imPipelineLayout;
static std::unique_ptr<RenderPipeline> g_imPipeline;
static std::unique_ptr<RenderPipeline> g_imAdditivePipeline;

template<typename T>
static void ExecuteCopyCommandList(const T& function)
{
	std::lock_guard lock(g_copyMutex);

	g_copyCommandList->begin();
	function();
	g_copyCommandList->end();
	g_copyQueue->executeCommandLists(g_copyCommandList.get(), g_copyCommandFence.get());
	g_copyQueue->waitForCommandFence(g_copyCommandFence.get());
}


static constexpr uint32_t PITCH_ALIGNMENT = 0x100;
static constexpr uint32_t PLACEMENT_ALIGNMENT = 0x200;

struct ImGuiPushConstants
{
	ImVec2 boundsMin{};
	ImVec2 boundsMax{};
	ImU32 gradientTopLeft{};
	ImU32 gradientTopRight{};
	ImU32 gradientBottomRight{};
	ImU32 gradientBottomLeft{};
	uint32_t shaderModifier{};
	uint32_t texture2DDescriptorIndex{};
	ImVec2 displaySize{};
	ImVec2 inverseDisplaySize{};
	ImVec2 origin{ 0.0f, 0.0f };
	ImVec2 scale{ 1.0f, 1.0f };
	ImVec2 proceduralOrigin{ 0.0f, 0.0f };
	float outline{};
};

extern ImFontBuilderIO g_fontBuilderIO;

static void CreateImGuiBackend()
{
	printf("Init ImGui, please wait...\n");
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	/**#ifdef ENABLE_IM_FONT_ATLAS_SNAPSHOT
		IM_DELETE(io.Fonts);
		io.Fonts = ImFontAtlasSnapshot::Load();
	#else*/
	io.Fonts->AddFontDefault();
	ImFontAtlasSnapshot::GenerateGlyphRanges();
	//#endif

	InitImGuiUtils();
	/**AchievementMenu::Init();
	AchievementOverlay::Init();
	ButtonGuide::Init();
	MessageWindow::Init();
	OptionsMenu::Init();
	InstallerWizard::Init();*/

	ImGui_ImplSDL2_InitForOther(GameWindow::s_pWindow);


	/**#ifdef ENABLE_IM_FONT_ATLAS_SNAPSHOT //todo
		g_imFontTexture = LoadTexture(
			decompressZstd(g_im_font_atlas_texture, g_im_font_atlas_texture_uncompressed_size).get(), g_im_font_atlas_texture_uncompressed_size);
	#else*/
	io.Fonts->FontBuilderIO = &g_fontBuilderIO;
	io.Fonts->Build();

	g_imFontTexture = std::make_unique<GuestTexture>(ResourceType::Texture);

	uint8_t* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	RenderTextureDesc textureDesc;
	textureDesc.dimension = RenderTextureDimension::TEXTURE_2D;
	textureDesc.width = width;
	textureDesc.height = height;
	textureDesc.depth = 1;
	textureDesc.mipLevels = 1;
	textureDesc.arraySize = 1;
	textureDesc.format = RenderFormat::R8G8B8A8_UNORM;

	g_imFontTexture->textureHolder = g_device->createTexture(textureDesc);
	g_imFontTexture->texture = g_imFontTexture->textureHolder.get();

	uint32_t rowPitch = (width * 4 + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
	uint32_t slicePitch = (rowPitch * height + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);
	auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
	uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

	if (rowPitch == (width * 4))
	{
		memcpy(mappedMemory, pixels, slicePitch);
	}
	else
	{
		for (size_t i = 0; i < height; i++)
		{
			memcpy(mappedMemory, pixels, width * 4);
			pixels += width * 4;
			mappedMemory += rowPitch;
		}
	}

	uploadBuffer->unmap();

	ExecuteCopyCommandList([&]
		{
			g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(g_imFontTexture->texture, RenderTextureLayout::COPY_DEST));

			g_copyCommandList->copyTextureRegion(
				RenderTextureCopyLocation::Subresource(g_imFontTexture->texture, 0),
				RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), RenderFormat::R8G8B8A8_UNORM, width, height, 1, rowPitch / 4, 0));
		});

	g_imFontTexture->layout = RenderTextureLayout::COPY_DEST;

	RenderTextureViewDesc textureViewDesc;
	textureViewDesc.format = textureDesc.format;
	textureViewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
	textureViewDesc.mipLevels = 1;
	g_imFontTexture->textureView = g_imFontTexture->texture->createTextureView(textureViewDesc);

	g_imFontTexture->descriptorIndex = g_textureDescriptorAllocator.allocate();
	g_textureDescriptorSet->setTexture(g_imFontTexture->descriptorIndex, g_imFontTexture->texture, RenderTextureLayout::SHADER_READ, g_imFontTexture->textureView.get());
	//#endif

	io.Fonts->SetTexID(g_imFontTexture.get());

	RenderPipelineLayoutBuilder pipelineLayoutBuilder;
	pipelineLayoutBuilder.begin(false, true);

	RenderDescriptorSetBuilder descriptorSetBuilder;
	descriptorSetBuilder.begin();
	descriptorSetBuilder.addTexture(0, TEXTURE_DESCRIPTOR_SIZE);
	descriptorSetBuilder.end(true, TEXTURE_DESCRIPTOR_SIZE);
	pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

	descriptorSetBuilder.begin();
	descriptorSetBuilder.addSampler(0, SAMPLER_DESCRIPTOR_SIZE);
	descriptorSetBuilder.end(true, SAMPLER_DESCRIPTOR_SIZE);
	pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

	pipelineLayoutBuilder.addPushConstant(0, 2, sizeof(ImGuiPushConstants), RenderShaderStageFlag::VERTEX | RenderShaderStageFlag::PIXEL);

	pipelineLayoutBuilder.end();
	g_imPipelineLayout = pipelineLayoutBuilder.create(g_device.get());

	auto vertexShader = CREATE_SHADER(imgui_vs);
	auto pixelShader = CREATE_SHADER(imgui_ps);

	RenderInputElement inputElements[3];
	inputElements[0] = RenderInputElement("POSITION", 0, 0, RenderFormat::R32G32_FLOAT, 0, offsetof(ImDrawVert, pos));
	inputElements[1] = RenderInputElement("TEXCOORD", 0, 1, RenderFormat::R32G32_FLOAT, 0, offsetof(ImDrawVert, uv));
	inputElements[2] = RenderInputElement("COLOR", 0, 2, RenderFormat::R8G8B8A8_UNORM, 0, offsetof(ImDrawVert, col));

	RenderInputSlot inputSlot(0, sizeof(ImDrawVert));

	RenderGraphicsPipelineDesc pipelineDesc;
	pipelineDesc.pipelineLayout = g_imPipelineLayout.get();
	pipelineDesc.vertexShader = vertexShader.get();
	pipelineDesc.pixelShader = pixelShader.get();
	pipelineDesc.renderTargetFormat[0] = BACKBUFFER_FORMAT;
	pipelineDesc.renderTargetBlend[0] = RenderBlendDesc::AlphaBlend();
	pipelineDesc.renderTargetCount = 1;
	pipelineDesc.inputElements = inputElements;
	pipelineDesc.inputElementsCount = std::size(inputElements);
	pipelineDesc.inputSlots = &inputSlot;
	pipelineDesc.inputSlotsCount = 1;
	g_imPipeline = g_device->createGraphicsPipeline(pipelineDesc);

	pipelineDesc.renderTargetBlend[0].dstBlend = RenderBlend::ONE;
	g_imAdditivePipeline = g_device->createGraphicsPipeline(pipelineDesc);

#ifndef ENABLE_IM_FONT_ATLAS_SNAPSHOT
	ImFontAtlasSnapshot snapshot;
	snapshot.Snap();

	FILE* file = fopen("im_font_atlas.bin", "wb");
	if (file)
	{
		fwrite(snapshot.data.data(), 1, snapshot.data.size(), file);
		fclose(file);
	}

	ddspp::Header header;
	ddspp::HeaderDXT10 headerDX10;
	ddspp::encode_header(ddspp::R8G8B8A8_UNORM, width, height, 1, ddspp::Texture2D, 1, 1, header, headerDX10);

	file = fopen("im_font_atlas.dds", "wb");
	if (file)
	{
		fwrite(&ddspp::DDS_MAGIC, 4, 1, file);
		fwrite(&header, sizeof(header), 1, file);
		fwrite(&headerDX10, sizeof(headerDX10), 1, file);
		fwrite(pixels, 4, width * height, file);
		fclose(file);
	}
#endif

	printf("Done.\n");
}

static void CheckSwapChain()
{
	g_swapChain->setVsyncEnabled(Config::VSync);
	g_swapChainValid &= !g_swapChain->needsResize();

	if (!g_swapChainValid)
	{
		Video::WaitForGPU();
		g_backBuffer->framebuffers.clear();
		g_swapChainValid = g_swapChain->resize();
		g_needsResize = g_swapChainValid;
	}

	if (g_swapChainValid)
	{
		g_swapChainAcquireProfiler.Begin();
		g_swapChainValid = g_swapChain->acquireTexture(g_acquireSemaphores[g_frame].get(), &g_backBufferIndex);
		g_swapChainAcquireProfiler.End();
	}

	if (g_needsResize)
		Video::ComputeViewportDimensions();

	g_backBuffer->width = Video::s_viewportWidth;
	g_backBuffer->height = Video::s_viewportHeight;
}

static void BeginCommandList()
{
	g_renderTarget = g_backBuffer;
	g_depthStencil = nullptr;
	g_framebuffer = nullptr;

	g_pipelineState.renderTargetFormat = BACKBUFFER_FORMAT;
	g_pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;

	if (g_swapChainValid)
	{
		uint32_t width = Video::s_viewportWidth;
		uint32_t height = Video::s_viewportHeight;

		bool usingIntermediaryTexture = (width != g_swapChain->getWidth()) || (height != g_swapChain->getHeight()) ||
			(abs(Config::Brightness - 0.5f) > 0.001f);

		if (usingIntermediaryTexture)
		{
			if (g_intermediaryBackBufferTextureWidth != width ||
				g_intermediaryBackBufferTextureHeight != height)
			{
				if (g_intermediaryBackBufferTextureDescriptorIndex == NULL)
					g_intermediaryBackBufferTextureDescriptorIndex = g_textureDescriptorAllocator.allocate();


				Video::WaitForGPU(); // Fine to wait for GPU, this'll only happen during resize.

				g_intermediaryBackBufferTexture = g_device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET));
				g_textureDescriptorSet->setTexture(g_intermediaryBackBufferTextureDescriptorIndex, g_intermediaryBackBufferTexture.get(), RenderTextureLayout::SHADER_READ);

				g_intermediaryBackBufferTextureWidth = width;
				g_intermediaryBackBufferTextureHeight = height;

				g_backBuffer->framebuffers.clear();
			}

			g_backBuffer->texture = g_intermediaryBackBufferTexture.get();
		}
		else
		{
			g_backBuffer->texture = g_swapChain->getTexture(g_backBufferIndex);
		}
	}
	else
	{
		g_backBuffer->texture = g_backBuffer->textureHolder.get();
	}

	g_backBuffer->layout = RenderTextureLayout::UNKNOWN;

	for (size_t i = 0; i < 16; i++)
	{
		g_sharedConstants.texture2DIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D;
		g_sharedConstants.texture3DIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D;
		g_sharedConstants.textureCubeIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE;
	}

	memset(g_textures, 0, sizeof(g_textures));

	/**if (Config::GITextureFiltering == EGITextureFiltering::Bicubic)
		g_pipelineState.specConstants |= SPEC_CONSTANT_BICUBIC_GI_FILTER;
	else*/
	//g_pipelineState.specConstants &= ~SPEC_CONSTANT_BICUBIC_GI_FILTER;

	auto& commandList = g_commandLists[g_frame];

	commandList->begin();
	commandList->resetQueryPool(g_queryPools[g_frame].get(), 0, NUM_QUERIES);
	commandList->writeTimestamp(g_queryPools[g_frame].get(), 0);
	commandList->setGraphicsPipelineLayout(g_pipelineLayout.get()); //also set root signature
	commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0); //texture 2D also set RootDescriptorTable
	commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 1); //texture 3D
	commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 2); //texture cube
	commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 3); //sampler

	g_readyForCommands = true;
	g_readyForCommands.notify_one();
}

static GuestBuffer* CreateVertexBuffer(uint32_t length);

bool Video::CreateHostDevice(const char* sdlVideoDriver, bool graphicsApiRetry)
{
	//the most important loop
	for (uint32_t i = 0; i < 16; i++)
		g_inputSlots[i].index = i; //plume check the index in setVertexBuffers function.

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	GameWindow::Init(sdlVideoDriver);


#ifdef SHADOW_RECOMP_D3D12
	g_vulkan = DetectWine() || Config::GraphicsAPI == EGraphicsAPI::Vulkan;
#endif

	// Attempt to create the possible backends using a vector of function pointers. Whichever succeeds first will be the chosen API.
	using RenderInterfaceFunction = std::unique_ptr<RenderInterface>(void);
	std::vector<RenderInterfaceFunction*> interfaceFunctions;

#ifdef SHADOW_RECOMP_D3D12
	bool allowVulkanRedirection = true;

	if (graphicsApiRetry)
	{
		// If we are attempting to create again after a reboot due to a crash, swap the order.
		g_vulkan = !g_vulkan;

		// Don't allow redirection to Vulkan if we are retrying after a crash, 
		// so the user can at least boot the game with D3D12 if Vulkan fails to work.
		allowVulkanRedirection = false;
	}

	interfaceFunctions.push_back(g_vulkan ? CreateVulkanInterfaceWrapper : CreateD3D12Interface);
	interfaceFunctions.push_back(g_vulkan ? CreateD3D12Interface : CreateVulkanInterfaceWrapper);
#else
	interfaceFunctions.push_back(CreateVulkanInterfaceWrapper);
#endif

	for (size_t i = 0; i < interfaceFunctions.size(); i++)
	{
		RenderInterfaceFunction* interfaceFunction = interfaceFunctions[i];

#ifdef SHADOW_RECOMP_D3D12
		// Wrap the device creation in __try/__except to survive from driver crashes.
		__try
#endif
		{
			g_interface = interfaceFunction();
			if (g_interface == nullptr)
			{
				continue;
			}

			g_device = g_interface->createDevice(Config::GraphicsDevice);
			if (g_device != nullptr)
			{
				const RenderDeviceDescription& deviceDescription = g_device->getDescription();

#ifdef SHADOW_RECOMP_D3D12
				if (interfaceFunction == CreateD3D12Interface)
				{
					if (allowVulkanRedirection)
					{
						bool redirectToVulkan = false;

						if (deviceDescription.vendor == RenderDeviceVendor::AMD)
						{
							// AMD Drivers before this version have a known issue where MSAA resolve targets will fail to work correctly.
							// If no specific graphics API was selected, we silently destroy this one and move to the next option as it'll
							// just work incorrectly otherwise and result in visual glitches and 3D rendering not working in general.
							constexpr uint64_t MinimumAMDDriverVersion = 0x1F00005DC2005CULL; // 31.0.24002.92
							if ((Config::GraphicsAPI == EGraphicsAPI::Auto) && (deviceDescription.driverVersion < MinimumAMDDriverVersion))
								redirectToVulkan = true;
						}
						else if (deviceDescription.vendor == RenderDeviceVendor::INTEL)
						{
							// Intel drivers on D3D12 are extremely buggy, introducing various graphical glitches.
							// We will redirect users to Vulkan until a workaround can be found.
							if (Config::GraphicsAPI == EGraphicsAPI::Auto)
								redirectToVulkan = true;
						}

						if (redirectToVulkan)
						{
							g_device.reset();
							g_interface.reset();

							// In case Vulkan fails to initialize, we will try D3D12 again afterwards, 
							// just to get the game to boot. This only really happens in very old Intel GPU drivers.
							if (!g_vulkan)
							{
								interfaceFunctions.push_back(CreateD3D12Interface);
								allowVulkanRedirection = false;
							}

							continue;
						}
					}

					// Hardware resolve seems to be completely bugged on Intel D3D12 drivers.
					g_hardwareResolve = (deviceDescription.vendor != RenderDeviceVendor::INTEL);
					g_hardwareDepthResolve = (deviceDescription.vendor != RenderDeviceVendor::INTEL);
				}

				g_vulkan = (interfaceFunction == CreateVulkanInterfaceWrapper);
#endif

				break;
			}
		}
#ifdef SHADOW_RECOMP_D3D12
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			if (graphicsApiRetry)
			{
				// If we were retrying, and this also failed, then we'll show the user neither of the graphics APIs succeeded.
				return false;
			}
			else
			{
				// If this is the first crash we ran into, reboot and try the other graphics API.
				//os::process::StartProcess(os::process::GetExecutablePath(), { "--graphics-api-retry" });
				std::_Exit(0);
			}
		}
#endif
	}

	if (g_device == nullptr)
	{
		return false;
	}

#ifdef SHADOW_RECOMP_D3D12
	if (graphicsApiRetry)
	{
		// If we managed to create a device after retrying it in a reboot, remember the one we picked.
		Config::GraphicsAPI = g_vulkan ? EGraphicsAPI::Vulkan : EGraphicsAPI::D3D12;
	}
#endif

	g_capabilities = g_device->getCapabilities();

	//    LoadEmbeddedResources();


	constexpr uint64_t LowEndMemoryLimit = 2048ULL * 1024ULL * 1024ULL;
	RenderDeviceDescription deviceDescription = g_device->getDescription();
	bool lowEndType = deviceDescription.type != RenderDeviceType::UNKNOWN && deviceDescription.type != RenderDeviceType::DISCRETE;
	bool lowEndMemory = deviceDescription.dedicatedVideoMemory < LowEndMemoryLimit;
	bool lowEndUMA = deviceDescription.type == RenderDeviceType::UNKNOWN && g_capabilities.uma;
	if (lowEndType || lowEndMemory || lowEndUMA)
	{
		// Switch to low end defaults if a non-discrete GPU was detected or a low amount of VRAM was detected.
		// Checking for UMA on D3D12 seems to be a reliable way to detect integrated GPUs.
	//	ApplyLowEndDefaults();
	}

	const RenderSampleCounts colourSampleCount = g_device->getSampleCountsSupported(RenderFormat::R16G16B16A16_FLOAT);
	const RenderSampleCounts depthSampleCount = g_device->getSampleCountsSupported(DepthStencilFormat);
	const RenderSampleCounts commonSampleCount = colourSampleCount & depthSampleCount;

	// Disable specific MSAA levels if they are not supported.
	/**if ((commonSampleCount & RenderSampleCount::COUNT_2) == 0)
		Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA2x);
	if ((commonSampleCount & RenderSampleCount::COUNT_4) == 0)
		Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA4x);
	if ((commonSampleCount & RenderSampleCount::COUNT_8) == 0)
		Config::AntiAliasing.InaccessibleValues.emplace(EAntiAliasing::MSAA8x);

	// Set Anti-Aliasing to nearest supported level.
	Config::AntiAliasing.SnapToNearestAccessibleValue(false);*/

	g_queue = g_device->createCommandQueue(RenderCommandListType::DIRECT);

	for (auto& commandList : g_commandLists)
		commandList = g_queue->createCommandList(); // ->createCommandList(RenderCommandListType::DIRECT);

	for (auto& commandFence : g_commandFences)
		commandFence = g_device->createCommandFence();

	for (auto& queryPool : g_queryPools)
		queryPool = g_device->createQueryPool(NUM_QUERIES);

	g_copyQueue = g_device->createCommandQueue(RenderCommandListType::COPY);
	g_copyCommandList = g_copyQueue->createCommandList();
	g_copyCommandFence = g_device->createCommandFence();

	uint32_t bufferCount = 2;

	switch (Config::TripleBuffering)
	{
	case ETripleBuffering::Auto:
		if (g_vulkan)
		{
			// Defaulting to 3 is fine if presentWait as supported, as the maximum frame latency allowed is only 1.
			bufferCount = g_device->getCapabilities().presentWait ? 3 : 2;
		}
		else
		{
			// Defaulting to 3 is fine on D3D12 thanks to flip discard model.
			bufferCount = 3;
		}

		break;
	case ETripleBuffering::On:
		bufferCount = 3;
		break;
	case ETripleBuffering::Off:
		bufferCount = 2;
		break;
	}

	g_swapChain = g_queue->createSwapChain(GameWindow::s_renderWindow, bufferCount, BACKBUFFER_FORMAT, Config::MaxFrameLatency);
	g_swapChain->setVsyncEnabled(Config::VSync);
	g_swapChainValid = !g_swapChain->needsResize();

	for (auto& acquireSemaphore : g_acquireSemaphores)
		acquireSemaphore = g_device->createCommandSemaphore();

	for (auto& renderSemaphore : g_renderSemaphores)
		renderSemaphore = g_device->createCommandSemaphore();

	RenderPipelineLayoutBuilder pipelineLayoutBuilder;
	pipelineLayoutBuilder.begin(false, true);

	RenderDescriptorSetBuilder descriptorSetBuilder;
	descriptorSetBuilder.begin();
	descriptorSetBuilder.addTexture(0, TEXTURE_DESCRIPTOR_SIZE);
	descriptorSetBuilder.end(true, TEXTURE_DESCRIPTOR_SIZE);

	g_textureDescriptorSet = descriptorSetBuilder.create(g_device.get());

	for (size_t i = 0; i < TEXTURE_DESCRIPTOR_NULL_COUNT; i++)
	{
		auto& texture = g_blankTextures[i];
		auto& textureView = g_blankTextureViews[i];

		RenderTextureDesc desc;
		desc.width = 1;
		desc.height = 1;
		desc.depth = 1;
		desc.mipLevels = 1;
		desc.format = RenderFormat::R8_UNORM;

		RenderTextureViewDesc viewDesc;
		viewDesc.format = desc.format;
		viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::ZERO, RenderSwizzle::ZERO, RenderSwizzle::ZERO, RenderSwizzle::ZERO);
		viewDesc.mipLevels = 1;

		switch (i)
		{
		case TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D:
			desc.dimension = RenderTextureDimension::TEXTURE_2D;
			desc.arraySize = 1;
			viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
			break;

		case TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D:
			desc.dimension = RenderTextureDimension::TEXTURE_3D;
			desc.arraySize = 1;
			viewDesc.dimension = RenderTextureViewDimension::TEXTURE_3D;
			break;

		case TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE:
			desc.dimension = RenderTextureDimension::TEXTURE_2D;
			desc.arraySize = 6;
			desc.flags = RenderTextureFlag::CUBE;
			viewDesc.dimension = RenderTextureViewDimension::TEXTURE_CUBE;
			break;

		default:
			assert(false && "Unknown null descriptor dimension");
			break;
		}

		texture = g_device->createTexture(desc);
		textureView = texture->createTextureView(viewDesc);

		g_textureDescriptorSet->setTexture(i, texture.get(), RenderTextureLayout::SHADER_READ, textureView.get());
	}

	pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);
	pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);
	pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

	descriptorSetBuilder.begin();
	descriptorSetBuilder.addSampler(0, SAMPLER_DESCRIPTOR_SIZE);
	descriptorSetBuilder.end(true, SAMPLER_DESCRIPTOR_SIZE);

	g_samplerDescriptorSet = descriptorSetBuilder.create(g_device.get());
	auto& [descriptorIndex, sampler] = g_samplerStates[XXH3_64bits(&g_samplerDescs[0], sizeof(RenderSamplerDesc))];
	descriptorIndex = 1;
	sampler = g_device->createSampler(g_samplerDescs[0]);
	g_samplerDescriptorSet->setSampler(0, sampler.get());

	pipelineLayoutBuilder.addDescriptorSet(descriptorSetBuilder);

	if (g_vulkan)
	{
		pipelineLayoutBuilder.addPushConstant(0, 4, 24, RenderShaderStageFlag::VERTEX | RenderShaderStageFlag::PIXEL);
	}
	else
	{
		pipelineLayoutBuilder.addRootDescriptor(0, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
		pipelineLayoutBuilder.addRootDescriptor(1, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
		pipelineLayoutBuilder.addRootDescriptor(2, 4, RenderRootDescriptorType::CONSTANT_BUFFER);
		pipelineLayoutBuilder.addPushConstant(3, 4, 4, RenderShaderStageFlag::PIXEL); // For copy/resolve shaders.
	}
	pipelineLayoutBuilder.end();

	g_pipelineLayout = pipelineLayoutBuilder.create(g_device.get());

	g_copyShader = CREATE_SHADER(copy_vs);
	g_copyColorShader = CREATE_SHADER(copy_color_ps);
	auto copyDepthShader = CREATE_SHADER(copy_depth_ps);

	RenderGraphicsPipelineDesc desc;
	desc.pipelineLayout = g_pipelineLayout.get();
	desc.vertexShader = g_copyShader.get();
	desc.pixelShader = copyDepthShader.get();
	desc.depthFunction = RenderComparisonFunction::ALWAYS;
	desc.depthEnabled = true;
	desc.depthWriteEnabled = true;
	desc.depthTargetFormat = DepthStencilFormat;
	g_copyDepthPipeline = g_device->createGraphicsPipeline(desc);




	//CreateImGuiBackend();

	auto gammaCorrectionShader = CREATE_SHADER(gamma_correction_ps);

	desc = {};
	desc.pipelineLayout = g_pipelineLayout.get();
	desc.vertexShader = g_copyShader.get();
	desc.pixelShader = gammaCorrectionShader.get();
	desc.renderTargetFormat[0] = BACKBUFFER_FORMAT;
	desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
	desc.renderTargetCount = 1;
	g_gammaCorrectionPipeline = g_device->createGraphicsPipeline(desc);

	FixedFuncVS = CREATE_SHADER(FixedFunctionVertexShader);
	FixedFuncPS = CREATE_SHADER(FixedFunctionPixelShader);
	PassthroughVS = CREATE_SHADER(PassthroughVertexShader);


	// NOTE: We initially allocate this on host memory to make the installer work, even if the 4 GB memory allocation fails.
	g_backBufferHolder = std::make_unique<GuestSurface>(ResourceType::RenderTarget);

	g_backBuffer = g_backBufferHolder.get();
	g_backBuffer->width = 640;
	g_backBuffer->height = 480;
	g_backBuffer->format = BACKBUFFER_FORMAT;
	g_backBuffer->textureHolder = g_device->createTexture(RenderTextureDesc::Texture2D(1, 1, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET));

	Video::ComputeViewportDimensions();
	CheckSwapChain();
	BeginCommandList();

	RenderTextureBarrier blankTextureBarriers[TEXTURE_DESCRIPTOR_NULL_COUNT];
	for (size_t i = 0; i < TEXTURE_DESCRIPTOR_NULL_COUNT; i++)
		blankTextureBarriers[i] = RenderTextureBarrier(g_blankTextures[i].get(), RenderTextureLayout::SHADER_READ);

	g_commandLists[g_frame]->barriers(RenderBarrierStage::NONE, blankTextureBarriers, std::size(blankTextureBarriers));

	return true;
}

static uint32_t g_waitForGPUCount = 0;

void Video::WaitForGPU()
{
	g_waitForGPUCount++;

	// Wait for all queued frames to finish.
	for (size_t i = 0; i < NUM_FRAMES; i++)
	{
		if (g_commandListStates[i])
		{
			g_queue->waitForCommandFence(g_commandFences[i].get());
			g_commandListStates[i] = false;
		}
	}

	// Execute an empty command list and wait for it to end to guarantee that any remaining presentation has finished.
	g_commandLists[0]->begin();
	g_commandLists[0]->end();
	g_queue->executeCommandLists(g_commandLists[0].get(), g_commandFences[0].get());
	g_queue->waitForCommandFence(g_commandFences[0].get());
}


static GuestVertexDeclaration* CreateVertexDeclaration(D3DVERTEXELEMENT9* vertexElements, const bool isFixed)
{
	auto vertexDeclaration = CreateVertexDeclarationWithoutAddRef(vertexElements, isFixed);
	//vertexDeclaration->AddRef();
	return vertexDeclaration;
}

static void SetVertexDeclaration(GuestVertexDeclaration* vertexDeclaration)
{
	RenderCommand cmd;
	cmd.type = RenderCommandType::SetVertexDeclaration;
	cmd.setVertexDeclaration.vertexDeclaration = vertexDeclaration;
	g_renderQueue.enqueue(cmd);
}

static void ProcSetVertexDeclaration(const RenderCommand& cmd)
{
	auto& args = cmd.setVertexDeclaration;

	if (args.vertexDeclaration != nullptr)
	{
		//SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.swappedTexcoords, args.vertexDeclaration->swappedTexcoords);

		/**uint32_t specConstants = g_pipelineState.specConstants;
		if (args.vertexDeclaration->hasR11G11B10Normal)
			specConstants |= SPEC_CONSTANT_R11G11B10_NORMAL;
		else
			specConstants &= ~SPEC_CONSTANT_R11G11B10_NORMAL;

		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);*/
	}

	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexDeclaration, args.vertexDeclaration);
}


extern bool logIndexedPrimitive;
static void CopyVertexShaderConstant(int32_t Register, DWORD* pConstantData, const uint32_t count);
void UpdateHostVertexDeclaration(HostVertexDeclaration* pHostVertexDeclaration)
{
	// Titles can specify default values for registers via calls like SetVertexData4f
	// HLSL shaders need to know whether to use vertex data or default vertex shader values
	// Any register not in the vertex declaration should be set to the default value
	float vertexDefaultFlags[X_VSH_MAX_ATTRIBUTES];
	for (int32_t i = 0; i < X_VSH_MAX_ATTRIBUTES; i++)
	{
		vertexDefaultFlags[i] = pHostVertexDeclaration->vRegisterInDeclaration[i] ? 0.0f : 1.0f;
		if (logIndexedPrimitive)
		{
			//printf("vRegisterInDeclaration %d, %f\n", i, vertexDefaultFlags[i]);
		}
		vsh_constants_dirty[HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_BASE + i] = true;
	}


	//192 + 16 = 208 vRegisterDefaultFlagsPacked
	CopyVertexShaderConstant(HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_BASE, (DWORD*)vertexDefaultFlags, HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_SIZE);
	vsh_constants[HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_BASE][0] = vsh_constants[HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_BASE][0];
	g_vertexConstantsDirtyFlags = true;
}

//TODO
static ShaderCacheEntry* FindShaderCacheEntry(XXH64_hash_t hash)
{
	/**auto end = g_shaderCacheEntries + g_shaderCacheEntryCount;
	auto findResult = std::lower_bound(g_shaderCacheEntries, end, hash, [](ShaderCacheEntry& lhs, XXH64_hash_t rhs)
		{
			return lhs.hash < rhs;
		});*/

	return nullptr; //findResult != end && findResult->hash == hash ? findResult : nullptr;
}

std::unordered_map<uint32_t, GuestShader*> g_VertexShaderMap;
/**static GuestShader* CreateShader(const uint32_t* pDeclaration, ResourceType resourceType)
{
	size_t size = 0;
	auto curDeclaration = pDeclaration;

	std::ofstream  outFile2("HashShader.txt", std::ios::app);

	while (*curDeclaration != X_D3DVSD_END())
	{
		outFile2 << std::hex << *curDeclaration << '\n';
		size += sizeof(uint32_t);
		curDeclaration++;
	}

	// include the END token
	size += sizeof(uint32_t);

	XXH64_hash_t hash = XXH3_64bits(pDeclaration, size);

	printf("hash shader %llx\n", hash);

	outFile2 << '\n';
	outFile2 << "hash 0x" << std::hex << hash << '\n';

	outFile2 << '\n';
	outFile2.close();

	auto findResult = FindShaderCacheEntry(hash);
	GuestShader* shader = nullptr;
	hash = hash;
	if (findResult != nullptr)
	{
		if (findResult->guestShader == nullptr)
		{
			shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);

			shader->shaderCacheEntry = findResult;
			findResult->guestShader = shader;
		}
		else
		{
			shader = findResult->guestShader;
		}
	}

	if (shader == nullptr)
		shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);
	else
		shader->AddRef();

	if (hash == 0xe555c89eab6e17ed)
		shader->shader = CREATE_SHADER(recomp0);
	else if (hash == 0x89dd45174e675122)
		shader->shader = CREATE_SHADER(recomp1);
	else if (hash == 0xa660f57bbbc6ff47) //(hash == 0x7351ba28d201371d)
	{
		shader->shader = CREATE_SHADER(recomp2);
	}
	else if (hash == 0xa90eacbcad6cc71c)
	{
		shader->shader = CREATE_SHADER(recomp3);
	}
	else
	{
		hash = hash;
		printf("unknown shader, it's time to implement xshader recomp\n");

	}

	/**if (hash == 0x31173204A896098A)
		g_csdShader = shader;

	return shader;
}*/


GuestShader* CreateShader(ResourceType resourceType)
{
	GuestShader* shader = FindShaderCacheEntryNew();

	if (shader)
	{
		shader->AddRef();
	}
	else
	{
		uint64_t hash = GetVertexShaderKey();

		shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);

		//if (hash == 0xe555c89eab6e17ed)
		if (hash == 0xb2b7a0a5e7d5805f)
		{
			shader->shader = CREATE_SHADER(recomp0);
		}
		else if (hash == 0xcefa8ae484bff7a2) //(hash == 0x89dd45174e675122)
		{
			shader->shader = CREATE_SHADER(recomp1);
		}
		else if (hash == 0xcee36c8c99adc694)  //(hash == 0x7351ba28d201371d)
		{
			shader->shader = CREATE_SHADER(recomp2);
		}
		else if (hash == 0xdfae999de3ea6283) //(hash == 0xa90eacbcad6cc71c)
		{
			shader->shader = CREATE_SHADER(recomp3);
		}
		else
		{
			printf("unknown shader, it's time to implement xshader recomp\n");
		}

		UpdateShaderCacheEntryNew(shader, hash);
	}

	return shader;
}


//Since DX8 doesn't separate Vertex Declaration like DX9, we have to let the original code run so it can create the og shader and extract
//the vertex elements from it later.

X86_FUNC_IMPL(__imp__sub_3D1BA0);
/**int32_t CreateVertexShader(uint32_t* pDeclaration, uint32_t* pFunction, GuestShader* pHandle, int32_t Usage)
{
	//let original code run first
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	__imp__sub_3D1BA0(*ctx, base);
	ctx->esp.u32 -= 20; //4 * 4 + 4 esp

	// We can't count on pFunction as it can be null, so we will use the vertex declaration for hash and cache
	auto type = ResourceType::VertexShader;

	GuestShader* Shader = CreateShader(pDeclaration, type);

	auto guest_shaderAddr = (uint32_t)X86_LOAD_U32(ctx->esp.u32 + 12);
	auto guest_shader = X86_LOAD_U32(guest_shaderAddr);
	g_VertexShaderMap.insert({ guest_shader, Shader });

	return 0;
}*/

bool lastCallOk = false;
bool logVB = false;
void UpdateHostViewPortOffsetAndScaleConstants();
X86_FUNC_IMPL(__imp__sub_3D2280);
uint32_t lastHandleAddr = 0;

void UpdateVertexDeclaration(uint32_t Handle)
{
	size_t host_handle = (size_t)g_memory.Translate(Handle);

	bool isFixed = false;
	if ((host_handle & 1) == 0 && Handle < 0xFFFF) //FVF stuff
	{
		isFixed = true;
	}
	//as mentioned above, there is no separate function that create vertex declaration, so we have to manually extract and handle them.
	//we handle vertex declaration immediately to avoid a race issue.
	if (isFixed)
		SetFixedFunctionDefaultVertexAttributes(host_handle);
	auto vertexElement = HostGetVertexDeclaration(host_handle, isFixed);
	UpdateHostVertexDeclaration(vertexElement);
	auto vertexDeclaration = CreateVertexDeclaration(vertexElement->pHostVertexElements, isFixed);
	SetVertexDeclaration(vertexDeclaration);

}

void UpdateVertexShader(uint32_t Handle)
{
	size_t host_handle = (size_t)g_memory.Translate(Handle);
	DWORD vshFlags = GetXboxVertexShaderForHandle(host_handle)->Flags;
	RenderCommand cmd;
	bool isFixed = false;
	if ((host_handle & 1) == 0 && Handle < 0xFFFF) //FVF stuff
	{
		isFixed = true;
	}
	else
	{
		auto shader = CreateShader(ResourceType::VertexShader); //g_VertexShaderMap[Handle];
		if (shader == nullptr)
		{
			return;
		}
		else
		{
			cmd.setVertexShader.shader = shader;
		}
	}

	cmd.type = RenderCommandType::SetVertexShader;
	cmd.setVertexShader.handle = Handle;

	if (!isFixed && cmd.setVertexShader.shader)
	{
		cmd.setVertexShader.type = VertexShaderMode::ShaderProgram;
	}
	else
	{
		cmd.setVertexShader.type = VertexShaderMode::FixedFunction;
	}

	UpdateVertexDeclaration(Handle);

	g_renderQueue.enqueue(cmd);
}

// DX8 has a fixed pipeline, meaning we won't always have a shader from this function, sometimes it can be FVF too
// Since DX12 only support Shader, we will use a uber shader to emulate FVF stuff, see FixedFunctionVertexShader.hlsl
static void SetVertexShader(uint32_t Handle)
{
	lastHandleAddr = Handle;

	//let original code run first so it will call SelectVertexShader function
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	__imp__sub_3D2280(*ctx, base);
	ctx->esp.u32 -= 8; //1 arg + ret value*/

	UpdateVertexShader(Handle);

}

static void ProcSetVertexShader(const RenderCommand& cmd)
{

	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShaderType, cmd.setVertexShader.type);
	const bool isFixed = cmd.setVertexShader.type == VertexShaderMode::FixedFunction;

	size_t host_handle = (size_t)g_memory.Translate(cmd.setVertexShader.handle);

	if (isFixed)
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShader, (GuestShader*)host_handle);
	}
	else
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexShader, cmd.setVertexShader.shader);
	}

}

void SelectVertexShader(DWORD Handle, DWORD Address)
{
	g_Xbox_VertexShader_FunctionSlots_StartAddress = Address;
	UpdateVertexShader(Handle);
}

void LoadVertexShaderProgram(DWORD* pFunction, DWORD Address)
{
	auto tokens = (DWORD*)&pFunction[1];
	auto shaderHeader = *((X_VSH_SHADER_HEADER*)pFunction);
	SetVertexShaderSlots(tokens, Address, shaderHeader.NumInst);
}

void LoadVertexShader(DWORD Handle, DWORD Address)
{
	// Handle is always address of an X_D3DVertexShader struct, thus always or-ed with 1 (X_D3DFVF_RESERVED0)
// Address is the slot (offset) from which the program must be written onwards (as whole DWORDS)
// D3DDevice_LoadVertexShader pushes the program contained in the Xbox VertexShader struct to the NV2A

	X_D3DVertexShader* pXboxVertexShader = VshHandleToXboxVertexShader((size_t)g_memory.Translate(Handle));

	DWORD NrTokens;
	DWORD* pNV2ATokens = GetVertexShaderTokens(pXboxVertexShader, &NrTokens);

#if 1 // TODO : Remove dirty hack (?once CreateVertexShader trampolines to Xbox code that sets ProgramAndConstantsDwords correctly?) :
	if (NrTokens == 0)
		NrTokens = 10000;
#endif

	static unsigned ConstantAddress = 0;
	DWORD* pEnd = pNV2ATokens + NrTokens;
	while (pNV2ATokens < pEnd)
	{
		DWORD dwMethod, dwSubChannel, nrDWORDS;
		D3DPUSH_DECODE(*pNV2ATokens++, dwMethod, dwSubChannel, nrDWORDS);
		if (nrDWORDS == 0)
		{
			printf("Zero-length NV2A method detected!\n");
			break;
		}

		switch (dwMethod) {
		case NV2A_VP_UPLOAD_INST(0): { // = 0x00000B00
			if ((nrDWORDS & 3) != 0)
				printf("NV2A_VP_UPLOAD_INST arguments should be a multiple of 4!\n");

			unsigned nrSlots = nrDWORDS / X_VSH_INSTRUCTION_SIZE;
			SetVertexShaderSlots(pNV2ATokens, Address, nrSlots);
			Address += nrSlots;
			break;
		}
		case NV2A_VP_UPLOAD_CONST_ID: // = 0x00001EA4
			if (nrDWORDS != 1)
				printf("NV2A_VP_UPLOAD_CONST_ID should have one argument!\n");

			ConstantAddress = *pNV2ATokens;
			break;
		case NV2A_VP_UPLOAD_CONST(0): { // = 0x00000B80
			if ((nrDWORDS & 3) != 0)
				printf("NV2A_VP_UPLOAD_CONST arguments should be a multiple of 4!\n");

			unsigned nrConstants = nrDWORDS / X_VSH_INSTRUCTION_SIZE;
			// TODO : FIXME : Implement and call SetVertexShaderConstants(pNV2ATokens, ConstantAddress, nrConstants);
			ConstantAddress += nrConstants;
			break;
		}
		default:
			// TODO : Remove this break-out hack once NrTokens is reliable and instead have: DEFAULT_UNREACHABLE;
			printf("Stopping at unexpected NV2A method\n");
			pEnd = pNV2ATokens;
			break;
		}

		pNV2ATokens += nrDWORDS;
	}
}

int32_t CreatePixelShader(const uint32_t pPSDef, DWORD* pHandle)
{
	return 0;
}

//todo check if we can pass a GuestShader or if we will have to do it differently
static void SetPixelShader(uint32_t handle)
{
	if (handle != 0)
	{
		printf("WAIT\n");
	}
	RenderCommand cmd;
	cmd.type = RenderCommandType::SetPixelShader;
	cmd.setPixelShader.handle = handle;
	cmd.setPixelShader.isFixed = handle == 0;
	g_renderQueue.enqueue(cmd);
}

static void ProcSetPixelShader(const RenderCommand& cmd)
{
	GuestShader* shader = cmd.setPixelShader.shaderHandle;
	//SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader, shader);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.isFixedFunctionPS, cmd.setPixelShader.isFixed);
}


bool logIndexed = false;
static void FlushVertexBufferForRendering();
X86_FUNC_IMPL(__imp__sub_3D1E10);
static void SetStreamSource(uint32_t StreamNumber, GuestBuffer* buffer, uint32_t stride)
{
	auto base = g_memory.base;


	g_Xbox_SetStreamSource[StreamNumber].VertexBuffer = buffer;
	g_Xbox_SetStreamSource[StreamNumber].Stride = stride;



	/**RenderCommand cmd;
	cmd.type = RenderCommandType::SetStreamSource;
	cmd.setStreamSource.StreamNumber = StreamNumber;
	cmd.setStreamSource.vertexBuffer = buffer;
	cmd.setStreamSource.stride = stride;
	g_renderQueue.enqueue(cmd);*/

	//we need to let the original code run otherwise this preven't other vertex buffer to be used

	//printf("set VB %x, stream number %d, stride %d\n", buffer, StreamNumber, stride);
	auto ctx = GetX86Context();
	__imp__sub_3D1E10(*ctx, base);
	ctx->esp.u32 -= (4 + 12);
}

//offset might be 0 but I'm not sure, to do check
static void ProcSetStreamSource(const RenderCommand& cmd)
{
	const auto& args = cmd.setStreamSource;

	auto vb = args.vertexBuffer;
	void* gpuRef = vb ? (void*)vb->buffer->at(0).ref : nullptr;
	/**printf("[BIND-SETSTREAM] stream=%u guestVB=%p gpuRef=%p stride=%u size=%u\n",
		args.StreamNumber, vb, gpuRef, args.stride,
		vb ? vb->dataSize : 0u);*/



	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[args.StreamNumber], uint8_t(args.vertexBuffer != nullptr ? args.stride : 0));

	bool dirty = false;

	SetDirtyValue(dirty, g_vertexBufferViews[args.StreamNumber].buffer, args.vertexBuffer != nullptr ? args.vertexBuffer->buffer->at(0) : RenderBufferReference{});
	SetDirtyValue(dirty, g_vertexBufferViews[args.StreamNumber].size, args.vertexBuffer != nullptr ? (args.vertexBuffer->dataSize - 0) : 0u);
	SetDirtyValue(dirty, g_inputSlots[args.StreamNumber].stride, args.vertexBuffer != nullptr ? args.stride : 0u);

	if (dirty)
	{
		g_dirtyStates.vertexStreamFirst = std::min<uint8_t>(g_dirtyStates.vertexStreamFirst, args.StreamNumber);
		g_dirtyStates.vertexStreamLast = std::max<uint8_t>(g_dirtyStates.vertexStreamLast, args.StreamNumber);
	}
}

static const char* DeviceTypeName(RenderDeviceType type)
{
	switch (type)
	{
	case RenderDeviceType::INTEGRATED:
		return "Integrated";
	case RenderDeviceType::DISCRETE:
		return "Discrete";
	case RenderDeviceType::VIRTUAL:
		return "Virtual";
	case RenderDeviceType::CPU:
		return "CPU";
	default:
		return "Unknown";
	}
}

static std::atomic<uint32_t> g_bufferUploadCount = 0;

static void DrawProfiler()
{
	bool toggleProfiler = SDL_GetKeyboardState(nullptr)[SDL_SCANCODE_F1] != 0;

	if (!g_profilerWasToggled && toggleProfiler)
	{
		g_profilerVisible = !g_profilerVisible;

		GameWindow::SetFullscreenCursorVisibility(App::s_isInit ? g_profilerVisible : true);
	}

	g_profilerWasToggled = toggleProfiler;

	if (!g_profilerVisible)
		return;

	ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
	float defaultScale = font->Scale;
	font->Scale = ImGui::GetDefaultFont()->FontSize / font->FontSize;
	ImGui::PushFont(font);

	if (ImGui::Begin("Profiler", &g_profilerVisible))
	{
		g_applicationValues[g_profilerValueIndex] = App::s_deltaTime * 1000.0;

		const double applicationAvg = std::accumulate(g_applicationValues, g_applicationValues + PROFILER_VALUE_COUNT, 0.0) / PROFILER_VALUE_COUNT;
		double gpuFrameAvg = g_gpuFrameProfiler.UpdateAndReturnAverage();
		double presentAvg = g_presentProfiler.UpdateAndReturnAverage();
		double updateDirectorAvg = g_updateDirectorProfiler.UpdateAndReturnAverage();
		double renderDirectorAvg = g_renderDirectorProfiler.UpdateAndReturnAverage();
		double frameFenceAvg = g_frameFenceProfiler.UpdateAndReturnAverage();
		double presentWaitAvg = g_presentWaitProfiler.UpdateAndReturnAverage();
		double swapChainAcquireAvg = g_swapChainAcquireProfiler.UpdateAndReturnAverage();

		if (ImPlot::BeginPlot("Frame Time"))
		{
			ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, 20.0);
			ImPlot::SetupAxis(ImAxis_Y1, "ms", ImPlotAxisFlags_None);
			ImPlot::PlotLine<double>("Application", g_applicationValues, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
			ImPlot::PlotLine<double>("GPU Frame", g_gpuFrameProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
			ImPlot::PlotLine<double>("Present", g_presentProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
			ImPlot::PlotLine<double>("Update Director", g_updateDirectorProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
			ImPlot::PlotLine<double>("Render Director", g_renderDirectorProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
			ImPlot::PlotLine<double>("Frame Fence", g_frameFenceProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
			ImPlot::PlotLine<double>("Present Wait", g_presentWaitProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
			ImPlot::PlotLine<double>("Swap Chain Acquire", g_swapChainAcquireProfiler.values, PROFILER_VALUE_COUNT, 1.0, 0.0, ImPlotLineFlags_None, g_profilerValueIndex);
			ImPlot::EndPlot();
		}

		g_profilerValueIndex = (g_profilerValueIndex + 1) % PROFILER_VALUE_COUNT;

		ImGui::Text("Current Application: %g ms (%g FPS)", App::s_deltaTime * 1000.0, 1.0 / App::s_deltaTime);
		ImGui::Text("Current GPU Frame: %g ms (%g FPS)", g_gpuFrameProfiler.value.load(), 1000.0 / g_gpuFrameProfiler.value.load());
		ImGui::Text("Current Present: %g ms (%g FPS)", g_presentProfiler.value.load(), 1000.0 / g_presentProfiler.value.load());
		ImGui::Text("Current Update Director: %g ms (%g FPS)", g_updateDirectorProfiler.value.load(), 1000.0 / g_updateDirectorProfiler.value.load());
		ImGui::Text("Current Render Director: %g ms (%g FPS)", g_renderDirectorProfiler.value.load(), 1000.0 / g_renderDirectorProfiler.value.load());
		ImGui::Text("Current Frame Fence: %g ms", g_frameFenceProfiler.value.load());
		ImGui::Text("Current Present Wait: %g ms", g_presentWaitProfiler.value.load());
		ImGui::Text("Current Swap Chain Acquire: %g ms", g_swapChainAcquireProfiler.value.load());

		ImGui::NewLine();

		ImGui::Text("Average Application: %g ms (%g FPS)", applicationAvg, 1000.0 / applicationAvg);
		ImGui::Text("Average GPU Frame: %g ms (%g FPS)", gpuFrameAvg, 1000.0 / gpuFrameAvg);
		ImGui::Text("Average Present: %g ms (%g FPS)", presentAvg, 1000.0 / presentAvg);
		ImGui::Text("Average Update Director: %g ms (%g FPS)", updateDirectorAvg, 1000.0 / updateDirectorAvg);
		ImGui::Text("Average Render Director: %g ms (%g FPS)", renderDirectorAvg, 1000.0 / renderDirectorAvg);
		ImGui::Text("Average Frame Fence: %g ms", frameFenceAvg);
		ImGui::Text("Average Present Wait: %g ms", presentWaitAvg);
		ImGui::Text("Average Swap Chain Acquire: %g ms", swapChainAcquireAvg);

		ImGui::NewLine();

		if (g_userHeap.heap != nullptr && g_userHeap.physicalHeap != nullptr)
		{
			O1HeapDiagnostics diagnostics, physicalDiagnostics;
			{
				std::lock_guard lock(g_userHeap.mutex);
				diagnostics = o1heapGetDiagnostics(g_userHeap.heap);
			}
			{
				std::lock_guard lock(g_userHeap.physicalMutex);
				physicalDiagnostics = o1heapGetDiagnostics(g_userHeap.physicalHeap);
			}

			ImGui::Text("Heap Allocated: %d MB", int32_t(diagnostics.allocated / (1024 * 1024)));
			ImGui::Text("Physical Heap Allocated: %d MB", int32_t(physicalDiagnostics.allocated / (1024 * 1024)));
		}

		ImGui::Text("GPU Waits: %d", int32_t(g_waitForGPUCount));
		ImGui::Text("Buffer Uploads: %d", int32_t(g_bufferUploadCount));
		ImGui::NewLine();

		ImGui::Text("Present Wait: %s", g_capabilities.presentWait ? "Supported" : "Unsupported");
		ImGui::Text("Triangle Fan: %s", g_capabilities.triangleFan ? "Supported" : "Unsupported");
		ImGui::Text("Dynamic Depth Bias: %s", g_capabilities.dynamicDepthBias ? "Supported" : "Unsupported");
		//ImGui::Text("Triangle Strip Workaround: %s", g_triangleStripWorkaround ? "Enabled" : "Disabled");
		ImGui::Text("Hardware Resolve: %s", g_hardwareResolve ? "Enabled" : "Disabled");
		ImGui::Text("Hardware Depth Resolve: %s", g_hardwareDepthResolve ? "Enabled" : "Disabled");
		ImGui::NewLine();

		ImGui::Text("API: %s", g_vulkan ? "Vulkan" : "D3D12");
		ImGui::Text("Device: %s", g_device->getDescription().name.c_str());
		ImGui::Text("Device Type: %s", DeviceTypeName(g_device->getDescription().type));
		ImGui::Text("VRAM: %.2f MiB", (double)(g_device->getDescription().dedicatedVideoMemory) / (1024.0 * 1024.0));
		ImGui::Text("UMA: %s", g_capabilities.uma ? "Supported" : "Unsupported");
		ImGui::Text("GPU Upload Heap: %s", g_capabilities.gpuUploadHeap ? "Supported" : "Unsupported");

		const char* sdlVideoDriver = SDL_GetCurrentVideoDriver();
		if (sdlVideoDriver != nullptr)
			ImGui::Text("SDL Video Driver: %s", sdlVideoDriver);

		ImGui::NewLine();
		ImGui::Checkbox("Show FPS", &Config::ShowFPS);
		ImGui::NewLine();

		if (ImGui::TreeNode("Device Names"))
		{
			ImGui::Indent();

			uint32_t deviceIndex = 0;
			for (const std::string& deviceName : g_interface->getDeviceNames())
			{
				ImGui::Text("Option #%d: %s", deviceIndex++, deviceName.c_str());
			}

			ImGui::Unindent();
			ImGui::TreePop();
		}
	}
	ImGui::End();

	ImGui::PopFont();
	font->Scale = defaultScale;
}

static void DrawFPS()
{

	if (!Config::ShowFPS)
		return;

	double time = ImGui::GetTime();
	static double updateTime = time;
	static double fps = 0;
	static double totalDeltaTime = 0.0;
	static uint32_t totalDeltaCount = 0;

	totalDeltaTime += g_presentProfiler.value.load();
	totalDeltaCount++;

	if (time - updateTime >= 1.0f)
	{
		fps = 1000.0 / std::max(totalDeltaTime / double(totalDeltaCount), 1.0);
		updateTime = time;
		totalDeltaTime = 0.0;
		totalDeltaCount = 0;
	}

	auto drawList = ImGui::GetBackgroundDrawList();

	auto fmt = fmt::format("FPS: {:.2f}", fps);
	//auto font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-M.otf");
	auto font = ImGui::GetIO().Fonts->Fonts[0];
	auto fontSize = Scale(10);
	auto textSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0, fmt.c_str());

	ImVec2 min = { Scale(40), Scale(30) };
	ImVec2 max = { min.x + std::max(Scale(75), textSize.x + Scale(10)), min.y + Scale(15) };
	ImVec2 textPos = { min.x + Scale(2), CENTRE_TEXT_VERT(min, max, textSize) + Scale(0.2f) };

	drawList->AddRectFilled(min, max, IM_COL32(0, 0, 0, 200));
	drawList->AddText(font, fontSize, textPos, IM_COL32_WHITE, fmt.c_str());
}

static void DrawImGui()
{
	ImGui_ImplSDL2_NewFrame();

	auto& io = ImGui::GetIO();
	io.DisplaySize = { float(Video::s_viewportWidth), float(Video::s_viewportHeight) };

	// ImGui doesn't know that we center the screen for specific aspect ratio
	// settings, which causes mouse events to not work correctly. To fix this, 
	// we can adjust the mouse events before ImGui processes them.
	uint32_t width = g_swapChain->getWidth();
	uint32_t height = g_swapChain->getHeight();
	float mousePosScaleX = float(width) / float(GameWindow::s_width);
	float mousePosScaleY = float(height) / float(GameWindow::s_height);
	float mousePosOffsetX = (width - Video::s_viewportWidth) / 2.0f;
	float mousePosOffsetY = (height - Video::s_viewportHeight) / 2.0f;
	for (int i = 0; i < io.Ctx->InputEventsQueue.Size; i++)
	{
		auto& e = io.Ctx->InputEventsQueue[i];
		if (e.Type == ImGuiInputEventType_MousePos)
		{
			if (e.MousePos.PosX != -FLT_MAX)
			{
				e.MousePos.PosX *= mousePosScaleX;
				e.MousePos.PosX -= mousePosOffsetX;
			}

			if (e.MousePos.PosY != -FLT_MAX)
			{
				e.MousePos.PosY *= mousePosScaleY;
				e.MousePos.PosY -= mousePosOffsetY;
			}
		}
	}

	ImGui::NewFrame();

	ResetImGuiCallbacks();

#ifdef ASYNC_PSO_DEBUG
	if (ImGui::Begin("Async PSO Stats"))
	{
		ImGui::Text("Pipelines Created In Render Thread: %d", g_pipelinesCreatedInRenderThread.load());
		ImGui::Text("Pipelines Created Asynchronously: %d", g_pipelinesCreatedAsynchronously.load());
		ImGui::Text("Pipelines Dropped: %d", g_pipelinesDropped.load());
		ImGui::Text("Pipelines Currently Compiling: %d", g_pipelinesCurrentlyCompiling.load());
		ImGui::Text("Compiling Pipeline Task Count: %d", g_compilingPipelineTaskCount.load());
		ImGui::Text("Pending Pipeline Task Count: %d", g_pendingPipelineTaskCount.load());

		std::lock_guard lock(g_debugMutex);
		ImGui::TextUnformatted(g_pipelineDebugText.c_str());
	}
	ImGui::End();
#endif

	/**AchievementMenu::Draw();
	OptionsMenu::Draw();
	AchievementOverlay::Draw();
	InstallerWizard::Draw();
	MessageWindow::Draw();
	ButtonGuide::Draw();
	Fader::Draw();
	BlackBar::Draw();*/

	assert(ImGui::GetBackgroundDrawList()->_ClipRectStack.Size == 1 && "Some clip rects were not removed from the stack!");

	DrawFPS();
	DrawProfiler();
	ImGui::Render();

	auto drawData = ImGui::GetDrawData();
	if (drawData->CmdListsCount != 0)
	{
		RenderCommand cmd;
		cmd.type = RenderCommandType::DrawImGui;
		g_renderQueue.enqueue(cmd);
	}
}

static void SetFramebuffer(GuestSurface* renderTarget, GuestSurface* depthStencil, bool settingForClear);

static void ProcDrawImGui(const RenderCommand& cmd)
{
	// Make sure the backbuffer is the current target.
	AddBarrier(g_backBuffer, RenderTextureLayout::COLOR_WRITE);
	FlushBarriers();
	SetFramebuffer(g_backBuffer, nullptr, false);

	auto& commandList = g_commandLists[g_frame];
	auto pipeline = g_imPipeline.get();

	commandList->setGraphicsPipelineLayout(g_imPipelineLayout.get());
	commandList->setPipeline(pipeline);
	commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
	commandList->setGraphicsDescriptorSet(g_samplerDescriptorSet.get(), 1);

	auto& drawData = *ImGui::GetDrawData();
	commandList->setViewports(RenderViewport(drawData.DisplayPos.x, drawData.DisplayPos.y, drawData.DisplaySize.x, drawData.DisplaySize.y));

	ImGuiPushConstants pushConstants{};
	pushConstants.displaySize = drawData.DisplaySize;
	pushConstants.inverseDisplaySize = { 1.0f / drawData.DisplaySize.x, 1.0f / drawData.DisplaySize.y };
	commandList->setGraphicsPushConstants(0, &pushConstants);

	size_t pushConstantRangeMin = ~0;
	size_t pushConstantRangeMax = 0;

	auto setPushConstants = [&](void* destination, const void* source, size_t size)
		{
			bool dirty = memcmp(destination, source, size) != 0;

			memcpy(destination, source, size);

			if (dirty)
			{
				size_t offset = reinterpret_cast<size_t>(destination) - reinterpret_cast<size_t>(&pushConstants);
				pushConstantRangeMin = std::min(pushConstantRangeMin, offset);
				pushConstantRangeMax = std::max(pushConstantRangeMax, offset + size);
			}
		};

	ImRect clipRect{};

	for (int i = 0; i < drawData.CmdListsCount; i++)
	{
		auto& drawList = drawData.CmdLists[i];

		auto vertexBufferAllocation = g_uploadAllocators[g_frame].allocate<false>(drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert), alignof(ImDrawVert));
		auto indexBufferAllocation = g_uploadAllocators[g_frame].allocate<false>(drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(uint16_t), alignof(uint16_t));

		const RenderVertexBufferView vertexBufferView(vertexBufferAllocation.buffer->at(vertexBufferAllocation.offset), drawList->VtxBuffer.Size * sizeof(ImDrawVert));
		const RenderInputSlot inputSlot(0, sizeof(ImDrawVert));
		commandList->setVertexBuffers(0, &vertexBufferView, 1, &inputSlot);

		const RenderIndexBufferView indexBufferView(indexBufferAllocation.buffer->at(indexBufferAllocation.offset), drawList->IdxBuffer.Size * sizeof(uint16_t), RenderFormat::R16_UINT);
		commandList->setIndexBuffer(&indexBufferView);

		for (int j = 0; j < drawList->CmdBuffer.Size; j++)
		{
			auto& drawCmd = drawList->CmdBuffer[j];
			if (drawCmd.UserCallback != nullptr)
			{
				auto callbackData = reinterpret_cast<const ImGuiCallbackData*>(drawCmd.UserCallbackData);

				switch (static_cast<ImGuiCallback>(reinterpret_cast<size_t>(drawCmd.UserCallback)))
				{
				case ImGuiCallback::SetGradient:
					setPushConstants(&pushConstants.boundsMin, &callbackData->setGradient, sizeof(callbackData->setGradient));
					break;
				case ImGuiCallback::SetShaderModifier:
					setPushConstants(&pushConstants.shaderModifier, &callbackData->setShaderModifier, sizeof(callbackData->setShaderModifier));
					break;
				case ImGuiCallback::SetOrigin:
					setPushConstants(&pushConstants.origin, &callbackData->setOrigin, sizeof(callbackData->setOrigin));
					break;
				case ImGuiCallback::SetScale:
					setPushConstants(&pushConstants.scale, &callbackData->setScale, sizeof(callbackData->setScale));
					break;
				case ImGuiCallback::SetMarqueeFade:
					setPushConstants(&pushConstants.boundsMin, &callbackData->setMarqueeFade, sizeof(callbackData->setMarqueeFade));
					break;
				case ImGuiCallback::SetOutline:
					setPushConstants(&pushConstants.outline, &callbackData->setOutline, sizeof(callbackData->setOutline));
					break;
				case ImGuiCallback::SetProceduralOrigin:
					setPushConstants(&pushConstants.proceduralOrigin, &callbackData->setProceduralOrigin, sizeof(callbackData->setProceduralOrigin));
					break;
				case ImGuiCallback::SetAdditive:
				{
					auto pipelineToSet = callbackData->setAdditive.enabled ? g_imAdditivePipeline.get() : g_imPipeline.get();
					if (pipeline != pipelineToSet)
					{
						commandList->setPipeline(pipelineToSet);
						pipeline = pipelineToSet;
					}
					break;
				}
				default:
					assert(false && "Unknown ImGui callback type.");
					break;
				}
			}
			else
			{
				if (drawCmd.ClipRect.z <= drawCmd.ClipRect.x || drawCmd.ClipRect.w <= drawCmd.ClipRect.y)
					continue;

				auto texture = reinterpret_cast<GuestTexture*>(drawCmd.TextureId);
				uint32_t descriptorIndex = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D;
				if (texture != nullptr)
				{
					if (texture->layout != RenderTextureLayout::SHADER_READ)
					{
						commandList->barriers(RenderBarrierStage::GRAPHICS | RenderBarrierStage::COPY,
							RenderTextureBarrier(texture->texture, RenderTextureLayout::SHADER_READ));

						texture->layout = RenderTextureLayout::SHADER_READ;
					}

					descriptorIndex = texture->descriptorIndex;

					if (texture == g_imFontTexture.get())
						descriptorIndex |= 0x80000000;

					setPushConstants(&pushConstants.texture2DDescriptorIndex, &descriptorIndex, sizeof(descriptorIndex));
				}

				if (pushConstantRangeMin < pushConstantRangeMax)
				{
					commandList->setGraphicsPushConstants(0, reinterpret_cast<const uint8_t*>(&pushConstants) + pushConstantRangeMin, pushConstantRangeMin, pushConstantRangeMax - pushConstantRangeMin);
					pushConstantRangeMin = ~0;
					pushConstantRangeMax = 0;
				}

				if (memcmp(&clipRect, &drawCmd.ClipRect, sizeof(clipRect)) != 0)
				{
					commandList->setScissors(RenderRect(int32_t(drawCmd.ClipRect.x), int32_t(drawCmd.ClipRect.y), int32_t(drawCmd.ClipRect.z), int32_t(drawCmd.ClipRect.w)));
					clipRect = drawCmd.ClipRect;
				}

				commandList->drawIndexedInstanced(drawCmd.ElemCount, 1, drawCmd.IdxOffset, drawCmd.VtxOffset, 0);
			}
		}
	}
}

// We have to check for this to properly handle the following situation:
// 1. Wait on swap chain.
// 2. Create loading thread.
// 3. Loading thread also waits on swap chain.
// 4. Loading thread presents and quits.
// 5. After the loading thread quits, application also presents.
static bool g_pendingWaitOnSwapChain = true;

void Video::WaitOnSwapChain()
{
	if (g_pendingWaitOnSwapChain)
	{
		if (g_swapChainValid)
		{
			g_presentWaitProfiler.Begin();
			g_swapChain->wait();
			g_presentWaitProfiler.End();
		}

		g_pendingWaitOnSwapChain = false;
	}
}

static bool g_shouldPrecompilePipelines;
static std::atomic<bool> g_executedCommandList;
std::chrono::steady_clock::time_point frameStartTime;

void SleepPrecise(std::chrono::steady_clock::time_point targetTime)
{
	using namespace std::chrono;
	// If we don't need to wait, return right away

	// Try to sleep for as much of the wait as we can
	// to save CPU usage / power
	// We expect sleep to overshoot, so give ourselves some extra time
	// Note currently we ask Windows to give us 1ms timer resolution
	constexpr auto sleepThreshold = 2ms; // Minimum remaining time before we attempt to use sleep

	auto sleepFor = (targetTime - sleepThreshold) - steady_clock::now();
	auto sleepMs = duration_cast<milliseconds>(sleepFor).count();

	// Sleep if required
	if (sleepMs >= 0) {
		Sleep((DWORD)sleepMs);
	}

	// Spin wait
	while (steady_clock::now() < targetTime) {
		;
	}
}
void Video::Present()
{

	g_readyForCommands = false;

	RenderCommand cmd;
	cmd.type = RenderCommandType::ExecutePendingCopyRectsCommands;
	g_renderQueue.enqueue(cmd);

	//DrawImGui();

	cmd.type = RenderCommandType::ExecuteCommandList;
	g_renderQueue.enqueue(cmd);


	if (g_shouldPrecompilePipelines)
	{
		//EnqueuePipelineTask(PipelineTaskType::PrecompilePipelines, {});
		g_shouldPrecompilePipelines = false;
	}

	g_executedCommandList.wait(false);
	g_executedCommandList = false;

	if (g_swapChainValid)
	{
		if (g_pendingWaitOnSwapChain)
		{
			g_presentWaitProfiler.Begin();
			g_swapChain->wait(); // Never gonna happen outside loading threads as explained above.
			g_presentWaitProfiler.End();
		}

		RenderCommandSemaphore* signalSemaphores[] = { g_renderSemaphores[g_frame].get() };
		g_swapChainValid = g_swapChain->present(g_backBufferIndex, signalSemaphores, std::size(signalSemaphores));
	}

	g_pendingWaitOnSwapChain = true;

	g_frame = g_nextFrame;
	g_nextFrame = (g_frame + 1) % NUM_FRAMES;

	if (g_commandListStates[g_frame])
	{
		g_frameFenceProfiler.Begin();
		g_queue->waitForCommandFence(g_commandFences[g_frame].get());
		g_frameFenceProfiler.End();
		g_commandListStates[g_frame] = false;

		// Update the GPU profiler with the results from the timestamps of the frame.
		g_queryPools[g_frame]->queryResults();
		const uint64_t* frameTimestamps = g_queryPools[g_frame]->getResults();
		g_gpuFrameProfiler.Set(double(frameTimestamps[1] - frameTimestamps[0]) / 1000000.0);
	}

	g_dirtyStates = DirtyStates(true);
	g_uploadAllocators[g_frame].reset();
	g_intermediaryUploadAllocator.reset();
	/**g_triangleFanIndexData.reset();
	g_quadIndexData.reset();*/

	CheckSwapChain();

	cmd.type = RenderCommandType::BeginCommandList;
	g_renderQueue.enqueue(cmd);

	/**auto targetRefreshRate = 60.0f;
	auto frameMs = (1000.0 / targetRefreshRate) * 1.0f;
	auto targetDuration = std::chrono::duration_cast<std::chrono::steady_clock::duration>(std::chrono::duration<double, std::milli>(frameMs));
	auto targetTimestamp = frameStartTime + targetDuration;
	SleepPrecise(targetTimestamp);*/

	frameStartTime = std::chrono::steady_clock::now();

	if (Config::FPS >= FPS_MIN && Config::FPS < FPS_MAX)
	{
		using namespace std::chrono_literals;

		static std::chrono::steady_clock::time_point s_next;

		auto now = std::chrono::steady_clock::now();

		if (now < s_next)
		{
			std::this_thread::sleep_for(std::chrono::floor<std::chrono::milliseconds>(s_next - now - 2ms));

			while ((now = std::chrono::steady_clock::now()) < s_next)
				std::this_thread::yield();
		}
		else
		{
			s_next = now;
		}

		s_next += 1000000000ns / Config::FPS;
	}

	g_presentProfiler.Reset();
}

void Video::StartPipelinePrecompilation()
{
	g_shouldPrecompilePipelines = true;
}

static void SetRootDescriptor(const UploadAllocation& allocation, size_t index)
{
	auto& commandList = g_commandLists[g_frame];

	if (g_vulkan)
		commandList->setGraphicsPushConstants(0, &allocation.deviceAddress, 8 * index, 8);
	else
		commandList->setGraphicsRootDescriptor(allocation.buffer->at(allocation.offset), index);
}

static void ProcExecuteCommandList(const RenderCommand& cmd)
{
	if (g_swapChainValid)
	{
		auto swapChainTexture = g_swapChain->getTexture(g_backBufferIndex);
		if (g_backBuffer->texture == g_intermediaryBackBufferTexture.get())
		{
			g_backBuffer->texture = g_backBuffer->texture;
			struct
			{
				float gammaR;
				float gammaG;
				float gammaB;
				uint32_t textureDescriptorIndex;

				int32_t viewportOffsetX;
				int32_t viewportOffsetY;
				int32_t viewportWidth;
				int32_t viewportHeight;
			} constants;

			constants.gammaR = 1.0f;
			constants.gammaG = 1.0f;
			constants.gammaB = 1.0f;

			float offset = (Config::Brightness - 0.5f) * 1.2f;

			constants.gammaR = 1.0f / std::clamp(constants.gammaR + offset, 0.1f, 4.0f);
			constants.gammaG = 1.0f / std::clamp(constants.gammaG + offset, 0.1f, 4.0f);
			constants.gammaB = 1.0f / std::clamp(constants.gammaB + offset, 0.1f, 4.0f);
			constants.textureDescriptorIndex = g_intermediaryBackBufferTextureDescriptorIndex;

			constants.viewportOffsetX = (int32_t(g_swapChain->getWidth()) - int32_t(Video::s_viewportWidth)) / 2;
			constants.viewportOffsetY = (int32_t(g_swapChain->getHeight()) - int32_t(Video::s_viewportHeight)) / 2;
			constants.viewportWidth = Video::s_viewportWidth;
			constants.viewportHeight = Video::s_viewportHeight;

			auto& framebuffer = g_backBuffer->framebuffers[swapChainTexture];
			if (!framebuffer)
			{
				RenderFramebufferDesc desc;
				desc.colorAttachments = const_cast<const RenderTexture**>(&swapChainTexture);
				desc.colorAttachmentsCount = 1;
				framebuffer = g_device->createFramebuffer(desc);
			}

			RenderTextureBarrier srcBarriers[] =
			{
				RenderTextureBarrier(g_intermediaryBackBufferTexture.get(), RenderTextureLayout::SHADER_READ),
				RenderTextureBarrier(swapChainTexture, RenderTextureLayout::COLOR_WRITE)
			};

			auto& commandList = g_commandLists[g_frame];
			commandList->barriers(RenderBarrierStage::GRAPHICS, srcBarriers, std::size(srcBarriers));
			commandList->setGraphicsPipelineLayout(g_pipelineLayout.get());
			commandList->setPipeline(g_gammaCorrectionPipeline.get());
			commandList->setGraphicsDescriptorSet(g_textureDescriptorSet.get(), 0);
			SetRootDescriptor(g_uploadAllocators[g_frame].allocate<false>(&constants, sizeof(constants), 0x100), 2);
			commandList->setFramebuffer(framebuffer.get());
			commandList->setViewports(RenderViewport(0.0f, 0.0f, g_swapChain->getWidth(), g_swapChain->getHeight()));
			commandList->setScissors(RenderRect(0, 0, g_swapChain->getWidth(), g_swapChain->getHeight()));
			commandList->drawInstanced(6, 1, 0, 0);

			commandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(swapChainTexture, RenderTextureLayout::PRESENT));
		}
		else
		{
			AddBarrier(g_backBuffer, RenderTextureLayout::PRESENT);
			FlushBarriers();
		}
	}

	auto& commandList = g_commandLists[g_frame];
	commandList->writeTimestamp(g_queryPools[g_frame].get(), 1);
	commandList->end();

	if (g_swapChainValid)
	{
		const RenderCommandList* commandLists[] = { commandList.get() };
		RenderCommandSemaphore* waitSemaphores[] = { g_acquireSemaphores[g_frame].get() };
		RenderCommandSemaphore* signalSemaphores[] = { g_renderSemaphores[g_frame].get() };

		g_queue->executeCommandLists(
			commandLists, std::size(commandLists),
			waitSemaphores, std::size(waitSemaphores),
			signalSemaphores, std::size(signalSemaphores),
			g_commandFences[g_frame].get());
	}
	else
	{
		g_queue->executeCommandLists(commandList.get(), g_commandFences[g_frame].get());
	}

	g_commandListStates[g_frame] = true;

	g_executedCommandList = true;
	g_executedCommandList.notify_one();
}

static void ProcBeginCommandList(const RenderCommand& cmd)
{
	DestructTempResources();
	BeginCommandList();
}

static GuestSurface* GetBackBuffer()
{
	g_backBuffer->AddRef();
	return g_backBuffer;
}

//todo
void Video::ComputeViewportDimensions()
{
	uint32_t width = g_swapChain->getWidth();
	uint32_t height = g_swapChain->getHeight();
	float aspectRatio = float(width) / float(height);

	s_viewportWidth = 640;
	s_viewportHeight = 480;

}

static RenderFormat ConvertFormat(X_D3DFORMAT format)
{
	switch (format)
	{

	case X_D3DFMT_L8:
		return RenderFormat::R8_UNORM;
	case X_D3DFMT_A1R5G5B5:
	case X_D3DFMT_A4R4G4B4:
	case X_D3DFMT_R5G6B5:
		return RenderFormat::R16_UNORM;
	case X_D3DFMT_INDEX16:
		return RenderFormat::R16_UINT;
	case X_D3DFMT_A8R8G8B8:
	case X_D3DFMT_X8R8G8B8:
	case X_D3DFMT_LIN_X8R8G8B8:
	case X_D3DFMT_P8: 	 //Palette get converted by the CPU to ARGB in ConvertD3DTextureToARGBBuffer
		return RenderFormat::B8G8R8A8_UNORM;

	case X_D3DFMT_LIN_D24S8: //Linear 24-bit depth + 8-bit stencil

		return RenderFormat::D32_FLOAT_S8_UINT;
	default:

		assert(false && "Unknown format");
		return RenderFormat::R8_UNORM;
	}
}

//todo check if this is all correct
static GuestTexture* CreateTexture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, X_D3DFORMAT format, X_D3DRESOURCETYPE type)
{
	auto texture = g_userHeap.AllocPhysical<GuestTexture>(type == X_D3DRTYPE_VOLUMETEXTURE ? ResourceType::VolumeTexture : ResourceType::Texture);

	RenderTextureDesc desc;
	desc.dimension = RenderTextureDimension::TEXTURE_2D; //check if it's accurate
	desc.width = width;
	desc.height = height;
	desc.depth = depth;
	desc.mipLevels = levels;
	desc.arraySize = 1;
	desc.format = ConvertFormat(format);


	if (desc.format == DepthStencilFormat && FormatIsDepthBuffer(format))
		desc.flags = RenderTextureFlag::DEPTH_TARGET;
	else if (FormatIsRenderTarget(format) && usage == 1)
		desc.flags = RenderTextureFlag::RENDER_TARGET;
	else
		desc.flags = RenderTextureFlag::NONE;


	texture->isRenderTarget = desc.flags == RenderTextureFlag::RENDER_TARGET;

	texture->textureHolder = g_device->createTexture(desc);
	texture->texture = texture->textureHolder.get();

	RenderTextureViewDesc viewDesc;
	viewDesc.format = desc.format;
	viewDesc.dimension = texture->type == ResourceType::VolumeTexture ? RenderTextureViewDimension::TEXTURE_3D : RenderTextureViewDimension::TEXTURE_2D;
	viewDesc.mipLevels = levels;

	switch (desc.format)
	{
	case RenderFormat::R8_UNORM: //todo check if this is correct
		viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::R, RenderSwizzle::ONE);
		break;
	case RenderFormat::B8G8R8A8_UNORM:
	case RenderFormat::R8G8B8A8_UNORM: //(confirmed) no swizzling needed, color order is already correct!
		viewDesc.componentMapping = RenderComponentMapping(RenderSwizzle::IDENTITY, RenderSwizzle::IDENTITY, RenderSwizzle::IDENTITY, RenderSwizzle::IDENTITY);
		break;
	}

	texture->textureView = texture->texture->createTextureView(viewDesc);

	texture->width = width;
	texture->height = height;
	texture->depth = depth;
	texture->format = desc.format;
	texture->viewDimension = viewDesc.dimension;
	texture->descriptorIndex = g_textureDescriptorAllocator.allocate();

	g_textureDescriptorSet->setTexture(texture->descriptorIndex, texture->texture, RenderTextureLayout::SHADER_READ, texture->textureView.get());

	return texture;
}

static GuestTexture* CreateTexture_(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, X_D3DFORMAT format, X_D3DRESOURCETYPE type)
{
	return CreateTexture(width, height, depth, levels, usage, format, type);
}

static RenderHeapType GetBufferHeapType()
{
	return g_capabilities.gpuUploadHeap ? RenderHeapType::GPU_UPLOAD : RenderHeapType::DEFAULT;
}


X86_FUNC_IMPL(__imp__sub_2E6750);
static GuestBuffer* CreateVertexBuffer(uint32_t length)
{
	auto base = g_memory.base;
	auto ctx = GetX86Context();

	//call original function
	/**__imp__sub_2E6750(*ctx, base);
	ctx->esp.u32 -= 4;*/

	int ogSize = length;

	int newSize = length;//(ogSize + 31) & 0xFFFFFFE0;


	//if (ctx->eax.u32 != 0)
	{
		//auto guestBufferData = (X_D3DResource*)g_memory.Translate(ctx->eax.u32);

		auto buffer = g_userHeap.AllocPhysical<GuestBuffer>(ResourceType::VertexBuffer);
		buffer->buffer = g_device->createBuffer(RenderBufferDesc::VertexBuffer(newSize, GetBufferHeapType(), RenderBufferFlag::INDEX));
		buffer->dataSize = newSize;
		buffer->guestResource.Common = 1;

		//memcpy(&buffer->guestResource, guestBufferData, sizeof(X_D3DResource));
#ifdef _DEBUG 
		buffer->buffer->setName(fmt::format("Vertex Buffer {:X}", g_memory.MapVirtual(buffer)));
		buffer->isVertexBuffer = true;
#endif
		//ctx->eax.u32 = g_memory.MapVirtual(buffer);
		return buffer;
	}

	return 0;
}

//it's possible that Shadow  don't have a CreateIndexBuffer function,
// if that's confirmed, we will have to create it on the fly during rendering with a cache system.
static GuestBuffer* CreateIndexBuffer(uint32_t IndexCount, X_D3DFORMAT format)
{
	uint32_t length = IndexCount * sizeof(int16_t);

	auto buffer = g_userHeap.AllocPhysical<GuestBuffer>(ResourceType::IndexBuffer);
	buffer->buffer = g_device->createBuffer(RenderBufferDesc::IndexBuffer(length, GetBufferHeapType()));
	buffer->dataSize = length;
	buffer->format = ConvertFormat(format);
	buffer->guestFormat = format;
#ifdef _DEBUG 
	buffer->buffer->setName(fmt::format("Index Buffer {:X}", g_memory.MapVirtual(buffer)));
#endif
	return buffer;
}

static std::vector<GuestSurface*> g_surfaceCache;


static GuestSurface* CreateSurface(uint32_t width, uint32_t height, X_D3DFORMAT format, bool formatSet = false)
{
	RenderTextureDesc desc;
	desc.dimension = RenderTextureDimension::TEXTURE_2D;
	desc.width = width;
	desc.height = height;
	desc.depth = 1;
	desc.mipLevels = 1;
	desc.arraySize = 1;
	desc.multisampling.sampleCount = RenderSampleCount::COUNT_1; //todo add as an option(?)
	desc.format = formatSet ? (plume::RenderFormat)format : ConvertFormat(format);
	desc.flags = desc.format == DepthStencilFormat ? RenderTextureFlag::DEPTH_TARGET : RenderTextureFlag::RENDER_TARGET;

	auto surface = g_userHeap.AllocPhysical<GuestSurface>(desc.format == DepthStencilFormat ?
		ResourceType::DepthStencil : ResourceType::RenderTarget);

	surface->textureHolder = g_device->createTexture(desc);
	surface->texture = surface->textureHolder.get();
	surface->width = width;
	surface->height = height;
	surface->format = desc.format;
	surface->guestFormat = format;
	surface->sampleCount = desc.multisampling.sampleCount;

	RenderTextureViewDesc viewDesc;
	viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
	viewDesc.format = desc.format;
	viewDesc.mipLevels = 1;
	surface->textureView = surface->textureHolder->createTextureView(viewDesc);
	surface->descriptorIndex = g_textureDescriptorAllocator.allocate();
	g_textureDescriptorSet->setTexture(surface->descriptorIndex, surface->textureHolder.get(), RenderTextureLayout::SHADER_READ, surface->textureView.get());

#ifdef _DEBUG 
	surface->texture->setName(fmt::format("{} {:X}", desc.flags & RenderTextureFlag::RENDER_TARGET ? "Render Target" : "Depth Stencil", g_memory.MapVirtual(surface)));
#endif

	surface->guestResource.Format = format;
	return surface;
}

static xxHashMap<GuestSurface*> g_surfaceLevel;
static GuestSurface* GetSurfaceLevel(GuestTexture* texture, int32_t level, X_D3DFORMAT Format)
{

	//return CreateSurface(texture->width, texture->height, (X_D3DFORMAT)texture->format, true);
	//return (GuestSurface*)texture;


	XXH64_hash_t hash = XXH3_64bits(texture, sizeof(GuestTexture) * level + 1);
	auto& surface = g_surfaceLevel[hash];
	if (surface != nullptr)
	{
		return surface;
	}


	// Calculate dimensions for this mip level
	uint32_t mipWidth = std::max(1u, texture->width >> level);
	uint32_t mipHeight = std::max(1u, texture->height >> level);

	// Allocate a surface object
	surface = g_userHeap.AllocPhysical<GuestSurface>(
		texture->format == DepthStencilFormat ?
		ResourceType::DepthStencil : ResourceType::RenderTarget
	);


	surface->texture = texture->texture;

	surface->width = mipWidth;
	surface->height = mipHeight;
	surface->format = texture->format;
	surface->guestFormat = texture->guestResource.Format;
	surface->sampleCount = RenderSampleCount::COUNT_1;

	// Create a view specifically for this mip level
	RenderTextureViewDesc viewDesc;
	viewDesc.dimension = RenderTextureViewDimension::TEXTURE_2D;
	viewDesc.format = texture->format;

	viewDesc.mipLevels = 1;             // Only this one level


	surface->textureView = texture->texture->createTextureView(viewDesc);

	// Allocate descriptor for this specific mip level view
	surface->descriptorIndex = g_textureDescriptorAllocator.allocate();
	g_textureDescriptorSet->setTexture(
		surface->descriptorIndex,
		texture->texture,  // Same texture
		RenderTextureLayout::SHADER_READ,
		surface->textureView.get()
	);


	surface->guestResource.Format = texture->guestResource.Format;
	texture->sourceSurface = surface;
	return surface;
}


X86_FUNC_IMPL(__imp__sub_3DA010);
//CreateSurfaceLevel = "view" or "interface" to access ONE specific level/slice of a texture resource.
//todo figure out this
GuestSurface* CreateSurfaceOfTexture(X_D3DFORMAT Format, uint32_t size, GuestBaseTexture* pParent, int32_t pvData)
{

	auto ctx = GetX86Context();
	auto base = g_memory.base;
	__imp__sub_3DA010(*ctx, base);
	ctx->esp.u32 -= 20;

	auto gSurface = (X_D3DSurface*)g_memory.Translate(ctx->eax.u32);
	auto newSurface = GetSurfaceLevel((GuestTexture*)pParent, 0, Format);
	memcpy(&newSurface->guestResource, gSurface, sizeof(X_D3DSurface));
	ctx->eax.u32 = g_memory.MapVirtual(newSurface);
	return newSurface;
	//return CreateSurface(pParent->width, pParent->height, (X_D3DFORMAT)pParent->format, true);
}

//CreateDepthStencilSurface
GuestSurface* CreateStandAloneSurface(int32_t Width, int32_t Height, int32_t a3, X_D3DFORMAT Format)
{
	return CreateSurface(Width, Height, Format);
}

static void FlushViewport()
{
	auto& commandList = g_commandLists[g_frame];

	if (g_dirtyStates.viewport)
	{
		auto viewport = g_viewport;

		if (viewport.minDepth > viewport.maxDepth)
			std::swap(viewport.minDepth, viewport.maxDepth);

		commandList->setViewports(viewport);

		g_dirtyStates.viewport = false;
	}

	if (g_dirtyStates.scissorRect)
	{
		auto scissorRect = g_scissorTestEnable ? g_scissorRect : RenderRect(
			g_viewport.x,
			g_viewport.y,
			g_viewport.x + g_viewport.width,
			g_viewport.y + g_viewport.height);

		commandList->setScissors(scissorRect);

		g_dirtyStates.scissorRect = false;
	}
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
	switch (a2 & 3)
	{
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


//is this broken? (hardware remove thing)
X86_FUNC_IMPL(__imp__sub_203A20);
X86_FUNC(sub_203A20) {

	__imp__sub_203A20(ctx, base);
}


X_D3DFORMAT MapToLinearD3DFORMAT(X_D3DFORMAT a1)
{
	X_D3DFORMAT result;

	result = a1;
	switch (a1)
	{
	case X_D3DFMT_A1R5G5B5:
		result = X_D3DFMT_LIN_A1R5G5B5;
		break;
	case X_D3DFMT_X1R5G5B5:
		result = X_D3DFMT_LIN_X1R5G5B5;
		break;
	case X_D3DFMT_R5G6B5:
		result = X_D3DFMT_LIN_R5G6B5;
		break;
	case X_D3DFMT_A8R8G8B8:
		result = X_D3DFMT_LIN_A8R8G8B8;
		break;
	case X_D3DFMT_X8R8G8B8:
		result = X_D3DFMT_LIN_X8R8G8B8;
		break;
	case X_D3DFMT_D24S8:
		result = X_D3DFMT_LIN_D24S8;
		break;
	case X_D3DFMT_F24S8:
		result = X_D3DFMT_LIN_F24S8;
		break;
	case X_D3DFMT_D16_LOCKABLE:
		result = X_D3DFMT_LIN_D16;
		break;
	case X_D3DFMT_F16:
		result = X_D3DFMT_LIN_F16;
		break;
	default:
		return result;
	}
	return result;
}


X86_FUNC_IMPL(__imp__sub_3DDAB0); //CreateDevice
X86_FUNC_IMPL(__imp__sub_3D4420);
X86_FUNC_IMPL(__imp__sub_3D4E30);


unsigned int D3DDevice_Clear(uint32_t count, _D3DRECT* pRects, uint32_t flags, uint32_t Color, float Z, uint32_t Stencil);
int32_t Direct3D_CreateDevice_r(int32_t Adapter, int32_t DeviceType, int32_t hFocusWindow, char Flags, X_D3DPRESENT_PARAMETERS* pPresentationParameters, uint32_t* device)
{
	printf("\nCREATE GUEST D3D DEVICE!\n");

	auto ctx = GetX86Context();
	auto base = g_memory.base;

	// Move backbuffer to guest memory.
	assert(!g_memory.IsInMemoryRange(g_backBuffer) && g_backBufferHolder != nullptr);
	g_backBuffer = g_userHeap.AllocPhysical<GuestSurface>(std::move(*g_backBufferHolder));

	// Check for stale reference. BeginCommandList() gets called before CreateDevice() which is where the assignment happens.
	if (g_renderTarget == g_backBufferHolder.get())
		g_renderTarget = g_backBuffer;

	if (g_depthStencil == g_backBufferHolder.get())
		g_depthStencil = g_backBuffer;

	// Free the host backbuffer.
	g_backBufferHolder = nullptr;


	//assign the guest array of texture render state to a pointer so we can get each texture state in real time
	g_texture_stage_states = (int32_t*)(base + g_DeferredTextureStateAddr);

	__imp__sub_3DDAB0(*ctx, base);
	ctx->esp.u32 -= 28; //readjust esp because of calling original function

	return 0; //success
}
static int desCount = 0;
static void DestructResource(GuestResource* resource)
{

	RenderCommand cmd;
	cmd.type = RenderCommandType::DestructResource;

	cmd.destructResource.resource = resource;
	g_renderQueue.enqueue(cmd);
	desCount++;
}

static void ProcDestructResource(const RenderCommand& cmd)
{
	desCount = desCount;
	const auto& args = cmd.destructResource;
	g_tempResources[g_frame].push_back(args.resource);
}

X_D3DFORMAT GetXboxPixelContainerFormat(const uint32_t XboxPixelContainer_Format)
{
	X_D3DFORMAT d3d_format = (X_D3DFORMAT)((XboxPixelContainer_Format & X_D3DFORMAT_FORMAT_MASK) >> X_D3DFORMAT_FORMAT_SHIFT);
	return d3d_format;
}

static uint32_t ComputeTexturePitch(GuestTexture* texture)
{
	return (texture->width * RenderFormatSize(texture->format) + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
}

static void LockTextureRect(GuestTexture* texture, int32_t level, GuestLockedRect* lockedRect, RECT* pRect, uint32_t flags)
{

	auto format = GetXboxPixelContainerFormat(texture->guestResource.Format);

	uint32_t pitch = ComputeTexturePitch(texture);
	uint32_t slicePitch = pitch * texture->height;


	if (texture->mappedMemory == nullptr)
		texture->mappedMemory = g_userHeap.AllocPhysical(slicePitch, 0x10);

	lockedRect->pitch = pitch;
	lockedRect->bits = g_memory.MapVirtual(texture->mappedMemory);
}

static void UnlockTextureRect(GuestTexture* texture)
{
	RenderCommand cmd;
	cmd.type = RenderCommandType::UnlockTextureRect;
	cmd.unlockTextureRect.texture = texture;
	g_renderQueue.enqueue(cmd);
}


static void CopyDynamicTextureToGPU(GuestTexture* tex)
{
	auto base = g_memory.base;
	auto format = GetXboxPixelContainerFormat((X_D3DFORMAT)tex->guestResource.Format);
	bool isSwizzled = FormatIsSwizzled(format);
	bool bCompressed = XBFormatIsCompressed(format);

	uint32_t pitch = ComputeTexturePitch(tex);
	uint32_t slicePitch = pitch * tex->height;

	auto allocation = g_uploadAllocators[g_frame].allocate(slicePitch, PLACEMENT_ALIGNMENT);
	uint8_t* srcData = reinterpret_cast<uint8_t*>(tex->mappedMemory);
	uint8_t* dstData = reinterpret_cast<uint8_t*>(allocation.memory);

	if (isSwizzled)
	{
		uint32_t bpp = bCompressed ?
			(format == X_D3DFMT_DXT1 ? 8 : 16) :
			(XBFormatBitsPerPixel(format) / 8);

		UnswizzleBox(
			srcData,
			tex->width,
			tex->height,
			1, // depth
			bpp,
			dstData,
			pitch,
			slicePitch);
	}
	else
	{
		// Already linear, just copy
		memcpy(dstData, srcData, slicePitch);
	}


	// Copy on DIRECT command list (texture should already be in COPY_DEST from barrier)
	g_commandLists[g_frame]->copyTextureRegion(
		RenderTextureCopyLocation::Subresource(tex->texture, 0),
		RenderTextureCopyLocation::PlacedFootprint(
			allocation.buffer,
			tex->format,
			tex->width,
			tex->height,
			1,
			pitch / RenderFormatSize(tex->format),
			allocation.offset));
}

static void ProcUnlockTextureRect(const RenderCommand& cmd)
{
	const auto& args = cmd.unlockTextureRect;

	AddBarrier(args.texture, RenderTextureLayout::COPY_DEST);
	FlushBarriers();


	CopyDynamicTextureToGPU(args.texture);
}


//should be stubbed
uint32_t D3D_MakeRequestedSpace_()
{
	auto base = g_memory.base;
	auto device = (GuestDevice*)g_memory.Translate(GuestDevicePtrAddr);

	return device->m_Pusher.m_pPut;
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


//todo finish
static void ProcSetViewport(const RenderCommand& cmd)
{
	const auto& args = cmd.setViewport;

	SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.x, args.x);
	SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.y, args.y);
	SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.width, args.width);
	SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.height, args.height);
	SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.minDepth, args.minDepth);
	SetDirtyValue<float>(g_dirtyStates.viewport, g_viewport.maxDepth, args.maxDepth);

	uint32_t specConstants = g_pipelineState.specConstants;
	/**if (args.minDepth > args.maxDepth)
		specConstants |= SPEC_CONSTANT_REVERSE_Z;
	else
		specConstants &= ~SPEC_CONSTANT_REVERSE_Z;*/

	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.specConstants, specConstants);

	g_dirtyStates.scissorRect |= g_dirtyStates.viewport;
}


float GetZScaleForPixelContainer(X_D3DPixelContainer* pSurface)
{
	// If no surface was present, fallback to 1
	if (pSurface == xbox::zeroptr)
		return 1.0f;

	auto format = pSurface->Format;

	if (format > 50)
		format = GetXboxPixelContainerFormat(pSurface->Format);

	switch (format)
	{
	case X_D3DFMT_D16:
	case X_D3DFMT_LIN_D16:
		return 65535.0f;

	case X_D3DFMT_D24S8:
	case X_D3DFMT_LIN_D24S8:
		return 16777215.0f;

	case X_D3DFMT_F16:
	case X_D3DFMT_LIN_F16:
		return 511.9375f;

	case X_D3DFMT_F24S8:
	case X_D3DFMT_LIN_F24S8:
		// 24bit floating point is close to precision maximum, so a lower value is used
		// We can't use a double here since the vertex shader is only at float precision
		return 1.0e30f;
	}

	// Default to 1 if unknown depth format
	printf("GetZScaleForSurface: Unknown Xbox Depth Format\n");
	return 1.0f;
}



void UpdateHostTextureScaling(GuestTexture* tex, const bool isFixedFunc)
{
	// Xbox works with "Linear" and "Swizzled" texture formats
	// Linear formats are not addressed with normalized coordinates
	// We want to use normalized coordinates in our shaders, so need to be able to scale the coordinates back
	// Note texcoords aren't only used for texture lookups
	// TODO store scaling per texture instead of per stage, and scale during lookup in the pixel shader

	// Each texture stage has one texture coordinate set associated with it
	// We'll store scale factors for each texture coordinate set
	std::array<std::array<float, 4>, X_D3DTS_STAGECOUNT> texcoordScales;
	texcoordScales.fill({ 1, 1, 1, 1 });

	for (int stage = 0; stage < X_D3DTS_STAGECOUNT; stage++)
	{
		auto pXboxBaseTexture = g_textures[stage];

		// No texture, no scaling to do
		if (pXboxBaseTexture == xbox::zeroptr)
		{
			continue;
		}

		// Texcoord index. Just the texture stage unless fixed function mode
		int texCoordIndex = stage;
		if (isFixedFunc)
		{
			// Get TEXCOORDINDEX for the current texture stage's state
			// Stores both the texture stage index and information for generating coordinates
			// See D3DTSS_TEXCOORDINDEX
			auto texCoordIndexState = g_sharedConstants.TEXCOORDINDEX[stage];

			// If coordinates are generated, we don't have to worry about the coordinates coming from the title
			bool isGenerated = texCoordIndexState >= X_D3DTSS_TCI_CAMERASPACENORMAL;
			if (isGenerated)
				continue;

			// Determine the texture coordinate addressing this texture stage
			texCoordIndex = (texCoordIndexState & 0x3); // 0 - 3
		}

		auto texCoordScale = &texcoordScales[texCoordIndex];

		// Check for active linear textures.
		X_D3DFORMAT XboxFormat = GetXboxPixelContainerFormat(pXboxBaseTexture->guestResource.Format);
		if (FormatIsLinear(XboxFormat))
		{
			// Test-case : This is often hit by the help screen in XDK samples.
			// Set scaling factor for this texture, which will be applied to
			// all texture-coordinates in the current vertex shader.
			// Note : Linear textures are two-dimensional at most (right?)
			float width, height;
			if (&pXboxBaseTexture->guestResource == &g_backBuffer->guestResource) //todo this likely cannot pass atm
			{
				// Account for MSAA
				printf("todo\n");
				//GetBackBufferPixelDimensions(width, height);
			}
			else {
				width = (float)pXboxBaseTexture->width;
				height = (float)pXboxBaseTexture->height;
			}

			*texCoordScale = {
				width,
				height,
				1.0f, // TODO should this be mip levels for volume textures?
				1.0f
			};
		}

		// When a depth buffer is used as a texture
		// We do 'Native Shadow Mapping'
		// https://aras-p.info/texts/D3D9GPUHacks.html
		// The z texture coordinate component holds a depth value, which needs to be normalized
		// TODO implement handling for
		// - X_D3DRS_SHADOWFUNC
		// - X_D3DRS_POLYGONOFFSETZSLOPESCALE
		// - X_D3DRS_POLYGONOFFSETZOFFSET
		if (FormatIsDepthBuffer(XboxFormat))
		{
			(*texCoordScale)[2] = (float)GetZScaleForPixelContainer((X_D3DPixelContainer*)&pXboxBaseTexture->guestResource);
		}
	}

	// Pass above determined texture scaling factors to our HLSL shader.
	// Note : the current vertex shader applies texture scaling on
	// VS_OUTPUT registers oT0 to oT3. It may be needed to move the scaling
	// and apply it on input registers instead. In that case, we'd have to
	// figure out which registers are used to pass texture-coordinates into
	// the shader and allow scaling on any of the input registers (so we'd
	// need to allow scaling on all 16 attributes, instead of just the four
	// textures like we do right now).
	// 

	CopyVertexShaderConstant(HOST_D3DVS_TEXTURES_SCALE_BASE, (DWORD*)texcoordScales.data(), HOST_D3DVS_TEXTURES_SCALE_SIZE);
}

bool aboutToDrawFlame = false;
const intptr_t flameTexAddr = 0x00000001a0d455b0;
const intptr_t RTTexAddr = 0x00000001a0bb72f0;
const intptr_t RTBuggy = 0x00000001a0ebefb0;
intptr_t lastTex = 0;

//D3DDevice_SetTexture
GuestTexture* hudTexCopy = nullptr;
void SetTexture(uint32_t stage, GuestTexture* texture)
{
	/**if (lastTex == RTTexAddr && texture != nullptr)
	{
		SetTexture(0, nullptr);

	}*/

	//lastTex = (intptr_t)texture;



	RenderCommand cmd;
	cmd.type = RenderCommandType::SetTexture;
	cmd.setTexture.index = stage;
	cmd.setTexture.texture = texture;

	g_renderQueue.enqueue(cmd);
}

static void SetTextureInRenderThread(uint32_t index, GuestTexture* texture)
{
	AddBarrier(texture, RenderTextureLayout::SHADER_READ);

	auto viewDimension = texture != nullptr ? texture->viewDimension : RenderTextureViewDimension::UNKNOWN;

	if (texture)
	{
		g_sharedConstants.TextureSampleType[index] = (uint32_t)(viewDimension)-1;
		UpdateHostTextureScaling(texture, g_pipelineState.vertexShaderType == VertexShaderMode::FixedFunction);
		//	printf("Assigned texture descriptor index: %u\n", texture->descriptorIndex);
	}
	else
	{
		g_sharedConstants.TextureSampleType[index] = 0;
	}

	SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture2DIndices[index],
		viewDimension == RenderTextureViewDimension::TEXTURE_2D ? texture->descriptorIndex : TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D);

	SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture3DIndices[index], texture != nullptr &&
		viewDimension == RenderTextureViewDimension::TEXTURE_3D ? texture->descriptorIndex : TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D);

	SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.textureCubeIndices[index], texture != nullptr &&
		viewDimension == RenderTextureViewDimension::TEXTURE_CUBE ? texture->descriptorIndex : TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE);
}



static void SetDefaultViewport(GuestDevice* device, GuestSurface* surface)
{
	if (surface != nullptr)
	{
		RenderCommand cmd;
		cmd.type = RenderCommandType::SetViewport;
		cmd.setViewport.x = 0.0f;
		cmd.setViewport.y = 0.0f;
		cmd.setViewport.width = float(surface->width);
		cmd.setViewport.height = float(surface->height);
		cmd.setViewport.minDepth = 0.0f;
		cmd.setViewport.maxDepth = 1.0f;
		g_renderQueue.enqueue(cmd);

		device->viewport.X = 0.0f;
		device->viewport.Y = 0.0f;
		device->viewport.Width = float(surface->width);
		device->viewport.Height = float(surface->height);
		device->viewport.MinZ = 0.0f;
		device->viewport.MaxZ = 1.0f;
	}
}


void sub_3D0040_r(X86Context& ctx, uint8_t* base)
{
	// sub esp, 0x0C
	auto temp_3D0040_U32_OLD = ctx.esp.u32;
	auto temp_3D0040_U32_SRC = 12;
	ctx.esp.u32 = temp_3D0040_U32_OLD - temp_3D0040_U32_SRC;
	ctx.eflags.update_flags_sub_U32(temp_3D0040_U32_OLD, temp_3D0040_U32_SRC, ctx.esp.u32);
	// push ebx
	X86_PUSH(ctx, base, ctx.ebx.u32);
	// push ebp
	X86_PUSH(ctx, base, ctx.ebp.u32);
	// mov ebp, [esp+0x18]
	ctx.ebp.u32 = X86_LOAD_U32(ctx.esp.u32 + 24);
	// push esi
	X86_PUSH(ctx, base, ctx.esi.u32);
	// xor esi, esi
	ctx.esi.u32 ^= ctx.esi.u32;
	ctx.eflags.update_flags_logic(ctx.esi.u32);
	// cmp ebp, esi
	ctx.eflags.update_compare_flags_U32(ctx.ebp.u32, ctx.esi.u32);
	// push edi
	X86_PUSH(ctx, base, ctx.edi.u32);
	// mov edi, [0x003E0A18]
	ctx.edi.u32 = X86_LOAD_U32(4065816);
	// jnz 0x003D0063
	if ((ctx.eflags.value & X86EFlags::ZF) == 0) goto loc_3D0063;
	// mov eax, [edi+0x1A04]
	ctx.eax.u32 = X86_LOAD_U32(ctx.edi.u32 + 6660);
	// mov [esp+0x20], eax
	X86_STORE_U32(ctx.esp.u32 + 32, ctx.eax.u32);
	// mov ebp, eax
	ctx.ebp.u32 = ctx.eax.u32;
loc_3D0063:
	// push ebp
	X86_PUSH(ctx, base, ctx.ebp.u32);
	// call 0x003D5E90
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D5E90(ctx, base);
	// mov eax, [edi+0x1A04]
	ctx.eax.u32 = X86_LOAD_U32(ctx.edi.u32 + 6660);
	// cmp eax, esi
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, ctx.esi.u32);
	// jz 0x003D008E
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D008E;
	// mov ecx, [eax+0x14]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.eax.u32 + 20);
	// cmp ecx, esi
	ctx.eflags.update_compare_flags_U32(ctx.ecx.u32, ctx.esi.u32);
	// jz 0x003D007C
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D007C;
	// mov eax, ecx
	ctx.eax.u32 = ctx.ecx.u32;
loc_3D007C:
	// mov ecx, [edi+0x2C]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.edi.u32 + 44);
	// mov [eax+0x08], ecx
	X86_STORE_U32(ctx.eax.u32 + 8, ctx.ecx.u32);
	// mov edx, [edi+0x1A04]
	ctx.edx.u32 = X86_LOAD_U32(ctx.edi.u32 + 6660);
	// push edx
	X86_PUSH(ctx, base, ctx.edx.u32);
	// call 0x003D5F70
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D5F70(ctx, base);
loc_3D008E:
	// mov eax, [edi+0x1A08]
	ctx.eax.u32 = X86_LOAD_U32(ctx.edi.u32 + 6664);
	// cmp eax, esi
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, ctx.esi.u32);
	// mov [edi+0x1A04], ebp
	X86_STORE_U32(ctx.edi.u32 + 6660, ctx.ebp.u32);
	// jz 0x003D00B9
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D00B9;
	// mov ecx, [eax+0x14]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.eax.u32 + 20);
	// cmp ecx, esi
	ctx.eflags.update_compare_flags_U32(ctx.ecx.u32, ctx.esi.u32);
	// jz 0x003D00A7
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D00A7;
	// mov eax, ecx
	ctx.eax.u32 = ctx.ecx.u32;
loc_3D00A7:
	// mov ecx, [edi+0x2C]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.edi.u32 + 44);
	// mov [eax+0x08], ecx
	X86_STORE_U32(ctx.eax.u32 + 8, ctx.ecx.u32);
	// mov edx, [edi+0x1A08]
	ctx.edx.u32 = X86_LOAD_U32(ctx.edi.u32 + 6664);
	// push edx
	X86_PUSH(ctx, base, ctx.edx.u32);
	// call 0x003D5F70
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D5F70(ctx, base);
loc_3D00B9:
	// mov ebx, [esp+0x24]
	ctx.ebx.u32 = X86_LOAD_U32(ctx.esp.u32 + 36);
	// cmp ebx, esi
	ctx.eflags.update_compare_flags_U32(ctx.ebx.u32, ctx.esi.u32);
	// mov [edi+0x1A08], ebx
	X86_STORE_U32(ctx.edi.u32 + 6664, ctx.ebx.u32);
	// jz 0x003D00E6
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D00E6;
	// push ebx
	X86_PUSH(ctx, base, ctx.ebx.u32);
	// call 0x003D5E90
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D5E90(ctx, base);
	// lea eax, [edi+0x948]
	ctx.eax.u32 = ctx.edi.u32 + 2376;
	// push eax
	X86_PUSH(ctx, base, ctx.eax.u32);
	// push ebx
	X86_PUSH(ctx, base, ctx.ebx.u32);
	// call 0x003D6370
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D6370(ctx, base);
	// push ebx
	X86_PUSH(ctx, base, ctx.ebx.u32);
	// call 0x003CFF50
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3CFF50(ctx, base);
	// mov [edi+0x94C], eax
	X86_STORE_U32(ctx.edi.u32 + 2380, ctx.eax.u32);
loc_3D00E6:
	// mov eax, [edi]
	ctx.eax.u32 = X86_LOAD_U32(ctx.edi.u32);
	// cmp eax, [edi+0x04]
	ctx.eflags.update_compare_flags_U32(ctx.eax.u32, X86_LOAD_U32(ctx.edi.u32 + 4));
	// jb 0x003D00F2
	if ((ctx.eflags.value & X86EFlags::CF) != 0) goto loc_3D00F2;
	// call 0x003D4C00
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D4C00(ctx, base);
loc_3D00F2:
	// cmp ebx, esi
	ctx.eflags.update_compare_flags_U32(ctx.ebx.u32, ctx.esi.u32);
	// mov ecx, [ebp+0x04]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.ebp.u32 + 4);
	// mov edx, eax
	ctx.edx.u32 = ctx.eax.u32;
	// mov [esp+0x18], ecx
	X86_STORE_U32(ctx.esp.u32 + 24, ctx.ecx.u32);
	// jz 0x003D0108
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D0108;
	// mov eax, [ebx+0x04]
	ctx.eax.u32 = X86_LOAD_U32(ctx.ebx.u32 + 4);
	// mov [esp+0x10], eax
	X86_STORE_U32(ctx.esp.u32 + 16, ctx.eax.u32);
	// jmp 0x003D010C
	goto loc_3D010C;
loc_3D0108:
	// mov [esp+0x10], esi
	X86_STORE_U32(ctx.esp.u32 + 16, ctx.esi.u32);
loc_3D010C:
	// mov dword ptr [esp+0x14], 0x02
	X86_STORE_U32(ctx.esp.u32 + 20, 0x2u);
	// mov ebp, 0x40100
	ctx.ebp.u32 = 0x40100u;
	// mov ebx, 0x40110
	ctx.ebx.u32 = 0x40110u;
	// mov edi, edi
	ctx.edi.u32 = ctx.edi.u32;
loc_3D0120:
	// mov eax, [esp+0x20]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esp.u32 + 32);
	// mov [edx], ebp
	X86_STORE_U32(ctx.edx.u32, ctx.ebp.u32);
	// mov [edx+0x04], esi
	X86_STORE_U32(ctx.edx.u32 + 4, ctx.esi.u32);
	// cmp [eax+0x10], esi
	ctx.eflags.update_compare_flags_U32(X86_LOAD_U32(ctx.eax.u32 + 16), ctx.esi.u32);
	// jnz 0x003D017D
	if ((ctx.eflags.value & X86EFlags::ZF) == 0) goto loc_3D017D;
	// mov ecx, [eax+0x0C]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.eax.u32 + 12);
	// shr ecx, 0x14
	auto temp_3998001 = ctx.ecx.u32;
	ctx.ecx.u32 >>= 20;
	ctx.eflags.update_flags_shr(temp_3998001, 20, ctx.ecx.u32);
	// and ecx, 0x0F
	ctx.ecx.u32 &= 0xf;
	ctx.eflags.update_flags_logic(ctx.ecx.u32);
	// mov eax, 0x01
	ctx.eax.u32 = 0x1u;
	// shl eax, cl
	auto temp_3998012 = ctx.eax.u32;
	ctx.eax.u32 <<= ctx.ecx.u8_lo;
	ctx.eflags.update_flags_shl(temp_3998012, ctx.ecx.u8_lo, ctx.eax.u32);
	// mov ecx, [esp+0x20]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.esp.u32 + 32);
	// movzx ecx, byte ptr [ecx+0x0D]
	ctx.ecx.u32 = (uint32_t)X86_LOAD_U8(ctx.ecx.u32 + 13);
	// cmp ecx, 0x0C
	ctx.eflags.update_compare_flags_U32(ctx.ecx.u32, 0xCu);
	// jz 0x003D0178
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D0178;
	// cmp ecx, 0x0D
	ctx.eflags.update_compare_flags_U32(ctx.ecx.u32, 0xDu);
	// jle 0x003D015E
	if (((ctx.eflags.value & X86EFlags::ZF) != 0) || (((ctx.eflags.value & X86EFlags::SF) != 0) != ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_3D015E;
	// cmp ecx, 0x0F
	ctx.eflags.update_compare_flags_U32(ctx.ecx.u32, 0xFu);
	// jnle 0x003D015E
	if (((ctx.eflags.value & X86EFlags::ZF) == 0) && (((ctx.eflags.value & X86EFlags::SF) != 0) == ((ctx.eflags.value & X86EFlags::OF) != 0))) goto loc_3D015E;
	// lea ecx, [eax*4]
	ctx.ecx.u32 = ctx.eax.u32 * 4;
	// jmp 0x003D0185
	goto loc_3D0185;
loc_3D015E:
	// mov ecx, [esp+0x20]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.esp.u32 + 32);
	// movzx ecx, byte ptr [ecx+0x0D]
	ctx.ecx.u32 = (uint32_t)X86_LOAD_U8(ctx.ecx.u32 + 13);
	// movzx ecx, byte ptr [ecx+0x3DE5E0]
	ctx.ecx.u32 = (uint32_t)X86_LOAD_U8(ctx.ecx.u32 + 4056544);
	// and ecx, 0x3C
	ctx.ecx.u32 &= 0x3c;
	ctx.eflags.update_flags_logic(ctx.ecx.u32);
	// imul ecx, eax
	int64_t imul_result_3D0170 = static_cast<int64_t>(static_cast<int32_t>(ctx.ecx.u32)) * static_cast<int64_t>(static_cast<int32_t>(ctx.eax.u32));
	ctx.ecx.u32 = static_cast<uint32_t>(imul_result_3D0170);
	ctx.eflags.value = (ctx.eflags.value & ~(X86EFlags::CF | X86EFlags::OF)) | ((imul_result_3D0170 < INT32_MIN || imul_result_3D0170 > INT32_MAX) ? (X86EFlags::CF | X86EFlags::OF) : 0);
	// shr ecx, 0x03
	auto temp_3998067 = ctx.ecx.u32;
	ctx.ecx.u32 >>= 3;
	ctx.eflags.update_flags_shr(temp_3998067, 3, ctx.ecx.u32);
	// jmp 0x003D0185
	goto loc_3D0185;
loc_3D0178:
	// lea ecx, [eax+eax*1]
	ctx.ecx.u32 = ctx.eax.u32 + ctx.eax.u32;
	// jmp 0x003D0185
	goto loc_3D0185;
loc_3D017D:
	// movzx ecx, byte ptr [eax+0x13]
	ctx.ecx.u32 = (uint32_t)X86_LOAD_U8(ctx.eax.u32 + 19);
	// inc ecx
	auto temp_3D0181_U32 = ctx.ecx.u32;
	ctx.ecx.u32++;
	ctx.eflags.update_flags_inc_U32(temp_3D0181_U32, ctx.ecx.u32);
	// shl ecx, 0x06
	auto temp_3998082 = ctx.ecx.u32;
	ctx.ecx.u32 <<= 6;
	ctx.eflags.update_flags_shl(temp_3998082, 6, ctx.ecx.u32);
loc_3D0185:
	// mov eax, [edi+0x94C]
	ctx.eax.u32 = X86_LOAD_U32(ctx.edi.u32 + 2380);
	// mov dword ptr [edx+0x08], 0x4020C
	X86_STORE_U32(ctx.edx.u32 + 8, 0x4020Cu);
	// shl eax, 0x10
	auto temp_3998098 = ctx.eax.u32;
	ctx.eax.u32 <<= 16;
	ctx.eflags.update_flags_shl(temp_3998098, 16, ctx.eax.u32);
	// and ecx, 0xFFFF
	ctx.ecx.u32 &= 0xffff;
	ctx.eflags.update_flags_logic(ctx.ecx.u32);
	// or eax, ecx
	ctx.eax.u32 |= ctx.ecx.u32;
	ctx.eflags.update_flags_logic(ctx.eax.u32);
	// mov [edx+0x0C], eax
	X86_STORE_U32(ctx.edx.u32 + 12, ctx.eax.u32);
	// mov eax, [esp+0x18]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esp.u32 + 24);
	// mov [edx+0x10], ebp
	X86_STORE_U32(ctx.edx.u32 + 16, ctx.ebp.u32);
	// mov [edx+0x14], esi
	X86_STORE_U32(ctx.edx.u32 + 20, ctx.esi.u32);
	// mov [edx+0x18], ebx
	X86_STORE_U32(ctx.edx.u32 + 24, ctx.ebx.u32);
	// mov [edx+0x1C], esi
	X86_STORE_U32(ctx.edx.u32 + 28, ctx.esi.u32);
	// mov ecx, [esp+0x10]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.esp.u32 + 16);
	// mov [edx+0x20], ebp
	X86_STORE_U32(ctx.edx.u32 + 32, ctx.ebp.u32);
	// mov [edx+0x24], esi
	X86_STORE_U32(ctx.edx.u32 + 36, ctx.esi.u32);
	// add edx, 0x20
	auto temp_3D01BA_U32_OLD = ctx.edx.u32;
	auto temp_3D01BA_U32_SRC = 32;
	ctx.edx.u32 = temp_3D01BA_U32_OLD + temp_3D01BA_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3D01BA_U32_OLD, temp_3D01BA_U32_SRC, ctx.edx.u32);
	// mov dword ptr [edx+0x08], 0x40210
	X86_STORE_U32(ctx.edx.u32 + 8, 0x40210u);
	// mov [edx+0x0C], eax
	X86_STORE_U32(ctx.edx.u32 + 12, ctx.eax.u32);
	// mov eax, [esp+0x14]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esp.u32 + 20);
	// mov [edx+0x10], ebp
	X86_STORE_U32(ctx.edx.u32 + 16, ctx.ebp.u32);
	// mov [edx+0x14], esi
	X86_STORE_U32(ctx.edx.u32 + 20, ctx.esi.u32);
	// mov [edx+0x18], ebx
	X86_STORE_U32(ctx.edx.u32 + 24, ctx.ebx.u32);
	// mov [edx+0x1C], esi
	X86_STORE_U32(ctx.edx.u32 + 28, ctx.esi.u32);
	// add edx, 0x20
	auto temp_3D01D7_U32_OLD = ctx.edx.u32;
	auto temp_3D01D7_U32_SRC = 32;
	ctx.edx.u32 = temp_3D01D7_U32_OLD + temp_3D01D7_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3D01D7_U32_OLD, temp_3D01D7_U32_SRC, ctx.edx.u32);
	// mov [edx], ebp
	X86_STORE_U32(ctx.edx.u32, ctx.ebp.u32);
	// mov [edx+0x04], esi
	X86_STORE_U32(ctx.edx.u32 + 4, ctx.esi.u32);
	// mov dword ptr [edx+0x08], 0x40214
	X86_STORE_U32(ctx.edx.u32 + 8, 0x40214u);
	// mov [edx+0x0C], ecx
	X86_STORE_U32(ctx.edx.u32 + 12, ctx.ecx.u32);
	// mov [edx+0x10], ebp
	X86_STORE_U32(ctx.edx.u32 + 16, ctx.ebp.u32);
	// mov [edx+0x14], esi
	X86_STORE_U32(ctx.edx.u32 + 20, ctx.esi.u32);
	// mov [edx+0x18], ebx
	X86_STORE_U32(ctx.edx.u32 + 24, ctx.ebx.u32);
	// mov [edx+0x1C], esi
	X86_STORE_U32(ctx.edx.u32 + 28, ctx.esi.u32);
	// add edx, 0x20
	auto temp_3D01F5_U32_OLD = ctx.edx.u32;
	auto temp_3D01F5_U32_SRC = 32;
	ctx.edx.u32 = temp_3D01F5_U32_OLD + temp_3D01F5_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3D01F5_U32_OLD, temp_3D01F5_U32_SRC, ctx.edx.u32);
	// dec eax
	auto temp_3D01F8_U32 = ctx.eax.u32;
	ctx.eax.u32--;
	ctx.eflags.update_flags_dec_U32(temp_3D01F8_U32, ctx.eax.u32);
	// mov [esp+0x14], eax
	X86_STORE_U32(ctx.esp.u32 + 20, ctx.eax.u32);
	// jnz 0x003D0120
	if ((ctx.eflags.value & X86EFlags::ZF) == 0) goto loc_3D0120;
	// mov ecx, edi
	ctx.ecx.u32 = ctx.edi.u32;
	// call 0x003D31E0
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D31E0(ctx, base);
	// cmp [0x003E09BC], esi
	ctx.eflags.update_compare_flags_U32(X86_LOAD_U32(4065724), ctx.esi.u32);
	// jz 0x003D0221
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D0221;
	// cmp [edi+0x1A08], esi
	ctx.eflags.update_compare_flags_U32(X86_LOAD_U32(ctx.edi.u32 + 6664), ctx.esi.u32);
	// jz 0x003D0221
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D0221;
	// mov ecx, 0x01
	ctx.ecx.u32 = 0x1u;
	// jmp 0x003D0223
	goto loc_3D0223;
loc_3D0221:
	// xor ecx, ecx
	ctx.ecx.u32 ^= ctx.ecx.u32;
	ctx.eflags.update_flags_logic(ctx.ecx.u32);
loc_3D0223:
	// mov dword ptr [eax], 0x4030C
	X86_STORE_U32(ctx.eax.u32, 0x4030Cu);
	// mov [eax+0x04], ecx
	X86_STORE_U32(ctx.eax.u32 + 4, ctx.ecx.u32);
	// cmp [0x003E09C0], esi
	ctx.eflags.update_compare_flags_U32(X86_LOAD_U32(4065728), ctx.esi.u32);
	// jz 0x003D0243
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D0243;
	// cmp [edi+0x1A08], esi
	ctx.eflags.update_compare_flags_U32(X86_LOAD_U32(ctx.edi.u32 + 6664), ctx.esi.u32);
	// jz 0x003D0243
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D0243;
	// mov ecx, 0x01
	ctx.ecx.u32 = 0x1u;
	// jmp 0x003D0245
	goto loc_3D0245;
loc_3D0243:
	// xor ecx, ecx
	ctx.ecx.u32 ^= ctx.ecx.u32;
	ctx.eflags.update_flags_logic(ctx.ecx.u32);
loc_3D0245:
	// mov dword ptr [eax+0x08], 0x4032C
	X86_STORE_U32(ctx.eax.u32 + 8, 0x4032Cu);
	// mov [eax+0x0C], ecx
	X86_STORE_U32(ctx.eax.u32 + 12, ctx.ecx.u32);
	// mov ecx, [edi+0x08]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.edi.u32 + 8);
	// add eax, 0x10
	auto temp_3D0252_U32_OLD = ctx.eax.u32;
	auto temp_3D0252_U32_SRC = 16;
	ctx.eax.u32 = temp_3D0252_U32_OLD + temp_3D0252_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3D0252_U32_OLD, temp_3D0252_U32_SRC, ctx.eax.u32);
	// test ch, 0x01
	ctx.eflags.update_test_flags_U8(ctx.ecx.u8_hi & 0x1u);
	// jz 0x003D027B
	if ((ctx.eflags.value & X86EFlags::ZF) != 0) goto loc_3D027B;
	// mov edx, [esp+0x20]
	ctx.edx.u32 = X86_LOAD_U32(ctx.esp.u32 + 32);
	// cmp edx, [edi+0x1A14]
	ctx.eflags.update_compare_flags_U32(ctx.edx.u32, X86_LOAD_U32(ctx.edi.u32 + 6676));
	// jnz 0x003D027B
	if ((ctx.eflags.value & X86EFlags::ZF) == 0) goto loc_3D027B;
	// and ecx, 0xFFFFFEFF
	ctx.ecx.u32 &= 0xfffffeff;
	ctx.eflags.update_flags_logic(ctx.ecx.u32);
	// mov [edi+0x08], ecx
	X86_STORE_U32(ctx.edi.u32 + 8, ctx.ecx.u32);
	// mov dword ptr [eax], 0x40130
	X86_STORE_U32(ctx.eax.u32, 0x40130u);
	// mov [eax+0x04], esi
	X86_STORE_U32(ctx.eax.u32 + 4, ctx.esi.u32);
	// add eax, 0x08
	auto temp_3D0278_U32_OLD = ctx.eax.u32;
	auto temp_3D0278_U32_SRC = 8;
	ctx.eax.u32 = temp_3D0278_U32_OLD + temp_3D0278_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3D0278_U32_OLD, temp_3D0278_U32_SRC, ctx.eax.u32);
loc_3D027B:
	// mov ecx, [esp+0x20]
	ctx.ecx.u32 = X86_LOAD_U32(ctx.esp.u32 + 32);
	// mov [edi], eax
	X86_STORE_U32(ctx.edi.u32, ctx.eax.u32);
	// mov eax, [esp+0x24]
	ctx.eax.u32 = X86_LOAD_U32(ctx.esp.u32 + 36);
	// push eax
	X86_PUSH(ctx, base, ctx.eax.u32);
	// push ecx
	X86_PUSH(ctx, base, ctx.ecx.u32);
	// call 0x003D6250
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D6250(ctx, base);
	// mov edx, 0x3DE548
	ctx.edx.u32 = 0x3DE548u;
	// mov ecx, edi
	ctx.ecx.u32 = ctx.edi.u32;
	// mov [edi+0x1A0C], eax
	X86_STORE_U32(ctx.edi.u32 + 6668, ctx.eax.u32);
	// call 0x003D3310
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D3310(ctx, base);
	// mov edx, [0x003E09FC]
	ctx.edx.u32 = X86_LOAD_U32(4065788);
	// push edx
	X86_PUSH(ctx, base, ctx.edx.u32);
	// call 0x003D29B0
	ctx.esp.u32 -= 0x4; //Adjust stack pointer for call instruction
	sub_3D29B0(ctx, base);
	// pop edi
	ctx.edi.u32 = X86_POP(ctx, base);
	// pop esi
	ctx.esi.u32 = X86_POP(ctx, base);
	// pop ebp
	ctx.ebp.u32 = X86_POP(ctx, base);
	// pop ebx
	ctx.ebx.u32 = X86_POP(ctx, base);
	// add esp, 0x0C
	auto temp_3D02AE_U32_OLD = ctx.esp.u32;
	auto temp_3D02AE_U32_SRC = 12;
	ctx.esp.u32 = temp_3D02AE_U32_OLD + temp_3D02AE_U32_SRC;
	ctx.eflags.update_flags_add_U32(temp_3D02AE_U32_OLD, temp_3D02AE_U32_SRC, ctx.esp.u32);
	// ret 0x08
	ctx.esp.u32 += 0x8;  //Adjust esp for return value (0xC2 0x8)
	ctx.esp.u32 += 0x4;  //Adjust esp for return instruction
	return;
}


X86_FUNC_IMPL(__imp__sub_3D0040);
X86_FUNC_IMPL(__imp__sub_3D6250);

//Specify where it should draws (Either framebuffer surface or texture surface)
//on DX8 Xbox, SetDepthStencilSurface is merged with SetRenderTarget.
uint32_t D3DDevice_SetRenderTarget(GuestSurface* surface, GuestSurface* pNewZStencil)
{



	auto ctx = GetX86Context();
	auto base = g_memory.base;

	auto device = X86_LOAD_U32(GuestDevicePtrAddr);


	if (!g_XboxBackBuffer)
	{
		g_renderTarget->guestResource = surface->guestResource;
		surface = g_renderTarget; //the game doesn't get the back buffer on its first SetRenderTarget call so we assign ours there instead.
		g_XboxBackBuffer = surface;
		X86_STORE_U32(ctx->esp.u32 + 4, g_memory.MapVirtual(surface));
		//X86_STORE_U32(device + 0x1A14, g_memory.MapVirtual(surface));
	}

	if (!g_XboxDefaultDepthStencilSurface)
	{
		auto depthStencil = CreateSurface(640, 480, X_D3DFMT_LIN_D24S8);
		depthStencil->guestResource = pNewZStencil->guestResource;
		pNewZStencil = depthStencil;
		g_XboxDefaultDepthStencilSurface = pNewZStencil;
		X86_STORE_U32(ctx->esp.u32 + 8, g_memory.MapVirtual(pNewZStencil));
	}



	__imp__sub_3D0040(*ctx, base);
	ctx->esp.u32 -= 12;

	//if ((intptr_t)surface != RTBuggy)
	{

		RenderCommand cmd;
		cmd.type = RenderCommandType::SetRenderTarget;
		cmd.setRenderTarget.renderTarget = surface;
		g_renderQueue.enqueue(cmd);

		SetDefaultViewport((GuestDevice*)g_memory.Translate(device), surface);
	}

	RenderCommand cmd2;
	cmd2.type = RenderCommandType::SetDepthStencilSurface;
	cmd2.setDepthStencilSurface.depthStencil = pNewZStencil;
	g_renderQueue.enqueue(cmd2);

	SetDefaultViewport((GuestDevice*)g_memory.Translate(device), pNewZStencil);


	X86_STORE_U32(device + 0x954, surface->width);
	X86_STORE_U32(device + 0x958, surface->height);

	return 1;
}


static void ProcSetRenderTarget(const RenderCommand& cmd)
{
	const auto& args = cmd.setRenderTarget;


	SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_renderTarget, args.renderTarget);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.renderTargetFormat, args.renderTarget != nullptr ? args.renderTarget->format : RenderFormat::UNKNOWN);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.sampleCount, args.renderTarget != nullptr ? args.renderTarget->sampleCount : RenderSampleCount::COUNT_1);



	// When alpha to coverage is enabled, update the alpha test mode as it's dependent on sample count.
	SetAlphaTestMode((g_pipelineState.specConstants & (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0);
}

static void ProcSetDepthStencilSurface(const RenderCommand& cmd)
{
	const auto& args = cmd.setDepthStencilSurface;

	SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_depthStencil, args.depthStencil);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat, args.depthStencil != nullptr ? args.depthStencil->format : RenderFormat::UNKNOWN);
}

X86_FUNC_IMPL(__imp__sub_3D0680);
static int countBS = 0;
unsigned int D3DDevice_SetViewport(GuestViewport* viewport)
{

	//call original func first
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	__imp__sub_3D0680(*ctx, base);
	ctx->esp.u32 -= 8;


	auto device = X86_LOAD_U32(GuestDevicePtrAddr);


	RenderCommand cmd;
	cmd.type = RenderCommandType::SetViewport;

	uint32_t rtWidth = g_renderTarget->width;
	uint32_t rtHeight = g_renderTarget->height;

	//for some reason, og xbox can send int_max for width and height, likely to trigger a reset(?)
	float width = (viewport->Width == INT_MAX) ? rtWidth : viewport->Width;
	float height = (viewport->Height == INT_MAX) ? rtHeight : viewport->Height;

	cmd.setViewport.x = viewport->X;
	cmd.setViewport.y = viewport->Y;
	cmd.setViewport.width = width;
	cmd.setViewport.height = height;
	cmd.setViewport.minDepth = viewport->MinZ;
	cmd.setViewport.maxDepth = viewport->MaxZ;
	g_renderQueue.enqueue(cmd);

	//set viewport data in the device
	uint32_t resX = X86_LOAD_U32(device + 0xEE8);
	resX = resX;

	auto padding616 = X86_LOAD_F32(device + 0x95C);
	auto valB = X86_LOAD_F32(device + 0x960);
	auto padding636 = X86_LOAD_F32(device + 0x970);
	auto padding68 = X86_LOAD_U32(device + 0x954);
	auto padding7 = X86_LOAD_U32(device + 0xEF8);
	valB = valB;

	auto widthG = X86_LOAD_U32(device + 0xEE8);

	/**if (LogVSConstants)
	{
		printf("SetViewPort\n");

		printf("viewport Width %d\n", width);
		printf("viewport Height %d\n", X86_LOAD_U32(device + 0xEEC));
		printf("D3D_g_pDevice->padding7 %f\n", padding7);
		printf("D3D_g_pDevice->padding6[8] %d\n", padding68);
		printf("D3D_g_pDevice->padding6[16] %f\n", padding616);
		printf("D3D_g_pDevice->padding6[20] %f\n", valB);
		printf("D3D_g_pDevice->padding6[36] %f\n", padding636);



		float v2 = (float)X86_LOAD_U32(device + 0xEE0) * (float)padding616 * (float)padding7;
		float scaleX = (float)X86_LOAD_U32(device + 0xEE8) * (float)padding616 * 0.5f;

		printf("v2 %f\n", v2);
		printf("scaleX %f\n", scaleX);
		printf("*offset res %f\n", scaleX + v2);

		printf("\n");
	}
	X86_STORE_U32(device + 0xEE0, (uint32_t)viewport->X);
	X86_STORE_U32(device + 0xEE4, (uint32_t)viewport->Y);
	X86_STORE_U32(device + 0xEE8, (uint32_t)width);
	X86_STORE_U32(device + 0xEEC, (uint32_t)height);
	X86_STORE_F32(device + 0xEF0, viewport->MinZ);
	X86_STORE_F32(device + 0xEF4, viewport->MaxZ);*/

	return 1;
}

static bool PopulateBarriersForCopyRect(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
	bool addedAny = false;

	for (const auto surface : { renderTarget, depthStencil })
	{
		if (surface != nullptr && !surface->destinationTextures.empty())
		{

			const bool multiSampling = surface->sampleCount != RenderSampleCount::COUNT_1;

			RenderTextureLayout srcLayout;
			RenderTextureLayout dstLayout;
			bool shaderResolve = true;

			if (multiSampling && g_hardwareResolve)
			{


				// Hardware depth resolve is only supported on D3D12 when programmable sample positions are available.
				bool hardwareDepthResolveAvailable = g_hardwareDepthResolve && !g_vulkan && g_capabilities.sampleLocations;

				if (surface->format != DepthStencilFormat || hardwareDepthResolveAvailable)
				{
					srcLayout = RenderTextureLayout::RESOLVE_SOURCE;
					dstLayout = RenderTextureLayout::RESOLVE_DEST;
					shaderResolve = false;
				}
			}

			if (shaderResolve)
			{
				srcLayout = RenderTextureLayout::SHADER_READ;
				dstLayout = (surface->format == DepthStencilFormat ? RenderTextureLayout::DEPTH_WRITE : RenderTextureLayout::COLOR_WRITE);
			}

			AddBarrier(surface, srcLayout);

			for (const auto texture : surface->destinationTextures)
				AddBarrier(texture, dstLayout);

			addedAny = true;
		}
	}

	return addedAny;
}

static void ExecutePendingCopyRectsCommands(GuestSurface* renderTarget, GuestSurface* depthStencil)
{
	auto& commandList = g_commandLists[g_frame];

	for (const auto surface : { renderTarget, depthStencil })
	{
		if (surface != nullptr && !surface->destinationTextures.empty())
		{
			const bool multiSampling = surface->sampleCount != RenderSampleCount::COUNT_1;

			for (const auto texture : surface->destinationTextures)
			{
				bool shaderResolve = true;

				if (multiSampling && g_hardwareResolve)
				{
					bool hardwareDepthResolveAvailable = g_hardwareDepthResolve && !g_vulkan && g_capabilities.sampleLocations;

					if (surface->format != DepthStencilFormat || hardwareDepthResolveAvailable)
					{
						if (surface->format == DepthStencilFormat)
							commandList->resolveTextureRegion(texture->texture, 0, 0, surface->texture, nullptr, RenderResolveMode::MIN);
						else
							commandList->resolveTexture(texture->texture, surface->texture);

						shaderResolve = false;
					}
				}

				if (shaderResolve)
				{
					RenderPipeline* pipeline = nullptr;

					if (multiSampling)
					{
						uint32_t pipelineIndex = 0;

						switch (surface->sampleCount)
						{
						case RenderSampleCount::COUNT_2:
							pipelineIndex = 0;
							break;
						case RenderSampleCount::COUNT_4:
							pipelineIndex = 1;
							break;
						case RenderSampleCount::COUNT_8:
							pipelineIndex = 2;
							break;
						default:
							assert(false && "Unsupported MSAA sample count");
							break;
						}

						//TODO CHECK IF THIS IS NEEDED

						if (texture->format == DepthStencilFormat)
						{
							pipeline = g_resolveMsaaDepthPipelines[pipelineIndex].get();
						}
						else
						{
							auto& resolveMsaaColorPipeline = g_resolveMsaaColorPipelines[surface->format][pipelineIndex];
							if (resolveMsaaColorPipeline == nullptr)
							{
								RenderGraphicsPipelineDesc desc;
								desc.pipelineLayout = g_pipelineLayout.get();
								desc.vertexShader = g_copyShader.get();
								desc.pixelShader = g_resolveMsaaColorShaders[pipelineIndex].get();
								desc.renderTargetFormat[0] = texture->format;
								desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
								desc.renderTargetCount = 1;
								resolveMsaaColorPipeline = g_device->createGraphicsPipeline(desc);
							}

							pipeline = resolveMsaaColorPipeline.get();
						}
					}
					else
					{
						if (texture->format == DepthStencilFormat)
						{
							pipeline = g_copyDepthPipeline.get();
						}
						else
						{
							auto& copyColorPipeline = g_copyColorPipelines[surface->format];
							if (copyColorPipeline == nullptr)
							{
								RenderGraphicsPipelineDesc desc;
								desc.pipelineLayout = g_pipelineLayout.get();
								desc.vertexShader = g_copyShader.get();
								desc.pixelShader = g_copyColorShader.get();
								desc.renderTargetFormat[0] = texture->format;
								desc.renderTargetBlend[0] = RenderBlendDesc::Copy();
								desc.renderTargetCount = 1;
								copyColorPipeline = g_device->createGraphicsPipeline(desc);
							}

							pipeline = copyColorPipeline.get();
						}
					}

					if (texture->framebuffer == nullptr)
					{
						if (texture->format == DepthStencilFormat)
						{
							RenderFramebufferDesc desc;
							desc.depthAttachment = texture->texture;
							texture->framebuffer = g_device->createFramebuffer(desc);
						}
						else
						{
							RenderFramebufferDesc desc;
							desc.colorAttachments = const_cast<const RenderTexture**>(&texture->texture);
							desc.colorAttachmentsCount = 1;
							texture->framebuffer = g_device->createFramebuffer(desc);
						}
					}

					if (g_framebuffer != texture->framebuffer.get())
					{
						commandList->setFramebuffer(texture->framebuffer.get());
						g_framebuffer = texture->framebuffer.get();
					}

					commandList->setPipeline(pipeline);
					commandList->setViewports(RenderViewport(0.0f, 0.0f, float(texture->width), float(texture->height), 0.0f, 1.0f));
					commandList->setScissors(RenderRect(0, 0, texture->width, texture->height));
					commandList->setGraphicsPushConstants(0, &surface->descriptorIndex, 0, sizeof(uint32_t));
					commandList->drawInstanced(6, 1, 0, 0);

					g_dirtyStates.renderTargetAndDepthStencil = true;
					g_dirtyStates.viewport = true;
					g_dirtyStates.pipelineState = true;
					g_dirtyStates.scissorRect = true;

					if (g_vulkan)
					{
						g_dirtyStates.vertexShaderConstants = true; // The push constant call invalidates vertex shader constants.
						g_dirtyStates.depthBias = true; // Static depth bias in copy pipeline invalidates dynamic depth bias.
					}
				}

				texture->sourceSurface = nullptr;

				// Check if any texture slots had this texture assigned, and make it point back at the original texture.
				for (uint32_t i = 0; i < std::size(g_textures); i++)
				{
					if (g_textures[i] == texture)
						SetTextureInRenderThread(i, texture);
				}
			}

			surface->destinationTextures.clear();
		}
	}
}

static void ProcExecutePendingCopyRectsCommands(const RenderCommand& cmd)
{
	bool foundAny = false;

	for (const auto surface : g_pendingSurfaceCopies)
	{
		//todo check if this is correct, are Depth stencil textures in this game guaranteed to be transient?
		if (surface->format != DepthStencilFormat)
			foundAny |= PopulateBarriersForCopyRect(surface, nullptr);
	}

	if (foundAny)
	{
		FlushBarriers();

		for (const auto surface : g_pendingSurfaceCopies)
		{
			if (surface->format != DepthStencilFormat)
				ExecutePendingCopyRectsCommands(surface, nullptr);

			for (const auto texture : surface->destinationTextures)
				texture->sourceSurface = nullptr;

			surface->destinationTextures.clear();
		}
	}

	g_pendingSurfaceCopies.clear();
	g_pendingMsaaResolves.clear();
}

static void SetFramebuffer(GuestSurface* renderTarget, GuestSurface* depthStencil, bool settingForClear)
{


	if (settingForClear || g_dirtyStates.renderTargetAndDepthStencil)
	{
		GuestSurface* framebufferContainer = nullptr;
		RenderTexture* framebufferKey = nullptr;

		if (renderTarget != nullptr && depthStencil != nullptr)
		{
			framebufferContainer = depthStencil; // Backbuffer texture changes per frame so we can't use the depth stencil as the key.
			framebufferKey = renderTarget->texture;
		}
		else if (renderTarget != nullptr && depthStencil == nullptr)
		{
			framebufferContainer = renderTarget;
			framebufferKey = renderTarget->texture; // Backbuffer texture changes per frame so we can't assume nullptr for it.
		}
		else if (renderTarget == nullptr && depthStencil != nullptr)
		{
			framebufferContainer = depthStencil;
			framebufferKey = nullptr;
		}

		auto& commandList = g_commandLists[g_frame];

		if (framebufferContainer != nullptr)
		{
			auto& framebuffer = framebufferContainer->framebuffers[framebufferKey];

			if (framebuffer == nullptr)
			{
				RenderFramebufferDesc desc;

				if (renderTarget != nullptr)
				{
					desc.colorAttachments = const_cast<const RenderTexture**>(&renderTarget->texture);
					desc.colorAttachmentsCount = 1;
				}

				if (depthStencil != nullptr)
					desc.depthAttachment = depthStencil->texture;

				framebuffer = g_device->createFramebuffer(desc);
			}

			if (g_framebuffer != framebuffer.get())
			{
				commandList->setFramebuffer(framebuffer.get());
				g_framebuffer = framebuffer.get();
			}
		}
		else if (g_framebuffer != nullptr)
		{
			commandList->setFramebuffer(nullptr);
			g_framebuffer = nullptr;
		}

		if (g_framebuffer != nullptr)
		{
			SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.halfPixelOffsetX, 1.0f / float(g_framebuffer->getWidth()));
			SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.halfPixelOffsetY, -1.0f / float(g_framebuffer->getHeight()));
		}

		g_dirtyStates.renderTargetAndDepthStencil = settingForClear;
	}
}

static void ProcClear(const RenderCommand& cmd)
{
	const auto& args = cmd.clear;

	if (PopulateBarriersForCopyRect(g_renderTarget, g_depthStencil))
	{
		FlushBarriers();
		ExecutePendingCopyRectsCommands(g_renderTarget, g_depthStencil);
	}

	AddBarrier(g_renderTarget, RenderTextureLayout::COLOR_WRITE);
	AddBarrier(g_depthStencil, RenderTextureLayout::DEPTH_WRITE);
	FlushBarriers();

	bool canClearInOnePass = (g_renderTarget == nullptr) || (g_depthStencil == nullptr) ||
		(g_renderTarget->width == g_depthStencil->width && g_renderTarget->height == g_depthStencil->height);

	if (canClearInOnePass)
		SetFramebuffer(g_renderTarget, g_depthStencil, true);

	auto& commandList = g_commandLists[g_frame];

	if (g_renderTarget != nullptr && (args.flags & X_D3DCLEAR_TARGET) != 0)
	{
		if (!canClearInOnePass)
			SetFramebuffer(g_renderTarget, nullptr, true);

		commandList->clearColor(0, RenderColor(args.color[0], args.color[1], args.color[2], args.color[3]));
	}

	if (g_depthStencil != nullptr && (args.flags & X_D3DCLEAR_ZBUFFER) != 0)
	{
		if (!canClearInOnePass)
			SetFramebuffer(nullptr, g_depthStencil, true);

		commandList->clearDepth(true, args.z);
	}
}


//Clears one or more surfaces such as a render target, multiple render targets, a stencil buffer, and a depth buffer.
unsigned int D3DDevice_Clear(uint32_t count, _D3DRECT* pRects, uint32_t flags, uint32_t Color, float Z, uint32_t Stencil)
{

	RenderCommand cmd;
	cmd.type = RenderCommandType::Clear;
	cmd.clear.flags = flags;


	//extract the RGBA components to convert the color to float
	float color[4] = { ((Color >> 16) & 0xFF) / 255.f,
				  ((Color >> 8) & 0xFF) / 255.f, (Color & 0xFF) / 255.f,
				  ((Color >> 24) & 0xFF) / 255.f };

	cmd.clear.color[0] = color[0];
	cmd.clear.color[1] = color[1];
	cmd.clear.color[2] = color[2];
	cmd.clear.color[3] = color[3];


	cmd.clear.z = float(Z);
	g_renderQueue.enqueue(cmd);

	return 1;
}

//D3DDevice_SetTransform
X86_FUNC_IMPL(__imp__sub_3CFCD0);
X86_FUNC(sub_3CFCD0)
{
	X_D3DTRANSFORMSTATETYPE State = (X_D3DTRANSFORMSTATETYPE)X86_LOAD_U32(ctx.esp.u32 + 4);
	auto matrixAddr = X86_LOAD_U32(ctx.esp.u32 + 8);
	D3DMATRIX* pMatrix = (D3DMATRIX*)(base + matrixAddr);

	//we let the original code run first (todo check if that's needed)
	__imp__sub_3CFCD0(ctx, base);


	g_Transforms[State] = *pMatrix;

	if (State == X_D3DTS_VIEW)
	{
		bWorldViewDirty.fill(true);
	}
	if ((X_D3DTS_WORLD <= State) && (State <= X_D3DTS_WORLD3))
	{
		bWorldViewDirty[State - X_D3DTS_WORLD] = true;
	}
}

//D3DDevice_SetScreenSpaceOffset
//Direct3D before version 10 had this pecularity called “half pixel offset”, where viewport coordinates are shifted by half a pixel 
//compared to anything else (OpenGL, D3D10+, Metal etc.). 
//we don't need that here as we use DX12 / Vulkan.
X86_FUNC_IMPL(__imp__sub_3D1320);
X86_FUNC(sub_3D1320)
{
	__imp__sub_3D1320(ctx, base);
	/**auto device = X86_LOAD_U32(GuestDevicePtrAddr);
	float offsetA = X86_LOAD_F32(device + 0xEF8);
	float offsetB = X86_LOAD_F32(device + 0xEFC);

	//remove the half pixel offset
	offsetA -= 0.53125;
	offsetB -= 0.53125;
	X86_STORE_U32(device + 0xEF8, offsetA);
	X86_STORE_U32(device + 0xEFC, offsetB);*/
}


DWORD g_Xbox_VertexShaderConstantMode = 1;

//SetShaderConstantMode
X86_FUNC_IMPL(__imp__sub_3D2010);
X86_FUNC(sub_3D2010)
{
	g_Xbox_VertexShaderConstantMode = X86_LOAD_U32(ctx.esp.u32 + 4);
	__imp__sub_3D2010(ctx, base);
}

void SetVertexAttribute(int32_t Register, FLOAT a, FLOAT b, FLOAT c, FLOAT d)
{

	vsh_constants_dirty[HOST_D3DVS_CONSTREG_VREGDEFAULTS_BASE + Register] = true; 	//vRegisterDefaultValues aka 192 + i
	g_vertexConstantsDirtyFlags = true;

	float attribute_floats[4];
	attribute_floats[0] = a;
	attribute_floats[1] = b;
	attribute_floats[2] = c;
	attribute_floats[3] = d;

	// Also, write the given register value to a matching host vertex shader constant
	// This allows us to implement Xbox functionality where SetVertexData4f can be used to specify attributes
	// not present in the vertex declaration.
	// We use range 193 and up to store these values, as Xbox shaders stop at c192!
	CopyVertexShaderConstant(HOST_D3DVS_CONSTREG_VREGDEFAULTS_BASE + Register, (DWORD*)attribute_floats, 1);
}

static float g_ZScale = 1.0f; //todo check if it needs to be updated

// Get viewport offset and scale values intended to match the XDK calculations
void GetXboxViewportOffsetAndScale(float(&vOffset)[4], float(&vScale)[4])
{
	// Antialiasing mode affects the viewport offset and scale
	float aaScaleX = 1.0f;
	float aaScaleY = 1.0f;
	float aaOffsetX = 0.5f;
	float aaOffsetY = 0.5f;
	//GetScreenScaleFactors(aaScaleX, aaScaleY); //todo check if this is needed

	float scaledX = g_viewport.x * aaScaleX;
	float scaledY = g_viewport.y * aaScaleY;
	float scaledWidth = g_viewport.width * aaScaleX;
	float scaledHeight = g_viewport.height * aaScaleY;

	// D3D9 viewport info
	// https://docs.microsoft.com/en-us/windows/win32/direct3d9/viewports-and-clipping
	// Unlike D3D9, Xbox should scale by the z buffer scale

	auto zRange = g_viewport.maxDepth - g_viewport.minDepth;
	vScale[0] = scaledWidth / 2;
	vScale[1] = -scaledHeight / 2;
	vScale[2] = zRange * g_ZScale;
	vScale[3] = 1;
	vOffset[0] = scaledWidth / 2 + scaledX;
	vOffset[1] = scaledHeight / 2 + scaledY;
	vOffset[2] = g_viewport.minDepth * g_ZScale;
	vOffset[3] = 0;
}

void UpdateHostViewPortOffsetAndScaleConstants()
{
	float vScaleOffset[2][4]; // 0 - scale 1 - offset
	GetXboxViewportOffsetAndScale(vScaleOffset[1], vScaleOffset[0]);


	// Xbox outputs vertex positions in rendertarget pixel coordinate space, with non-normalized Z
	// e.g. 0 < x < 640 and 0 < y < 480
	// We want to scale it back to normalized device coordinates i.e. XY are (-1, +1) and Z is (0, 1)

	// The screenspace is a combination of the rendertarget
	// and various scale factors
	// Get the rendertarget width and height
	float xboxRenderTargetWidth = g_renderTarget->width;
	float xboxRenderTargetHeight = g_renderTarget->height;
	//GetRenderTargetBaseDimensions(xboxRenderTargetWidth, xboxRenderTargetHeight);

	float screenScaleX = 1.0f;
	float screenScaleY = 1.0f;
	float aaOffsetX = 0.5f;
	float aaOffsetY = 0.5f;
	//GetScreenScaleFactors(screenScaleX, screenScaleY);
	//GetMultiSampleOffset(aaOffsetX, aaOffsetY);


	float xboxScreenspaceWidth = xboxRenderTargetWidth * screenScaleX;
	float xboxScreenspaceHeight = xboxRenderTargetHeight * screenScaleY;

	// Passthrough should range 0 to 1, instead of 0 to zbuffer depth
	float zOutputScale = 1.0f; //g_Xbox_VertexShaderMode == VertexShaderMode::Passthrough ? 1 : g_ZScale;

	float screenspaceScale[4] = { xboxScreenspaceWidth / 2,  -xboxScreenspaceHeight / 2, zOutputScale, 1 };
	float screenspaceOffset[4] = { xboxScreenspaceWidth / 2 + aaOffsetX, xboxScreenspaceHeight / 2 + aaOffsetY, 0, 0 };

	CopyVertexShaderConstant(HOST_D3DVS_SCREENSPACE_SCALE_BASE, (DWORD*)screenspaceScale, HOST_D3DVS_NORMALIZE_SCALE_SIZE);
	CopyVertexShaderConstant(HOST_D3DVS_SCREENSPACE_OFFSET_BASE, (DWORD*)screenspaceOffset, HOST_D3DVS_NORMALIZE_OFFSET_SIZE);
	// Store viewport offset and scale in constant registers 58 (c-38) and
	// 59 (c-37) used for screen space transformation.
	// We only do this if X_D3DSCM_NORESERVEDCONSTANTS is not set,
	// since enabling this flag frees up these registers for shader use
	if (!(g_Xbox_VertexShaderConstantMode & X_D3DSCM_NORESERVEDCONSTANTS)) {
		CopyVertexShaderConstant(X_D3DSCM_RESERVED_CONSTANT_SCALE_CORRECTED, reinterpret_cast<DWORD*>(vScaleOffset), 2);
	}
}


DWORD* D3D_CommonSetDebugRegisters()
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



unsigned int D3D_LazySetPointParams(int a1)
{
	return 0;
}

int RW_SetTextureStageStates(int a1, int a2)
{
	return 0;
}

int* D3DDevice_SetPalette(int a1, DWORD* a2)
{
	return 0;
}


//rwD3D8RasterShowRaster (call Device_Swap which replaces Present)
/*/X86_FUNC_IMPL(__imp__sub_2DDC00);
X86_FUNC(sub_2DDC00)
{

	ctx.esp.u32 += 4;
}*/



int D3DDevice_Swap(int32_t a1)
{
	Video::Present();
	return 0;
}


unsigned int D3D_SetFence(char a2)
{
	return 0;
}


//potentially corrupt the stack to do check
/**X86_FUNC_IMPL(__imp__sub_1575C0);
X86_FUNC(sub_1575C0)
{
	ctx.esp.u32 += 8;
	return;
	//__imp__sub_1575C0(ctx, base);
}*/

struct LocalRenderCommandQueue
{
	RenderCommand commands[20];
	uint32_t count = 0;

	RenderCommand& enqueue()
	{
		assert(count < std::size(commands));
		return commands[count++];
	}

	void submit()
	{
		g_renderQueue.enqueue_bulk(commands, count);
	}
};

static void SetSurface(uint32_t index, GuestSurface* surface)
{
	AddBarrier(surface, RenderTextureLayout::SHADER_READ);

	SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture2DIndices[index], surface->descriptorIndex);
	SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.texture3DIndices[index], uint32_t(TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D));
	SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.textureCubeIndices[index], uint32_t(TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE));
}


static void ProcSetTexture(const RenderCommand& cmd)
{
	const auto& args = cmd.setTexture;

	// If a pending copy operation is detected, set the source surface. The indices will be fixed later if flushing is necessary.
	bool shouldSetTexture = true;
	if (args.texture != nullptr)
	{
		if (args.texture->sourceSurface != nullptr)
		{
			// MSAA surfaces need to be resolved and cannot be used directly.
			if (args.texture->sourceSurface->sampleCount != RenderSampleCount::COUNT_1)
			{
				g_pendingMsaaResolves.emplace(args.texture->sourceSurface);
			}
			else
			{
				SetSurface(args.index, args.texture->sourceSurface);
				shouldSetTexture = false;
			}
		}

	}


	if (shouldSetTexture)
		SetTextureInRenderThread(args.index, args.texture);

	g_textures[args.index] = args.texture;
}

static RenderShader* GetOrLinkShader(GuestShader* guestShader, uint32_t specConstants)
{
	if (g_vulkan ||
		guestShader->shaderCacheEntry == nullptr ||
		guestShader->shaderCacheEntry->specConstantsMask == 0)
	{
		std::lock_guard lock(guestShader->mutex);

		if (guestShader->shader == nullptr)
		{
			assert(guestShader->shaderCacheEntry != nullptr);

			if (g_vulkan) //todo install smolv, will likely come with a potential XShaderRecomp
			{
				/**auto compressedSpirvData = g_shaderCache.get() + guestShader->shaderCacheEntry->spirvOffset;

				std::vector<uint8_t> decoded(smolv::GetDecodedBufferSize(compressedSpirvData, guestShader->shaderCacheEntry->spirvSize));
				bool result = smolv::Decode(compressedSpirvData, guestShader->shaderCacheEntry->spirvSize, decoded.data(), decoded.size());
				assert(result);

				guestShader->shader = g_device->createShader(decoded.data(), decoded.size(), "main", RenderShaderFormat::SPIRV);*/
			}
			else
			{
				guestShader->shader = g_device->createShader(g_shaderCache.get() + guestShader->shaderCacheEntry->dxilOffset,
					guestShader->shaderCacheEntry->dxilSize, "main", RenderShaderFormat::DXIL);
			}
		}

		return guestShader->shader.get();
	}

	specConstants &= guestShader->shaderCacheEntry->specConstantsMask;

	RenderShader* shader;
	{
		std::lock_guard lock(guestShader->mutex);
		shader = guestShader->linkedShaders[specConstants].get();
	}

#ifdef SHADOW_RECOMP_D3D12
	if (shader == nullptr)
	{
		static Mutex g_compiledSpecConstantLibraryBlobMutex;
		static ankerl::unordered_dense::map<uint32_t, ComPtr<IDxcBlob>> g_compiledSpecConstantLibraryBlobs;

		thread_local ComPtr<IDxcCompiler3> s_dxcCompiler;
		thread_local ComPtr<IDxcLinker> s_dxcLinker;
		thread_local ComPtr<IDxcUtils> s_dxcUtils;

		wchar_t specConstantsLibName[0x100];
		swprintf_s(specConstantsLibName, L"SpecConstants_%d", specConstants);

		ComPtr<IDxcBlob> specConstantLibraryBlob;
		{
			std::lock_guard lock(g_compiledSpecConstantLibraryBlobMutex);
			specConstantLibraryBlob = g_compiledSpecConstantLibraryBlobs[specConstants];
		}

		if (specConstantLibraryBlob == nullptr)
		{
			if (s_dxcCompiler == nullptr)
			{
				HRESULT hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(s_dxcCompiler.GetAddressOf()));
				assert(SUCCEEDED(hr) && s_dxcCompiler != nullptr);
			}

			char libraryHlsl[0x100];
			sprintf_s(libraryHlsl, "export uint g_SpecConstants() { return %d; }", specConstants);

			DxcBuffer buffer{};
			buffer.Ptr = libraryHlsl;
			buffer.Size = strlen(libraryHlsl);

			const wchar_t* args[1];
			args[0] = L"-T lib_6_3";

			ComPtr<IDxcResult> result;
			HRESULT hr = s_dxcCompiler->Compile(&buffer, args, std::size(args), nullptr, IID_PPV_ARGS(result.GetAddressOf()));
			assert(SUCCEEDED(hr) && result != nullptr);

			hr = result->GetResult(specConstantLibraryBlob.GetAddressOf());
			assert(SUCCEEDED(hr) && specConstantLibraryBlob != nullptr);

			std::lock_guard lock(g_compiledSpecConstantLibraryBlobMutex);
			g_compiledSpecConstantLibraryBlobs.emplace(specConstants, specConstantLibraryBlob);
		}

		if (s_dxcLinker == nullptr)
		{
			HRESULT hr = DxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(s_dxcLinker.GetAddressOf()));
			assert(SUCCEEDED(hr) && s_dxcLinker != nullptr);
		}

		s_dxcLinker->RegisterLibrary(specConstantsLibName, specConstantLibraryBlob.Get());

		wchar_t shaderLibName[0x100];
		swprintf_s(shaderLibName, L"Shader_%d", guestShader->shaderCacheEntry->dxilOffset);

		ComPtr<IDxcBlobEncoding> shaderLibraryBlob;
		{
			std::lock_guard lock(guestShader->mutex);
			shaderLibraryBlob = guestShader->libraryBlob;
		}

		if (shaderLibraryBlob == nullptr)
		{
			if (s_dxcUtils == nullptr)
			{
				HRESULT hr = DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(s_dxcUtils.GetAddressOf()));
				assert(SUCCEEDED(hr) && s_dxcUtils != nullptr);
			}

			HRESULT hr = s_dxcUtils->CreateBlobFromPinned(
				g_shaderCache.get() + guestShader->shaderCacheEntry->dxilOffset,
				guestShader->shaderCacheEntry->dxilSize,
				DXC_CP_ACP,
				shaderLibraryBlob.GetAddressOf());

			assert(SUCCEEDED(hr) && shaderLibraryBlob != nullptr);

			std::lock_guard lock(guestShader->mutex);
			guestShader->libraryBlob = shaderLibraryBlob;
		}

		s_dxcLinker->RegisterLibrary(shaderLibName, shaderLibraryBlob.Get());

		const wchar_t* libraryNames[] = { specConstantsLibName, shaderLibName };

		ComPtr<IDxcOperationResult> result;
		HRESULT hr = s_dxcLinker->Link(L"main", guestShader->type == ResourceType::VertexShader ? L"vs_6_0" : L"ps_6_0",
			libraryNames, std::size(libraryNames), nullptr, 0, result.GetAddressOf());

		assert(SUCCEEDED(hr) && result != nullptr);

		ComPtr<IDxcBlob> blob;
		hr = result->GetResult(blob.GetAddressOf());
		assert(SUCCEEDED(hr) && blob != nullptr);

		{
			std::lock_guard lock(guestShader->mutex);

			auto& linkedShader = guestShader->linkedShaders[specConstants];
			if (linkedShader == nullptr)
			{
				linkedShader = g_device->createShader(blob->GetBufferPointer(), blob->GetBufferSize(), "main", RenderShaderFormat::DXIL);
				guestShader->shaderBlobs.push_back(std::move(blob));
			}

			shader = linkedShader.get();
		}
	}
#endif

	return shader;
}

static void SanitizePipelineState(PipelineState& pipelineState)
{
	if (!pipelineState.zEnable && !pipelineState.stencilEnable)
	{
		pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;
	}

	if (!pipelineState.zEnable)
	{
		pipelineState.zWriteEnable = false;
		pipelineState.zFunc = RenderComparisonFunction::LESS;
		pipelineState.slopeScaledDepthBias = 0.0f;
		pipelineState.depthBias = 0;
		pipelineState.depthStencilFormat = RenderFormat::UNKNOWN;
	}

	if (!pipelineState.stencilEnable)
	{
		pipelineState.stencilFail = RenderStencilOp::KEEP;
		pipelineState.stencilZFail = RenderStencilOp::KEEP;
		pipelineState.stencilPass = RenderStencilOp::KEEP;
		pipelineState.stencilMask = 0xFFFFFFFF;
		pipelineState.stencilWriteMask = 0xFFFFFFFF;
		pipelineState.stencilRef = 0;
	}

	if (pipelineState.slopeScaledDepthBias == 0.0f)
		pipelineState.slopeScaledDepthBias = 0.0f; // Remove sign.

	if (!pipelineState.colorWriteEnable)
	{
		pipelineState.alphaBlendEnable = false;
		pipelineState.renderTargetFormat = RenderFormat::UNKNOWN;
	}

	if (!pipelineState.alphaBlendEnable)
	{
		pipelineState.srcBlend = RenderBlend::ONE;
		pipelineState.destBlend = RenderBlend::ZERO;
		pipelineState.blendOp = RenderBlendOperation::ADD;
		pipelineState.srcBlendAlpha = RenderBlend::ONE;
		pipelineState.destBlendAlpha = RenderBlend::ZERO;
		pipelineState.blendOpAlpha = RenderBlendOperation::ADD;
	}

	for (size_t i = 0; i < 16; i++)
	{
		if (!pipelineState.vertexDeclaration->vertexStreams[i])
			pipelineState.vertexStrides[i] = 0;
	}

	uint32_t specConstantsMask = 0;
	/**if (pipelineState.vertexShader->shaderCacheEntry != nullptr)
		specConstantsMask |= pipelineState.vertexShader->shaderCacheEntry->specConstantsMask;*/

	if (pipelineState.pixelShader != nullptr && pipelineState.pixelShader->shaderCacheEntry != nullptr)
		specConstantsMask |= pipelineState.pixelShader->shaderCacheEntry->specConstantsMask;

	pipelineState.specConstants &= specConstantsMask;
}

static std::unique_ptr<RenderPipeline> CreateGraphicsPipeline(const PipelineState& pipelineState)
{
#ifdef ASYNC_PSO_DEBUG
	++g_pipelinesCurrentlyCompiling;
#endif

	RenderGraphicsPipelineDesc desc;
	desc.pipelineLayout = g_pipelineLayout.get();

	if (pipelineState.vertexShaderType == VertexShaderMode::Passthrough)
	{
		desc.vertexShader = PassthroughVS.get();
	}
	else if (pipelineState.vertexShaderType == VertexShaderMode::FixedFunction)
	{
		desc.vertexShader = FixedFuncVS.get();
	}
	else
	{
		desc.vertexShader = GetOrLinkShader(pipelineState.vertexShader, pipelineState.specConstants);
	}

	desc.pixelShader = FixedFuncPS.get();//pipelineState.pixelShader != nullptr ? GetOrLinkShader(pipelineState.pixelShader, pipelineState.specConstants) : nullptr;
	desc.depthFunction = pipelineState.zFunc;
	desc.depthEnabled = pipelineState.zEnable;
	desc.depthWriteEnabled = pipelineState.zWriteEnable;
	desc.depthBias = pipelineState.depthBias;
	desc.slopeScaledDepthBias = pipelineState.slopeScaledDepthBias;
	desc.dynamicDepthBiasEnabled = g_capabilities.dynamicDepthBias;
	desc.depthClipEnabled = true;
	desc.primitiveTopology = pipelineState.primitiveTopology;
	desc.cullMode = pipelineState.cullMode;
	desc.renderTargetFormat[0] = pipelineState.renderTargetFormat;
	desc.renderTargetBlend[0].blendEnabled = pipelineState.alphaBlendEnable;
	desc.renderTargetBlend[0].srcBlend = pipelineState.srcBlend;
	desc.renderTargetBlend[0].dstBlend = pipelineState.destBlend;
	desc.renderTargetBlend[0].blendOp = pipelineState.blendOp;
	desc.renderTargetBlend[0].srcBlendAlpha = pipelineState.srcBlendAlpha;
	desc.renderTargetBlend[0].dstBlendAlpha = pipelineState.destBlendAlpha;
	desc.renderTargetBlend[0].blendOpAlpha = pipelineState.blendOpAlpha;
	desc.renderTargetBlend[0].renderTargetWriteMask = pipelineState.colorWriteEnable;
	desc.renderTargetCount = pipelineState.renderTargetFormat != RenderFormat::UNKNOWN ? 1 : 0;
	desc.depthTargetFormat = pipelineState.depthStencilFormat;
	desc.multisampling.sampleCount = pipelineState.sampleCount;
	desc.alphaToCoverageEnabled = pipelineState.enableAlphaToCoverage;
	desc.inputElements = pipelineState.vertexDeclaration->inputElements.get();
	desc.inputElementsCount = pipelineState.vertexDeclaration->inputElementCount;


	RenderSpecConstant specConstant{};
	specConstant.value = pipelineState.specConstants;

	if (pipelineState.specConstants != 0)
	{
		desc.specConstants = &specConstant;
		desc.specConstantsCount = 1;
	}

	RenderInputSlot inputSlots[16]{};
	uint32_t inputSlotIndices[16]{};
	uint32_t inputSlotCount = 0;

	for (size_t i = 0; i < pipelineState.vertexDeclaration->inputElementCount; i++)
	{
		auto& inputElement = pipelineState.vertexDeclaration->inputElements[i];
		auto& inputSlotIndex = inputSlotIndices[inputElement.slotIndex];

		if (inputSlotIndex == NULL)
			inputSlotIndex = ++inputSlotCount;

		auto& inputSlot = inputSlots[inputSlotIndex - 1];
		inputSlot.index = inputElement.slotIndex;
		inputSlot.stride = pipelineState.vertexStrides[inputElement.slotIndex];
		inputSlot.classification = RenderInputSlotClassification::PER_VERTEX_DATA;
	}

	desc.inputSlots = inputSlots;
	desc.inputSlotsCount = inputSlotCount;

	auto pipeline = g_device->createGraphicsPipeline(desc);

#ifdef ASYNC_PSO_DEBUG
	--g_pipelinesCurrentlyCompiling;
#endif

	return pipeline;
}

static RenderPipeline* CreateGraphicsPipelineInRenderThread(PipelineState pipelineState)
{
	SanitizePipelineState(pipelineState);

	XXH64_hash_t hash = XXH3_64bits(&pipelineState, sizeof(pipelineState));
	auto& pipeline = g_pipelines[hash];
	if (pipeline == nullptr)
	{
		pipeline = CreateGraphicsPipeline(pipelineState);

#ifdef ASYNC_PSO_DEBUG
		bool loading = *SWA::SGlobals::ms_IsLoading;

		if (loading)
			++g_pipelinesCreatedAsynchronously;
		else
			++g_pipelinesCreatedInRenderThread;

		pipeline->setName(fmt::format("{} {} {} {:X}", loading ? "ASYNC" : "",
			pipelineState.vertexShader->name, pipelineState.pixelShader != nullptr ? pipelineState.pixelShader->name : "<none>", hash));

		if (!loading)
		{
			std::lock_guard lock(g_debugMutex);
			g_pipelineDebugText = fmt::format(
				"PipelineState {:X}:\n"
				"  vertexShader: {}\n"
				"  pixelShader: {}\n"
				"  vertexDeclaration: {:X}\n"
				"  instancing: {}\n"
				"  zEnable: {}\n"
				"  zWriteEnable: {}\n"
				"  srcBlend: {}\n"
				"  destBlend: {}\n"
				"  cullMode: {}\n"
				"  zFunc: {}\n"
				"  alphaBlendEnable: {}\n"
				"  blendOp: {}\n"
				"  slopeScaledDepthBias: {}\n"
				"  depthBias: {}\n"
				"  srcBlendAlpha: {}\n"
				"  destBlendAlpha: {}\n"
				"  blendOpAlpha: {}\n"
				"  colorWriteEnable: {:X}\n"
				"  primitiveTopology: {}\n"
				"  vertexStrides[0]: {}\n"
				"  vertexStrides[1]: {}\n"
				"  vertexStrides[2]: {}\n"
				"  vertexStrides[3]: {}\n"
				"  renderTargetFormat: {}\n"
				"  depthStencilFormat: {}\n"
				"  sampleCount: {}\n"
				"  enableAlphaToCoverage: {}\n"
				"  specConstants: {:X}\n",
				hash,
				pipelineState.vertexShader->name,
				pipelineState.pixelShader != nullptr ? pipelineState.pixelShader->name : "<none>",
				reinterpret_cast<size_t>(pipelineState.vertexDeclaration),
				pipelineState.instancing,
				pipelineState.zEnable,
				pipelineState.zWriteEnable,
				magic_enum::enum_name(pipelineState.srcBlend),
				magic_enum::enum_name(pipelineState.destBlend),
				magic_enum::enum_name(pipelineState.cullMode),
				magic_enum::enum_name(pipelineState.zFunc),
				pipelineState.alphaBlendEnable,
				magic_enum::enum_name(pipelineState.blendOp),
				pipelineState.slopeScaledDepthBias,
				pipelineState.depthBias,
				magic_enum::enum_name(pipelineState.srcBlendAlpha),
				magic_enum::enum_name(pipelineState.destBlendAlpha),
				magic_enum::enum_name(pipelineState.blendOpAlpha),
				pipelineState.colorWriteEnable,
				magic_enum::enum_name(pipelineState.primitiveTopology),
				pipelineState.vertexStrides[0],
				pipelineState.vertexStrides[1],
				pipelineState.vertexStrides[2],
				pipelineState.vertexStrides[3],
				magic_enum::enum_name(pipelineState.renderTargetFormat),
				magic_enum::enum_name(pipelineState.depthStencilFormat),
				pipelineState.sampleCount,
				pipelineState.enableAlphaToCoverage,
				pipelineState.specConstants)
				+ g_pipelineDebugText;
		}
#endif

#ifdef PSO_CACHING
		std::lock_guard lock(g_pipelineCacheMutex);
		g_pipelineStatesToCache.emplace(hash, pipelineState);
#endif
	}

	return pipeline.get();
}

static RenderTextureAddressMode ConvertTextureAddressMode(size_t value)
{
	switch (value)
	{
	case 0: //DX8 can set to 0, it's safe to default to wrap.
	case D3DTADDRESS_WRAP:
		return RenderTextureAddressMode::WRAP;
	case D3DTADDRESS_MIRROR:
		return RenderTextureAddressMode::MIRROR;
	case D3DTADDRESS_CLAMP:
		return RenderTextureAddressMode::CLAMP;
	case D3DTADDRESS_BORDER:
		return RenderTextureAddressMode::BORDER;
	default:
		assert(false && "Unknown texture address mode");
		return RenderTextureAddressMode::UNKNOWN;
	}
}

static RenderFilter ConvertTextureFilter(uint32_t value)
{
	switch (value)
	{
	case X_D3DTEXF_POINT:
	case X_D3DTEXF_NONE:
		return RenderFilter::NEAREST;
	case X_D3DTEXF_LINEAR:
		return RenderFilter::LINEAR;
	default:
		assert(false && "Unknown texture filter");
		return RenderFilter::UNKNOWN;
	}
}

static RenderBorderColor ConvertBorderColor(uint32_t value)
{
	//Extract ARGB
	uint8_t a = (value >> 24) & 0xFF;
	uint8_t r = (value >> 16) & 0xFF;
	uint8_t g = (value >> 8) & 0xFF;
	uint8_t b = value & 0xFF;

	if (a == 0x00) // Fully transparent
		return RenderBorderColor::TRANSPARENT_BLACK;

	if (r == 0 && g == 0 && b == 0)
		return RenderBorderColor::OPAQUE_BLACK;

	if (r == 255 && g == 255 && b == 255)
		return RenderBorderColor::OPAQUE_WHITE;


	assert(false && "Unknown border color");
	return RenderBorderColor::UNKNOWN;
}


bool isTextureStageStateDirty()
{
	for (uint32_t i = 0; i < 4; i++)
	{
		for (uint32_t j = 0; j < X_D3DTSS_MAX; j++)
		{
			auto lastState = g_dirty_texture_stage_states[i * X_D3DTSS_MAX + j];
			int curState = g_texture_stage_states[i * X_D3DTSS_MAX + j];

			if (lastState != curState)
				return true;
		}
	}

	return false;
}

inline DWORD GetXboxCommonResourceType(const xbox::dword_xt XboxResource_Common)
{
#define X_D3DCOMMON_TYPE_MASK          0x00070000
	DWORD dwCommonType = XboxResource_Common & X_D3DCOMMON_TYPE_MASK;
	return dwCommonType;
}

inline DWORD GetXboxCommonResourceType(X_D3DResource* pXboxResource)
{
	return GetXboxCommonResourceType(pXboxResource->Common);
}

void* GetDataFromXboxResource(X_D3DResource* pXboxResource)
{

	xbox::addr_xt pData = pXboxResource->Data;

	DWORD dwCommonType = GetXboxCommonResourceType(pXboxResource);
	pData |= PHYSICAL_MAP_BASE;
	return (uint8_t*)pData;
}

static void FlushRenderStateForMainThread(LocalRenderCommandQueue& queue)
{

	if (g_pipelineState.vertexShaderType == VertexShaderMode::FixedFunction)
		UpdateFixedFunctionVertexShaderState((size_t)g_pipelineState.vertexShader, g_pipelineState.isFixedFunctionPS);
	else
		UpdateHostViewPortOffsetAndScaleConstants();

	if (isTextureStageStateDirty() == true)
	{
		auto base = g_memory.base;
		for (uint32_t i = 0; i < 4; i++)
		{
			auto& cmd = queue.enqueue();
			cmd.type = RenderCommandType::SetTextureStageState;
			cmd.SetTextureStageState.index = i;
			cmd.SetTextureStageState.D3DTSS_ADDRESSU = g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_ADDRESSU];
			cmd.SetTextureStageState.D3DTSS_ADDRESSV = g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_ADDRESSV];

			cmd.SetTextureStageState.D3DTSS_ADDRESSW = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_ADDRESSW];
			cmd.SetTextureStageState.D3DTSS_MAGFILTER = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_MAGFILTER];
			cmd.SetTextureStageState.D3DTSS_MINFILTER = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_MINFILTER];
			cmd.SetTextureStageState.D3DTSS_MIPFILTER = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_MIPFILTER];
			cmd.SetTextureStageState.D3DTSS_BORDERCOLOR = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_BORDERCOLOR];

			cmd.SetTextureStageState.D3DTSS_COLOROP = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_COLOROP];
			cmd.SetTextureStageState.D3DTSS_COLORARG0 = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_COLORARG0];
			cmd.SetTextureStageState.D3DTSS_COLORARG1 = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_COLORARG1];
			cmd.SetTextureStageState.D3DTSS_COLORARG2 = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_COLORARG2];
			cmd.SetTextureStageState.D3DTSS_ALPHAOP = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_ALPHAOP];
			cmd.SetTextureStageState.D3DTSS_ALPHAARG0 = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_ALPHAARG0];
			cmd.SetTextureStageState.D3DTSS_ALPHAARG1 = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_ALPHAARG1];
			cmd.SetTextureStageState.D3DTSS_ALPHAARG2 = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_ALPHAARG2];
			cmd.SetTextureStageState.D3DTSS_RESULTARG = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_RESULTARG];
		}

		memcpy(g_dirty_texture_stage_states, (uint32_t*)g_texture_stage_states, sizeof(uint32_t) * 128);

		//g_dirtyStates.textureStageState = false;
	}

	//todo check if this is correct
	if (g_vertexConstantsDirtyFlags != 0)
	{
		int batchStartIndex = -1; // -1 means we aren't in a batch

		for (int i = 0; i < HOST_D3DVS_CONSTREG_FULL; i++)
		{
			if (batchStartIndex == -1 && vsh_constants_dirty[i])
			{
				batchStartIndex = i; // Start a batch
			}
			else if (batchStartIndex != -1 && !vsh_constants_dirty[i])
			{
				// Finish the batch
				int count = i - batchStartIndex;


				auto& cmd = queue.enqueue();
				cmd.type = RenderCommandType::SetVertexShaderConstants;
				cmd.setVertexShaderConstants.memory =
					g_intermediaryUploadAllocator.allocate(&vsh_constants[batchStartIndex][0], count * sizeof(float) * 4);
				cmd.setVertexShaderConstants.index = batchStartIndex;
				cmd.setVertexShaderConstants.size = count;

				batchStartIndex = -1;
			}

			// Constant is no longer dirty
			vsh_constants_dirty[i] = false;
		}

		// Final batch if loop ended while still in one
		if (batchStartIndex != -1)
		{
			int count = HOST_D3DVS_CONSTREG_FULL - batchStartIndex;

			auto& cmd = queue.enqueue();
			cmd.type = RenderCommandType::SetVertexShaderConstants;
			cmd.setVertexShaderConstants.memory =
				g_intermediaryUploadAllocator.allocate(&vsh_constants[batchStartIndex][0],
					count * sizeof(float) * 4);
			cmd.setVertexShaderConstants.index = batchStartIndex;
			cmd.setVertexShaderConstants.size = count;
		}

		g_vertexConstantsDirtyFlags = false;
	}
}


static constexpr int32_t COMMON_DEPTH_BIAS_VALUE = int32_t((1 << 24) * 0.002f);
static constexpr float COMMON_SLOPE_SCALED_DEPTH_BIAS_VALUE = 1.0f;
static void FlushRenderStateForRenderThread()
{

	auto renderTarget = g_pipelineState.colorWriteEnable ? g_renderTarget : nullptr;
	auto depthStencil = g_pipelineState.zEnable || g_pipelineState.stencilEnable ? g_depthStencil : nullptr;

	bool foundAny = PopulateBarriersForCopyRect(renderTarget, depthStencil);

	for (const auto surface : g_pendingMsaaResolves)
	{
		bool isDepthStencil = (surface->format == DepthStencilFormat);
		foundAny |= PopulateBarriersForCopyRect(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr);
	}

	if (foundAny)
	{
		FlushBarriers();
		ExecutePendingCopyRectsCommands(renderTarget, depthStencil);

		for (const auto surface : g_pendingMsaaResolves)
		{
			bool isDepthStencil = (surface->format == DepthStencilFormat);
			ExecutePendingCopyRectsCommands(isDepthStencil ? nullptr : surface, isDepthStencil ? surface : nullptr);
		}
	}

	if (!g_pendingMsaaResolves.empty())
		g_pendingMsaaResolves.clear();

	AddBarrier(renderTarget, RenderTextureLayout::COLOR_WRITE);
	AddBarrier(depthStencil, RenderTextureLayout::DEPTH_WRITE);

	FlushBarriers();

	SetFramebuffer(renderTarget, depthStencil, false);
	FlushViewport();

	auto& commandList = g_commandLists[g_frame];

	// D3D12 resets depth bias values to the pipeline values, even if they are dynamic.
	// We can reduce unnecessary calls by making common depth bias values part of the pipeline.
	if (g_capabilities.dynamicDepthBias && !g_vulkan)
	{
		bool useDepthBias = (g_depthBias != 0) || (g_slopeScaledDepthBias != 0.0f);

		int32_t depthBias = useDepthBias ? COMMON_DEPTH_BIAS_VALUE : 0;
		float slopeScaledDepthBias = useDepthBias ? COMMON_SLOPE_SCALED_DEPTH_BIAS_VALUE : 0.0f;

		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias, depthBias);
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.slopeScaledDepthBias, slopeScaledDepthBias);
	}


	if (g_dirtyStates.pipelineState)
	{


		commandList->setPipeline(CreateGraphicsPipelineInRenderThread(g_pipelineState));


		// D3D12 resets the depth bias values. Check if they need to be set again.
		if (g_capabilities.dynamicDepthBias && !g_vulkan)
			g_dirtyStates.depthBias = (g_depthBias != g_pipelineState.depthBias) || (g_slopeScaledDepthBias != g_pipelineState.slopeScaledDepthBias);
	}


	if (g_dirtyStates.depthBias && g_capabilities.dynamicDepthBias)
		commandList->setDepthBias(g_depthBias, 0.0f, g_slopeScaledDepthBias);



	if (g_dirtyStates.vertexShaderConstants)
	{

		auto vertexShaderConstants = g_uploadAllocators[g_frame].allocate<true>(g_vertexShaderConstants, sizeof(g_vertexShaderConstants), 0x100);

		SetRootDescriptor(vertexShaderConstants, 0);
	}

	if (g_dirtyStates.pixelShaderConstants)
	{
		auto pixelShaderConstants = g_uploadAllocators[g_frame].allocate<true>(g_pixelShaderConstants, sizeof(g_pixelShaderConstants), 0x100);
		SetRootDescriptor(pixelShaderConstants, 1);
	}

	if (g_dirtyStates.sharedConstants)
	{
		auto sharedConstants = g_uploadAllocators[g_frame].allocate<false>(&g_sharedConstants, sizeof(g_sharedConstants), 0x100);
		SetRootDescriptor(sharedConstants, 2);
	}



	if (g_dirtyStates.vertexStreamFirst <= g_dirtyStates.vertexStreamLast)
	{
		commandList->setVertexBuffers(
			g_dirtyStates.vertexStreamFirst,
			g_vertexBufferViews + g_dirtyStates.vertexStreamFirst,
			g_dirtyStates.vertexStreamLast - g_dirtyStates.vertexStreamFirst + 1,
			g_inputSlots + g_dirtyStates.vertexStreamFirst);
	}

	if (g_dirtyStates.indices && (!g_vulkan || g_indexBufferView.buffer.ref != nullptr))
		commandList->setIndexBuffer(&g_indexBufferView);

	g_dirtyStates = DirtyStates(false);
}

// lookup table for converting vertex count to primitive count
const unsigned g_XboxPrimitiveTypeInfo[11][2] =
{
	// First number is the starting number of vertices the draw requires
	// Second number the number of vertices per primitive
	// Example : Triangle list, has no starting vertices, and uses 3 vertices for each triangle
	// Example : Triangle strip, starts with 2 vertices, and adds 1 for each triangle
	{0, 0}, // NULL
	{0, 1}, // X_D3DPT_POINTLIST
	{0, 2}, // X_D3DPT_LINELIST
	{1, 1}, // X_D3DPT_LINELOOP
	{1, 1}, // X_D3DPT_LINESTRIP
	{0, 3}, // X_D3DPT_TRIANGLELIST
	{2, 1}, // X_D3DPT_TRIANGLESTRIP
	{2, 1}, // X_D3DPT_TRIANGLEFAN
	{0, 4}, // X_D3DPT_QUADLIST
	{2, 2}, // X_D3DPT_QUADSTRIP
	{0, 1}, // X_D3DPT_POLYGON
};

// convert from vertex count to primitive count (Xbox)
inline uint32_t ConvertXboxVertexCountToPrimitiveCount(X_D3DPRIMITIVETYPE XboxPrimitiveType, uint32_t VertexCount)
{
	assert(XboxPrimitiveType < X_D3DPT_MAX);
	auto res = (VertexCount - g_XboxPrimitiveTypeInfo[XboxPrimitiveType][0]) / g_XboxPrimitiveTypeInfo[XboxPrimitiveType][1];
	return res;
}

//primive count is different on PC compared to Xbox so we need to convert it
uint32_t GetNewPrimitiveCount(X_D3DPRIMITIVETYPE XboxPrimitiveType, uint32_t OriginalVertexCount)
{
	if (XboxPrimitiveType == X_D3DPT_QUADSTRIP)
	{
		// Quad strip is just like a triangle strip, but requires two vertices per primitive.
		// A quadstrip starts with 4 vertices and adds 2 vertices per additional quad.
		// This is much like a trianglestrip, which starts with 3 vertices and adds
		// 1 vertex per additional triangle, so we use that instead. The planar nature
		// of the quads 'survives' through this change. There's a catch though :
		// In a trianglestrip, every 2nd triangle has an opposing winding order,
		// which would cause backface culling - but this seems to be intelligently
		// handled by d3d
		return ConvertXboxVertexCountToPrimitiveCount(X_D3DPT_TRIANGLESTRIP, OriginalVertexCount);
	}
	else {
		return ConvertXboxVertexCountToPrimitiveCount(XboxPrimitiveType, OriginalVertexCount);
	}
}

static RenderPrimitiveTopology ConvertPrimitiveType(uint32_t primitiveType)
{
	switch (primitiveType)
	{
	case X_D3DPT_POINTLIST:
		return RenderPrimitiveTopology::POINT_LIST;
	case X_D3DPT_LINELIST:
		return RenderPrimitiveTopology::LINE_LIST;
	case X_D3DPT_LINESTRIP:
		return RenderPrimitiveTopology::LINE_STRIP;
	case X_D3DPT_TRIANGLELIST:
	case X_D3DPT_QUADLIST:
		return RenderPrimitiveTopology::TRIANGLE_LIST;
	case X_D3DPT_TRIANGLESTRIP:
		return RenderPrimitiveTopology::TRIANGLE_STRIP;
	case X_D3DPT_TRIANGLEFAN:
		return g_capabilities.triangleFan ? RenderPrimitiveTopology::TRIANGLE_FAN : RenderPrimitiveTopology::TRIANGLE_LIST;
		//todo check if this is correct:
	case X_D3DPT_QUADSTRIP:
		return RenderPrimitiveTopology::TRIANGLE_STRIP;
	case X_D3DPT_POLYGON:
		return g_capabilities.triangleFan ?
			RenderPrimitiveTopology::TRIANGLE_FAN :
			RenderPrimitiveTopology::TRIANGLE_LIST;
	default:
		assert(false && "Unknown primitive type");
		return RenderPrimitiveTopology::UNKNOWN;
	}
}

static void SetPrimitiveType(uint32_t primitiveType)
{
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.primitiveTopology, ConvertPrimitiveType(primitiveType));
}


static void UnlockIndexBuffer(GuestBuffer* buffer);
static void* LockIndexBuffer(GuestBuffer* buffer, uint32_t flags);
static void ProcSetIndices(const RenderCommand& cmd)
{
	const auto& args = cmd.setIndices;

	SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, args.indexBuffer != nullptr ? args.indexBuffer->buffer->at(0) : RenderBufferReference{});
	SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format, args.indexBuffer != nullptr ? args.indexBuffer->format : RenderFormat::R16_UINT);
	SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, args.indexBuffer != nullptr ? args.indexBuffer->dataSize : 0u);
}

static void SetIndices(GuestBuffer* buffer)
{
	RenderCommand cmd;
	cmd.type = RenderCommandType::SetIndices;
	cmd.setIndices.indexBuffer = buffer;
	g_renderQueue.enqueue(cmd);
}

static void* LockBuffer(GuestBuffer* buffer, uint32_t flags)
{
	buffer->lockedReadOnly = (flags & 0x10) != 0;

	if (buffer->mappedMemory == nullptr)
		buffer->mappedMemory = g_userHeap.AllocPhysical(buffer->dataSize, 0x10);


	buffer->isDirty = true;

	if (buffer->isVertexBuffer)
	{
		buffer->guestResource.Data |= 0x80000000;
	}

	return buffer->mappedMemory;
}


//D3DVertexBuffer_Lock2 - DX8 on Xbox doesn't have an unlock function so we will unlock right before rendering
void* LockVertexBuffer(GuestBuffer* buffer, uint32_t flags2)
{
	return LockBuffer(buffer, flags2);
}


template<typename T>
static void UnlockBuffer(GuestBuffer* buffer, bool useCopyQueue)
{

	auto copyBuffer = [&](T* dest)
		{

			auto src = reinterpret_cast<const T*>(buffer->mappedMemory);

			for (size_t i = 0; i < buffer->dataSize; i += sizeof(T))
			{
				*dest = *src;
				++dest;
				++src;
			}

		};

	if (useCopyQueue && g_capabilities.gpuUploadHeap)
	{
		copyBuffer(reinterpret_cast<T*>(buffer->buffer->map()));
		buffer->buffer->unmap();
	}
	else
	{
		auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(buffer->dataSize));
		copyBuffer(reinterpret_cast<T*>(uploadBuffer->map()));
		uploadBuffer->unmap();

		if (useCopyQueue)
		{
			ExecuteCopyCommandList([&]
				{
					g_copyCommandList->copyBufferRegion(buffer->buffer->at(0), uploadBuffer->at(0), buffer->dataSize);
				});

			if (buffer->isVertexBuffer)
			{
				//printf("[UPLOAD] guestVB=%p -> gpuResource=%p size=%u\n", buffer, (void*)buffer->buffer->at(0).ref, buffer->dataSize);
			}
		}
		else
		{
			auto& commandList = g_commandLists[g_frame];

			commandList->barriers(RenderBarrierStage::COPY, RenderBufferBarrier(buffer->buffer.get(), RenderBufferAccess::WRITE));
			commandList->copyBufferRegion(buffer->buffer->at(0), uploadBuffer->at(0), buffer->dataSize);
			commandList->barriers(RenderBarrierStage::GRAPHICS, RenderBufferBarrier(buffer->buffer.get(), RenderBufferAccess::READ));

			g_tempBuffers[g_frame].emplace_back(std::move(uploadBuffer));
		}
	}

	g_bufferUploadCount++;
}


template<typename T>
static void UnlockBufferToGPU(GuestBuffer* buffer, bool useCopyQueue)
{
	auto uniqueGPUBuffer = CreateVertexBuffer(buffer->dataSize);// g_userHeap.AllocPhysical<GuestBuffer>(ResourceType::VertexBuffer);
	//uniqueGPUBuffer->buffer = g_device->createBuffer(RenderBufferDesc::VertexBuffer(buffer->dataSize, GetBufferHeapType(), RenderBufferFlag::INDEX));
	uniqueGPUBuffer->mappedMemory = g_userHeap.AllocPhysical(buffer->dataSize, 0x10);
	memcpy(uniqueGPUBuffer->mappedMemory, buffer->mappedMemory, buffer->dataSize);

	auto copyBuffer = [&](T* dest)
		{

			auto src = reinterpret_cast<const T*>(buffer->mappedMemory);

			for (size_t i = 0; i < buffer->dataSize; i += sizeof(T))
			{
				*dest = *src;
				++dest;
				++src;
			}

		};

	if (useCopyQueue && g_capabilities.gpuUploadHeap)
	{
		copyBuffer(reinterpret_cast<T*>(uniqueGPUBuffer->buffer->map()));
		uniqueGPUBuffer->buffer->unmap();
	}
	else
	{
		auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(buffer->dataSize));

		copyBuffer(reinterpret_cast<T*>(uploadBuffer->map()));

		uploadBuffer->unmap();

		if (useCopyQueue)
		{
			ExecuteCopyCommandList([&]
				{
					g_copyCommandList->copyBufferRegion(uniqueGPUBuffer->buffer->at(0), uploadBuffer->at(0), buffer->dataSize);
				});
		}
		else
		{
			auto& commandList = g_commandLists[g_frame];

			commandList->barriers(RenderBarrierStage::COPY, RenderBufferBarrier(uniqueGPUBuffer->buffer.get(), RenderBufferAccess::WRITE));
			commandList->copyBufferRegion(uniqueGPUBuffer->buffer->at(0), uploadBuffer->at(0), buffer->dataSize);
			commandList->barriers(RenderBarrierStage::GRAPHICS, RenderBufferBarrier(uniqueGPUBuffer->buffer.get(), RenderBufferAccess::READ));

			g_tempBuffers[g_frame].emplace_back(std::move(uploadBuffer));
		}
	}

	g_bufferUploadCount++;
}

template<typename T>
static void UnlockBuffer(GuestBuffer* buffer)
{
	if (!buffer->lockedReadOnly)
	{
		UnlockBuffer<T>(buffer, true);
	}
}

static void ProcUnlockBuffer16(const RenderCommand& cmd)
{
	UnlockBuffer<uint16_t>(cmd.unlockBuffer.indexBuffer, false);
}

static void ProcUnlockBuffer32(const RenderCommand& cmd)
{
	UnlockBuffer<uint32_t>(cmd.unlockBuffer.indexBuffer, false);
}

static void UnlockVertexBuffer(GuestBuffer* buffer)
{
	//UnlockBufferToGPU<uint32_t>(buffer, true);
	UnlockBuffer<uint32_t>(buffer);
}

static void* LockIndexBuffer(GuestBuffer* buffer, uint32_t flags)
{
	return LockBuffer(buffer, flags);
}

static void UnlockIndexBuffer(GuestBuffer* buffer)
{
	UnlockBuffer<uint16_t>(buffer);
}

//D3D12 requires vertex buffer strides to be aligned to 4 bytes unlike Xbox, so we need to fix that
uint32_t AlignStride(uint32_t stride, uint32_t alignment = 4) {
	return (stride + alignment - 1) & ~(alignment - 1);
}

GuestBuffer* GetGuestVertexBuffer(XD3DResource* buffer, int32_t VertexCount, int i)
{
	unsigned long dataAddr = (unsigned long)GetDataFromXboxResource((X_D3DResource*)(g_Xbox_SetStreamSource[i].VertexBuffer));
	uint8_t* pXboxVertexData = reinterpret_cast<uint8_t*>(g_memory.Translate(dataAddr));
	const uint32_t originalStride = g_Xbox_SetStreamSource[i].Stride;
	//size_t size = buffer->dataSize;
	auto AlignedStrige = AlignStride(originalStride);
	const bool requirePadding = (AlignedStrige != originalStride);
	const size_t size = VertexCount * AlignedStrige;

	XXH64_hash_t hash = XXH3_64bits(pXboxVertexData, size);
	auto& bufferCache = g_vertexBufferCache[hash];
	if (bufferCache == nullptr)
	{
		bufferCache = CreateVertexBuffer(size);
		bufferCache->hash = hash;
		bufferCache->mappedMemory = g_userHeap.AllocPhysical(bufferCache->dataSize, 0x10);
		if (requirePadding == false)
		{
			memcpy(bufferCache->mappedMemory, pXboxVertexData, bufferCache->dataSize);
		}
		else
		{
			// Need padding - copy vertex by vertex
			const uint8_t* srcPtr = static_cast<const uint8_t*>(pXboxVertexData);
			uint8_t* destPtr = static_cast<uint8_t*>(bufferCache->mappedMemory);

			for (uint32_t j = 0; j < VertexCount; j++)
			{
				memcpy(destPtr, srcPtr, originalStride);
				memset(destPtr + originalStride, 0, AlignedStrige - originalStride);
				srcPtr += originalStride;
				destPtr += AlignedStrige;
			}
		}

		UnlockVertexBuffer(bufferCache);
	}

	return bufferCache;
}

//D3D_DrawVertices originally
void DrawPrimitive(X_D3DPRIMITIVETYPE PrimitiveType, uint32_t StartVertex, uint32_t VertexCount)
{

	for (int i = 0; i < 16; i++)
	{
		if (g_Xbox_SetStreamSource[i].VertexBuffer)
		{
			auto buffer = (XD3DResource*)g_Xbox_SetStreamSource[i].VertexBuffer;
			//auto buffer = g_Xbox_SetStreamSource[i].VertexBuffer;
			//uint8_t* pXboxVertexData = reinterpret_cast<uint8_t*>(buffer->mappedMemory);

			auto bufferCache = GetGuestVertexBuffer(buffer, VertexCount, i);

			RenderCommand cmd;
			cmd.type = RenderCommandType::SetStreamSource;
			cmd.setStreamSource.StreamNumber = i;
			cmd.setStreamSource.vertexBuffer = bufferCache;
			cmd.setStreamSource.stride = g_Xbox_SetStreamSource[i].Stride;
			g_renderQueue.enqueue(cmd);
		}
	}

	LocalRenderCommandQueue queue;
	FlushRenderStateForMainThread(queue);


	auto& cmd = queue.enqueue();
	cmd.type = RenderCommandType::DrawPrimitive;
	cmd.drawPrimitive.primitiveType = PrimitiveType;
	cmd.drawPrimitive.startVertex = StartVertex;
	cmd.drawPrimitive.primitiveCount = VertexCount;

	queue.submit();
}

static void ProcDrawPrimitive(const RenderCommand& cmd)
{
	const auto& args = cmd.drawPrimitive;

	//convert primitive
	SetPrimitiveType(args.primitiveType);
	FlushRenderStateForRenderThread();
	auto& commandList = g_commandLists[g_frame];
	commandList->drawInstanced(args.primitiveCount, 1, args.startVertex, 0);
}

static void ComputeIndexRange(const int16_t* indices, uint32_t startIndex, uint32_t indexCount, uint32_t& outMin, uint32_t& outMax)
{

	const int16_t* startPtr = indices + startIndex;
	uint32_t minIdx = UINT32_MAX;
	uint32_t maxIdx = 0;
	for (uint32_t i = 0; i < indexCount; ++i)
	{
		uint32_t idx = (uint16_t)startPtr[i];
		if (idx < minIdx)
			minIdx = idx;

		if (idx > maxIdx)
			maxIdx = idx;
	}

	if (minIdx == UINT32_MAX)
	{
		minIdx = 0;
		maxIdx = 0;
	}

	outMin = minIdx;
	outMax = maxIdx;
}

GuestBuffer* GetGuestVertexBufferIndexed(XD3DResource* buffer, int32_t VertexCount, int i, uint32_t minIdx, uint32_t maxIdx)
{
	uint32_t usedVertices = maxIdx - minIdx + 1;
	const uint32_t originalStride = g_Xbox_SetStreamSource[i].Stride;


	unsigned long dataAddr = (unsigned long)GetDataFromXboxResource((X_D3DResource*)(g_Xbox_SetStreamSource[i].VertexBuffer));
	uint8_t* pXboxVertexData = reinterpret_cast<uint8_t*>(g_memory.Translate(dataAddr));

	auto alignedStride = AlignStride(originalStride);
	const bool requirePadding = (alignedStride != originalStride);
	size_t size = usedVertices * alignedStride;

	const size_t originalSize = usedVertices * originalStride;
	XXH64_hash_t hash = XXH3_64bits(pXboxVertexData + minIdx * originalStride, originalSize);
	auto& bufferCache = g_vertexBufferCache[hash];

	if (bufferCache == nullptr)
	{
		bufferCache = CreateVertexBuffer(size);
		bufferCache->mappedMemory = g_userHeap.AllocPhysical(bufferCache->dataSize, 0x10);
		bufferCache->hash = hash;
		if (requirePadding == false)
		{
			memcpy(bufferCache->mappedMemory, pXboxVertexData + minIdx * originalStride, originalSize);
		}
		else
		{
			const uint8_t* srcPtr = pXboxVertexData + (minIdx * originalStride);
			uint8_t* destPtr = static_cast<uint8_t*>(bufferCache->mappedMemory);

			for (uint32_t j = 0; j < usedVertices; j++)
			{
				memcpy(destPtr, srcPtr, originalStride);
				memset(destPtr + originalStride, 0, alignedStride - originalStride);
				srcPtr += originalStride;
				destPtr += alignedStride;
			}
		}

		UnlockVertexBuffer(bufferCache);
	}

	return bufferCache;
}

void RunDebuggingShader(uint8_t* vertexData);
//D3DDevice_DrawIndexedVertices (seems to be used to display message)
//On Xbox, VertexCount = IndexCount, both are the same
void DrawIndexedPrimitive(X_D3DPRIMITIVETYPE PrimitiveType, uint32_t VertexCount, int16_t* pIndexData)
{
	//shadow = 2192 vertex = stride 24
	//object = 91 vertex =  stride 24

	uint32_t minIdx = 0, maxIdx = 0;
	ComputeIndexRange(pIndexData, 0, VertexCount, minIdx, maxIdx);

	XXH64_hash_t hash = XXH3_64bits(pIndexData, VertexCount * sizeof(int16_t));
	auto& bufferIndexCache = g_indexBufferCache[hash];

	GuestBuffer* bufferIndex;
	if (bufferIndexCache == nullptr)
	{
		bufferIndex = CreateIndexBuffer(VertexCount, X_D3DFMT_INDEX16);

		bufferIndexCache = bufferIndex;
		bufferIndex->hash = hash;

		auto memory = LockIndexBuffer(bufferIndex, 0);

		memcpy(memory, pIndexData, sizeof(int16_t) * VertexCount);
		UnlockIndexBuffer(bufferIndex);
	}
	else
	{
		bufferIndex = bufferIndexCache;
	}


	for (int i = 0; i < 16; i++)
	{
		if (g_Xbox_SetStreamSource[i].VertexBuffer)
		{
			auto buffer = (XD3DResource*)g_Xbox_SetStreamSource[i].VertexBuffer;
			//auto buffer = g_Xbox_SetStreamSource[i].VertexBuffer;
			//uint8_t* pXboxVertexData = reinterpret_cast<uint8_t*>(buffer->mappedMemory);
			auto Stride = AlignStride(g_Xbox_SetStreamSource[i].Stride);
			const uint32_t originalStride = g_Xbox_SetStreamSource[i].Stride;
			auto bufferCache = GetGuestVertexBufferIndexed(buffer, VertexCount, i, minIdx, maxIdx);

			if (originalStride == 24)
			{

				unsigned long dataAddr = (unsigned long)GetDataFromXboxResource((X_D3DResource*)(g_Xbox_SetStreamSource[i].VertexBuffer));
				uint8_t* pXboxVertexData = reinterpret_cast<uint8_t*>(g_memory.Translate(dataAddr));
				//IndexedVertex* vertex = (IndexedVertex*)bufferCache->mappedMemory;

				IndexedVertex* vertex = (IndexedVertex*)pXboxVertexData;
				if (VertexCount == 171)
				{
					//printf("Vertex Log, Count %d\n", VertexCount);
					lastHandleAddr = lastHandleAddr;

					uint8_t* vertexData = (uint8_t*)vertex + (0 * originalStride);
					RunDebuggingShader(vertexData);
					for (int j = 0; j < 171; j++)
					{

						uint8_t* vertexData = (uint8_t*)vertex + (j * originalStride);
						float* pos = (float*)vertexData;
						float* color = (float*)vertexData + 12;
						float* uv = (float*)(vertexData + 24);

						/**printf("Vertex[%d] - pos %f, %f, %f\n", j, pos[0], pos[1], pos[2]);
						printf("Vertex[%d] - normal %f, %f, %f\n", j, color[0], color[1], color[2]);
						printf("Vertex[%d] - uv %f, %f\n", j, uv[0], uv[1]);*/

						if (color[1] == 0.0f || color[2] == 0.0f)
						{
							int debug = 1;
							debug = debug;
						}

						if (isinf(uv[0]) || isnan(uv[0])
							|| isinf(uv[1]) || isnan(uv[1]))
						{
							minIdx = minIdx;
							printf("WARNING NAN UV\n");
							printf("buffer Xbox %p - with %d vertex count, pass %d vertex uv %f, %f \n", buffer, VertexCount, j, uv[0], uv[1]);
						}

					}
					//printf("done\n");
					//printf("\n");
				}
			}

			RenderCommand cmd;
			cmd.type = RenderCommandType::SetStreamSource;
			cmd.setStreamSource.StreamNumber = i;
			cmd.setStreamSource.vertexBuffer = bufferCache;
			cmd.setStreamSource.stride = Stride;
			g_renderQueue.enqueue(cmd);
		}
	}



	SetIndices(bufferIndex);


	LocalRenderCommandQueue queue;


	FlushRenderStateForMainThread(queue);

	auto& cmd = queue.enqueue();
	cmd.type = RenderCommandType::DrawIndexedVertices;
	cmd.drawIndexedPrimitive.primitiveType = PrimitiveType;
	cmd.drawIndexedPrimitive.baseVertexIndex = -(int32_t)minIdx;
	cmd.drawIndexedPrimitive.startIndex = 0;
	cmd.drawIndexedPrimitive.primCount = VertexCount;

	queue.submit();
}

static void ProcDrawIndexedPrimitive(const RenderCommand& cmd)
{
	const auto& args = cmd.drawIndexedPrimitive;
	SetPrimitiveType(args.primitiveType);
	FlushRenderStateForRenderThread();

	g_commandLists[g_frame]->drawIndexedInstanced(args.primCount, 1, args.startIndex, args.baseVertexIndex, 0);
}



extern int countUntilFlameDraw;
//D3DDevice_DrawVerticesUP - used for 2D Stuff, does not require a vertex buffer (originally)
void DrawPrimitiveUP(X_D3DPRIMITIVETYPE PrimitiveType, uint32_t VertexCount, void* pVertexStreamZeroData, uint32_t VertexStreamZeroStride)
{


	LocalRenderCommandQueue queue;
	FlushRenderStateForMainThread(queue);

	if (logVB)
	{
		//pVertexStreamZeroData = testVertices;
		//VertexCount = 3;


	}

	// For some reason, some vertex have their position w set to Inf
	// while Xbox may not care about this, modern shaders definitely do, so we have to fix this
	// this isn't a recomp instruction bug, I checked on CXBX Reloaded and logged the values, pos.w is meant to be inf.
	// so far I only saw this issue with the message box on the title screen.

	UpVertex* vertexData = (UpVertex*)(pVertexStreamZeroData);
	for (int32_t i = 0; i < VertexCount; i++)
	{
		if (isinf(vertexData[i].w) || isnan(vertexData[i].w))
		{
			vertexData[i].w = 1.0f;
		}
	}




	auto& cmd = queue.enqueue();
	cmd.type = RenderCommandType::DrawVerticesUP;
	cmd.drawPrimitiveUP.primitiveType = PrimitiveType;
	cmd.drawPrimitiveUP.primitiveCount = VertexCount;
	cmd.drawPrimitiveUP.vertexStreamZeroData = g_intermediaryUploadAllocator.allocate(vertexData, VertexCount * VertexStreamZeroStride);
	cmd.drawPrimitiveUP.vertexStreamZeroSize = VertexCount * VertexStreamZeroStride;
	cmd.drawPrimitiveUP.vertexStreamZeroStride = VertexStreamZeroStride;

	queue.submit();
	aboutToDrawFlame = false;
}

static void ProcDrawPrimitiveUP(const RenderCommand& cmd)
{


	const auto& args = cmd.drawPrimitiveUP;
	X_D3DPRIMITIVETYPE XboxPrimitive = (X_D3DPRIMITIVETYPE)args.primitiveType;

	SetPrimitiveType(args.primitiveType);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[0], uint8_t(args.vertexStreamZeroStride));

	auto allocation = g_uploadAllocators[g_frame].allocate<true>(reinterpret_cast<const uint32_t*>(args.vertexStreamZeroData), args.vertexStreamZeroSize, 0x4);

	auto& vertexBufferView = g_vertexBufferViews[0];
	vertexBufferView.size = args.primitiveCount * args.vertexStreamZeroStride;
	vertexBufferView.buffer = allocation.buffer->at(allocation.offset);
	g_inputSlots[0].stride = args.vertexStreamZeroStride;
	g_dirtyStates.vertexStreamFirst = 0;

	uint32_t indexCount = 0;

	/**if (args.primitiveType == X_D3DPT_QUADLIST)
		indexCount = g_quadIndexData.prepare(args.primitiveCount);
	else if (!g_capabilities.triangleFan && args.primitiveType == X_D3DPT_TRIANGLEFAN)
		indexCount = g_triangleFanIndexData.prepare(args.primitiveCount);*/

		/**if (args.csdFilterState != CsdFilterState::Unknown &&
			(g_pipelineState.pixelShader == g_csdShader || g_pipelineState.pixelShader == g_csdFilterShader.get()))
		{
			SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader,
				args.csdFilterState == CsdFilterState::On ? g_csdFilterShader.get() : g_csdShader);
		}*/

	FlushRenderStateForRenderThread();

	uint32_t primitiveCount = args.primitiveCount;

	if (indexCount != 0)
		g_commandLists[g_frame]->drawIndexedInstanced(indexCount, 1, 0, 0, 0);
	else
		g_commandLists[g_frame]->drawInstanced(primitiveCount, 1, 0, 0);
}




static void ProcSetTextureStageState(const RenderCommand& cmd)
{
	const auto& args = cmd.SetTextureStageState;

	const auto addressU = ConvertTextureAddressMode(cmd.SetTextureStageState.D3DTSS_ADDRESSU);
	const auto addressV = ConvertTextureAddressMode(cmd.SetTextureStageState.D3DTSS_ADDRESSV);
	const auto addressW = ConvertTextureAddressMode(cmd.SetTextureStageState.D3DTSS_ADDRESSW);
	auto magFilter = ConvertTextureFilter(cmd.SetTextureStageState.D3DTSS_MAGFILTER);
	auto minFilter = ConvertTextureFilter(cmd.SetTextureStageState.D3DTSS_MINFILTER);
	auto mipFilter = ConvertTextureFilter(cmd.SetTextureStageState.D3DTSS_MIPFILTER);
	auto borderColor = ConvertBorderColor(cmd.SetTextureStageState.D3DTSS_BORDERCOLOR);

	bool anisotropyEnabled = false; // Config::AnisotropicFiltering > 0 && mipFilter == RenderFilter::LINEAR; //TODO
	if (anisotropyEnabled)
	{
		magFilter = RenderFilter::LINEAR;
		minFilter = RenderFilter::LINEAR;
	}

	auto& samplerDesc = g_samplerDescs[args.index];

	bool dirtySampler = false;

	SetDirtyValue(dirtySampler, samplerDesc.addressU, addressU);
	SetDirtyValue(dirtySampler, samplerDesc.addressV, addressV);
	SetDirtyValue(dirtySampler, samplerDesc.addressW, addressW);
	SetDirtyValue(dirtySampler, samplerDesc.minFilter, minFilter);
	SetDirtyValue(dirtySampler, samplerDesc.magFilter, magFilter);
	SetDirtyValue(dirtySampler, samplerDesc.mipmapMode, RenderMipmapMode(mipFilter));
	SetDirtyValue(dirtySampler, samplerDesc.maxAnisotropy, 16u); // anisotropyEnabled ? Config::AnisotropicFiltering : 16u);
	SetDirtyValue(dirtySampler, samplerDesc.anisotropyEnabled, anisotropyEnabled);
	SetDirtyValue(dirtySampler, samplerDesc.borderColor, borderColor);

	if (dirtySampler)
	{
		auto& [descriptorIndex, sampler] = g_samplerStates[XXH3_64bits(&samplerDesc, sizeof(RenderSamplerDesc))];
		if (descriptorIndex == NULL)
		{
			descriptorIndex = g_samplerStates.size();
			sampler = g_device->createSampler(samplerDesc);

			g_samplerDescriptorSet->setSampler(descriptorIndex - 1, sampler.get());
			//printf("Set Sampler index %d\n", descriptorIndex - 1);
		}

		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.samplerIndices[args.index], descriptorIndex - 1);
	}


	auto& sharedSamplerDesc = g_sharedSamplerDescs[args.index];

	bool dirtyCombiner = false;
	SetDirtyValue(dirtyCombiner, g_sharedConstants.color_op[args.index], cmd.SetTextureStageState.D3DTSS_COLOROP);
	SetDirtyValue(dirtyCombiner, g_sharedConstants.color_arg0[args.index], cmd.SetTextureStageState.D3DTSS_COLORARG0);
	SetDirtyValue(dirtyCombiner, g_sharedConstants.color_arg1[args.index], cmd.SetTextureStageState.D3DTSS_COLORARG1);
	SetDirtyValue(dirtyCombiner, g_sharedConstants.color_arg2[args.index], cmd.SetTextureStageState.D3DTSS_COLORARG2);
	SetDirtyValue(dirtyCombiner, g_sharedConstants.alpha_op[args.index], cmd.SetTextureStageState.D3DTSS_ALPHAOP);
	SetDirtyValue(dirtyCombiner, g_sharedConstants.alpha_arg0[args.index], cmd.SetTextureStageState.D3DTSS_ALPHAARG0);
	SetDirtyValue(dirtyCombiner, g_sharedConstants.alpha_arg1[args.index], cmd.SetTextureStageState.D3DTSS_ALPHAARG1);
	SetDirtyValue(dirtyCombiner, g_sharedConstants.alpha_arg2[args.index], cmd.SetTextureStageState.D3DTSS_ALPHAARG2);
	SetDirtyValue(dirtyCombiner, g_sharedConstants.result_arg[args.index], cmd.SetTextureStageState.D3DTSS_RESULTARG);


	bool dirtyTextureFlags = false;
	SetDirtyValue(dirtyTextureFlags, g_sharedConstants.BUMPENVMAT00[args.index], cmd.SetTextureStageState.D3DTSS_BUMPENVMAT00);
	SetDirtyValue(dirtyTextureFlags, g_sharedConstants.BUMPENVMAT01[args.index], cmd.SetTextureStageState.D3DTSS_BUMPENVMAT01);
	SetDirtyValue(dirtyTextureFlags, g_sharedConstants.BUMPENVMAT11[args.index], cmd.SetTextureStageState.D3DTSS_BUMPENVMAT11);
	SetDirtyValue(dirtyTextureFlags, g_sharedConstants.BUMPENVMAT10[args.index], cmd.SetTextureStageState.D3DTSS_BUMPENVMAT10);
	SetDirtyValue(dirtyTextureFlags, g_sharedConstants.BUMPENVLSCALE[args.index], cmd.SetTextureStageState.D3DTSS_BUMPENVLSCALE);
	SetDirtyValue(dirtyTextureFlags, g_sharedConstants.BUMPENVLOFFSET[args.index], cmd.SetTextureStageState.D3DTSS_BUMPENVLOFFSET);
	SetDirtyValue(dirtyTextureFlags, g_sharedConstants.TEXCOORDINDEX[args.index], cmd.SetTextureStageState.D3DTSS_TEXCOORDINDEX);


	if (dirtyCombiner || dirtyTextureFlags)
	{
		g_dirtyStates.sharedConstants = true;
	}
}

static void ProcSetVertexShaderConstants(const RenderCommand& cmd)
{
	auto& args = cmd.setVertexShaderConstants;
	//assert((args.index * sizeof(uint32_t) + args.size) <= sizeof(g_vertexShaderConstants));
	assert((args.index + args.size) * 4 <= HOST_D3DVS_CONSTREG_FULL * 4);
	memcpy(&g_vertexShaderConstants[args.index * 4], args.memory, args.size * 4 * sizeof(float));
	g_dirtyStates.vertexShaderConstants = true;
}

static void ProcSetPixelShaderConstants(const RenderCommand& cmd)
{
	/**auto& args = cmd.setPixelShaderConstants;
	assert((args.index * sizeof(uint32_t) + args.size) <= sizeof(g_pixelShaderConstants));

	memcpy(&g_pixelShaderConstants[args.index], args.memory, args.size);
	g_dirtyStates.pixelShaderConstants = true;*/
}

int sub_2F0CF0_r()
{
	return 0;
}


int D3DKCMiniportInitDMAChannel(DWORD* this_, int a2, int a3, int a4, int a5, DWORD* a6)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 20;
	return 1;
}


int D3DKCMiniportBindToChannel(DWORD* this_, int a2)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 4;
	return 1;
}

int D3DKCMiniportCreateGrObject(DWORD* this_, int a2, unsigned int a3, int a4)
{
	auto ctx = GetX86Context();
	ctx->esp.u32 += 12;
	return 1;
}

BOOL D3Device_IsBusy()
{
	return FALSE;
}

int CDevice_KickOff(int* this_)
{
	return 0;
}

int D3D_SetTileNoWait(int a1, int* a2)
{
	return 0;
}

//D3DResource_Release
X86_FUNC_IMPL(__imp__sub_3D5F10);
X86_FUNC(sub_3D5F10)
{
	auto resource = X86_LOAD_U32(ctx.esp.u32 + 4);
	if (resource)
	{
		__imp__sub_3D5F10(ctx, base);
	}
	else
	{
		ctx.esp.u32 += 0x8;  //Adjust esp for return value (0xC2 + 0x4)
	}
}



void CopyVertexShaderConstant(int32_t Register, DWORD* pConstantData, const uint32_t count)
{

	float* float_vsh_constants = (float*)&(vsh_constants[Register][0]);
	memcpy(float_vsh_constants, pConstantData, count * sizeof(float) * 4);

	for (DWORD i = 0; i < count; i++)
		vsh_constants_dirty[Register + i] = true;

	g_vertexConstantsDirtyFlags = true;
}



X86_FUNC_IMPL(__imp__sub_2C89D0);
X86_FUNC(sub_2C89D0)
{
	LogVSConstants = false;
	__imp__sub_2C89D0(ctx, base);
}

void D3DDevice_SetVertexShaderConstant1Fast(int32_t Register, DWORD* pConstantData)
{
	float* constants = (float*)pConstantData;
	if (LogVSConstants)
	{
		std::ofstream  outFile2("VSLogs.txt", std::ios::app);
		outFile2 << "D3DDevice_SetVertexShaderConstant1Fast" << '\n';


		float x = constants[0];
		float y = constants[1];
		float z = constants[2];
		float w = constants[3];

		//printf("Constant[%d]: (%f, %f, %f, %f)\n", Register, x, y, z, w);

		outFile2 << "Constant[" << Register << " * 4 + 0] " << x << '\n';
		outFile2 << "Constant[" << Register << " * 4 + 1] " << y << '\n';
		outFile2 << "Constant[" << Register << " * 4 + 2] " << z << '\n';
		outFile2 << "Constant[" << Register << " * 4 + 3] " << w << '\n';
		outFile2 << '\n';
		outFile2.close();
	}
	CopyVertexShaderConstant(Register, pConstantData, 1);
}


void D3DDevice_SetVertexShaderConstantNotInlineFast(int32_t Register, DWORD* pConstantData, uint32_t ConstantCount)
{
	float* constants = (float*)pConstantData;

	if (LogVSConstants)
	{
		std::ofstream  outFile2("VSLogs.txt", std::ios::app);
		outFile2 << "D3DDevice_SetVertexShaderConstantNotInlineFast" << '\n';

		int actualCount = ConstantCount / 4;
		for (int i = 0; i < actualCount; i++) {

			intptr_t addr = pConstantData[i * 4 + 0];
			addr = addr;
			float x = constants[i * 4 + 0];
			float y = constants[i * 4 + 1];
			float z = constants[i * 4 + 2];
			float w = constants[i * 4 + 3];

			outFile2 << "Constant[" << i << " * 4 + 0] " << x << '\n';
			outFile2 << "Constant[" << i << " * 4 + 1] " << y << '\n';
			outFile2 << "Constant[" << i << " * 4 + 2] " << z << '\n';
			outFile2 << "Constant[" << i << " * 4 + 3] " << w << '\n';


			//printf("Constant[%d]: (%f, %f, %f, %f)\n", i, x, y, z, w);
			//printf("\n");
			outFile2 << '\n';
		}
		//printf("\n");

		outFile2 << '\n';
		outFile2.close();
	}



	CopyVertexShaderConstant(Register, pConstantData, ConstantCount / 4);
}

enum RwRasterType : uint8_t
{
	rwRASTERTYPENORMAL = 0x0,
	rwRASTERTYPEZBUFFER = 0x1,
	rwRASTERTYPECAMERA = 0x2,
	rwRASTERTYPETEXTURE = 0x4,
	rwRASTERTYPECAMERATEXTURE = 0x5,
	rwRASTERTYPEMASK = 0x7,
	rwRASTERDONTALLOCATE = 0x80,
};


struct RwRaster
{
	uint32_t parent; //RwRaster*
	uint32_t cpPixels; //uint8_t* 
	uint32_t palette; //uint8_t*
	int32_t width;
	int32_t height;
	int32_t depth;
	int32_t stride;
	int16_t nOffsetX;
	int16_t nOffsetY;
	RwRasterType cType;
	uint32_t cFlags;
	uint8_t privateFlags;
	uint8_t cFormat;
	uint32_t originalPixels; //uint8_t*
	int32_t originalWidth;
	int32_t originalHeight;
	int32_t originalStride;
};

struct RwTexture
{
	uint32_t raster;   //RwRaster*
	uint8_t padding[0x54];
};

unsigned int GetPixelContainerMipMapLevels(X_D3DPixelContainer* pPixelContainer)
{
	if (pPixelContainer->Size == 0)
		return (pPixelContainer->Format & X_D3DFORMAT_MIPMAP_MASK) >> X_D3DFORMAT_MIPMAP_SHIFT;

	return 1;
}

void GetPixelContainerMeasures
(
	X_D3DPixelContainer* pPixelContainer,
	DWORD dwMipMapLevel,
	UINT* pWidth,
	UINT* pHeight,
	UINT* pDepth,
	UINT* pRowPitch,
	// Slice pitch (does not include mipmaps!)
	UINT* pSlicePitch
)
{
	DWORD Size = pPixelContainer->Size;
	X_D3DFORMAT X_Format = GetXboxPixelContainerFormat(pPixelContainer->Format);

	if (Size != 0)
	{
		*pRowPitch = (((Size & X_D3DSIZE_PITCH_MASK) >> X_D3DSIZE_PITCH_SHIFT) + 1) * X_D3DTEXTURE_PITCH_ALIGNMENT;
	}
	else
	{
		DWORD dwBPP = XBFormatBitsPerPixel(X_Format);

		*pRowPitch = (*pWidth) * dwBPP / 8;
	}

	*pSlicePitch = (*pRowPitch) * (*pHeight);

	if (XBFormatIsCompressed(X_Format))
		*pRowPitch *= 4;
}


//we need to copy the palette data so we can convert properly when copy texture in the GPU
static uint32_t* currentPaletteData;
//rwD3D8RasterLockPalette
X86_FUNC_IMPL(__imp__sub_2DF9F0);
X86_FUNC(sub_2DF9F0)
{
	__imp__sub_2DF9F0(ctx, base);
	if (ctx.eax.u32 == 1)
	{
		auto paletteData = X86_LOAD_U32(ctx.ecx.u32);
		currentPaletteData = (uint32_t*)(base + paletteData);
	}

}

void CopyTextureToGPU(GuestTexture* tex)
{
	if ((intptr_t)tex == 0x00000001a0ebf8b0)
	{
		printf("");
	}
	auto base = g_memory.base;
	auto format = GetXboxPixelContainerFormat((X_D3DFORMAT)tex->guestResource.Format);
	bool isSwizzled = FormatIsSwizzled(format);
	bool bCompressed = XBFormatIsCompressed(format);
	bool bCubemap = tex->guestResource.Format & X_D3DFORMAT_CUBEMAP;
	bool bConvertARGB = XBFormatRequiresConversionToARGB(format); // convert to RGBA if needed

	struct Slice
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		uint32_t srcOffset; // offset in xbox memory (relative to VirtualAddr + header)
		uint32_t dstOffset;  // offset in upload buffer
		uint32_t srcRowPitch; // as stored on Xbox (bytes or block-row bytes)
		uint32_t dstRowPitch; // aligned row pitch used in upload buffer
		uint32_t rowCount;   // number of rows (or block rows for compressed)
	};

	std::vector<Slice> slices;
	uint32_t curSrcOffset = 0;
	uint32_t curDstOffset = 0;
	uint32_t baseW = tex->width;
	uint32_t baseH = tex->height;
	uint32_t baseD = tex->depth == 0 ? 1 : tex->depth;

	UINT dwMipMapLevels = GetPixelContainerMipMapLevels((X_D3DPixelContainer*)&tex->guestResource);
	//UINT dwRowPitch, dwSlicePitch;
	//GetPixelContainerMeasures((X_D3DPixelContainer*)&tex->legacyTex, dwMipMapLevels, &tex->width, &tex->height, &tex->depth, &dwRowPitch, &dwSlicePitch);

	D3DCUBEMAP_FACES last_face = (bCubemap) ? D3DCUBEMAP_FACE_NEGATIVE_Z : D3DCUBEMAP_FACE_POSITIVE_X;

	// Block size only applies to compressed DXT formats
	// DXT1 block size is 8 bytes
	// Other Xbox DXT formats are 16 bytes
	DWORD blockSize = 0;

	if (bCompressed)
		blockSize = format == X_D3DFMT_DXT1 ? 8 : 16;

	for (int32_t face = D3DCUBEMAP_FACE_POSITIVE_X; face <= last_face; face++)
	{
		for (uint32_t mipmap_level = 0; mipmap_level < dwMipMapLevels; mipmap_level++)
		{
			auto& slice = slices.emplace_back();

			//ensures that we never go below 1 when computing sizes for each level.
			slice.width = std::max(1u, baseW >> mipmap_level);
			slice.height = std::max(1u, baseH >> mipmap_level);
			slice.depth = std::max(1u, baseD >> mipmap_level);


			// compute srcRowPitch (Xbox layout)
			if (bCompressed)
			{
				// rowPitch in bytes for block columns (each block covers 4 pixels horizontally)
				uint32_t blockCols = (slice.width + 3) / 4;
				slice.srcRowPitch = blockCols * blockSize;
				slice.rowCount = (slice.height + 3) / 4;
			}
			else
			{
				uint32_t bpp = XBFormatBitsPerPixel(format);
				uint32_t srcBytesPerRow = (slice.width * bpp) / 8;
				// Xbox often aligns rows to X_D3DTEXTURE_PITCH_ALIGNMENT (64) in "Size" path,
				// here we put the tight row (will be copied into a dst aligned pitch later)
				slice.srcRowPitch = srcBytesPerRow;
				slice.rowCount = slice.height;
			}

			uint32_t mip2dSize = slice.srcRowPitch * slice.rowCount;
			uint32_t mipSlicePitch = mip2dSize * slice.depth;

			// src offset (Xbox memory layout packs mips/faces tightly) — use curSrcOffset
			slice.srcOffset = curSrcOffset;

			// destination: align each slice placement to PLACEMENT_ALIGN
			uint32_t dstBpp = bConvertARGB ? 32 : XBFormatBitsPerPixel(format); // 32 bits if converting to ARGB
			uint32_t rawDstRowPitch = (slice.width * dstBpp) / 8;

			slice.dstRowPitch = (rawDstRowPitch + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
			slice.dstOffset = curDstOffset;

			curSrcOffset += mipSlicePitch;
			uint32_t alignedSliceSize = (slice.dstRowPitch * slice.rowCount * slice.depth + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);
			curDstOffset += alignedSliceSize;
		}
	}


	// allocate upload buffer with curDstOffset
	auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(curDstOffset));
	uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

	// For each slice: copy/unswizzle/convert into mapped + dstOffset

	for (auto& slice : slices)
	{
		unsigned long VirtualAddr = (unsigned long)GetDataFromXboxResource((X_D3DResource*)(&tex->guestResource));
		uint8_t* srcData = reinterpret_cast<uint8_t*>(base + VirtualAddr + slice.srcOffset);
		uint8_t* dstData = mappedMemory + slice.dstOffset;

		if (bConvertARGB)
		{
			ConvertD3DTextureToARGBBuffer(format, (uint8_t*)srcData, slice.width, slice.height, slice.srcRowPitch, slice.srcRowPitch * slice.rowCount,
				dstData, slice.dstRowPitch, slice.dstRowPitch * slice.rowCount, slice.depth, (void*)currentPaletteData);
		}
		else if (isSwizzled)
		{
			uint32_t dstSlicePitch = slice.dstRowPitch * slice.rowCount * slice.depth;
			uint32_t srcBpp = bCompressed ? (blockSize) : (XBFormatBitsPerPixel(format) / 8);

			UnswizzleBox(
				(PVOID)srcData, slice.width, slice.height, slice.depth,
				srcBpp,
				dstData, slice.dstRowPitch, dstSlicePitch);
		}
		else if (bCompressed)
		{
			memcpy(dstData, srcData, slice.srcRowPitch * slice.rowCount * slice.depth);
		}
		else
		{
			if (slice.srcRowPitch == slice.dstRowPitch)
			{
				memcpy(dstData, srcData, slice.srcRowPitch * slice.rowCount * slice.depth);
			}
			else
			{
				for (size_t i = 0; i < slice.rowCount * slice.depth; i++)
				{
					memcpy(dstData, srcData, slice.srcRowPitch);
					srcData += slice.srcRowPitch;
					dstData += slice.dstRowPitch;
				}
			}
		}
	}

	//finish
	uploadBuffer->unmap();

	ExecuteCopyCommandList([&]
		{
			g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(tex->texture, RenderTextureLayout::COPY_DEST));

			auto copyTextureRegion = [&](Slice& slice, uint32_t subresourceIndex)
				{
					uint32_t uploadRowPitchInTexels = slice.dstRowPitch / (bCompressed ? (blockSize) : (RenderFormatSize(tex->format)));

					/**g_copyCommandList->copyTextureRegion(
						RenderTextureCopyLocation::Subresource(tex->texture, subresourceIndex),
						RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), tex->format, slice.width, slice.height, slice.depth, uploadRowPitchInTexels, slice.dstOffset));*/

					g_copyCommandList->copyTextureRegion(
						RenderTextureCopyLocation::Subresource(tex->texture, subresourceIndex % dwMipMapLevels, subresourceIndex % dwMipMapLevels),
						RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), tex->format, slice.width, slice.height, slice.depth, uploadRowPitchInTexels, slice.dstOffset));
				};

			for (size_t i = 0; i < slices.size(); i++)
				copyTextureRegion(slices[i], i);

			// Duplicate the first face across the remaining 6 faces. (todo check if this is accurate)
			if (bCubemap)
			{
				for (size_t i = 1; i < 6; i++)
				{
					for (size_t j = 0; j < slices.size(); j++)
						copyTextureRegion(slices[j], (slices.size() * i) + j);
				}
			}
		});
}

//rwD3D8NativeTextureRead
X86_FUNC_IMPL(__imp__sub_2ED0D0);
X86_FUNC(sub_2ED0D0)
{
	//call original function first
	__imp__sub_2ED0D0(ctx, base);

	if (ctx.eax.u32 == 1) //did the function return 1 aka was the texture created
	{
		auto textureAddr = X86_LOAD_U32(ctx.ecx.u32);
		RwTexture* rwTex = (RwTexture*)(base + textureAddr);
		auto rasterAddr = X86_LOAD_U32(rwTex->raster);
		RwRaster* raster = (RwRaster*)(base + X86_LOAD_U32(rasterAddr));

		//renderware doesn't store texture data in the actual raster, instead it stores it in the raster extension offset
		auto RasterExtOffsetAddr = (uint32_t)(X86_LOAD_U32(rasterAddr) + X86_LOAD_U32(RwD3D8RasterExtOffsetAddr));
		auto TexAddr = X86_LOAD_U32(RasterExtOffsetAddr);
		GuestTexture* tex = (GuestTexture*)(base + TexAddr);

		//when native texture are used, we have to compute them so the memory alignment can match what DX12 expect
		//we also need to convert palette to regular format as DX12 does not support that directly
		//finally we need to unswizzle texture so they can match what DX12 expect.
		CopyTextureToGPU(tex);
	}
}


//rwD3D8RasterUnlock
X86_FUNC_IMPL(__imp__sub_2DF870);
X86_FUNC(sub_2DF870)
{
	//texture data aren't stored in the raster, instead they store the image in another raster using the extension offset to reach it
	auto rasterAddr = X86_LOAD_U32(ctx.esp.u32 + 8); //get raster param addr 
	auto RasterExtOffsetAddr = (uint32_t)(X86_LOAD_U32(rasterAddr) + X86_LOAD_U32(RwD3D8RasterExtOffsetAddr));
	auto TexAddr = X86_LOAD_U32(RasterExtOffsetAddr);
	GuestTexture* tex = (GuestTexture*)(base + TexAddr);
	UnlockTextureRect(tex);
	__imp__sub_2DF870(ctx, base);
}


//CreateTexture
X86_FUNC_IMPL(__imp__sub_2DFE20);
X86_FUNC(sub_2DFE20)
{
	RwRaster* info = (RwRaster*)g_memory.Translate(ctx.ecx.u32);
	uint8_t level = X86_LOAD_U8(ctx.eax.u32 + 19);

	X_D3DFORMAT format = (X_D3DFORMAT)ctx.esi.u32;
	__imp__sub_2DFE20(ctx, base);

	//RwRaster* rwM = (RwRaster*)g_memory.Translate(second);
	XD3DResource* texture = (XD3DResource*)(g_memory.Translate(ctx.eax.u32));

	ctx.esp.u32 -= 4;




	int usage = LOBYTE(info->cType) == rwRASTERTYPECAMERATEXTURE ? 1 : 0;
	if (usage)
	{
		printf("");
	}
	auto newTex = CreateTexture(info->width, info->height, 1, level, usage, format, X_D3DRTYPE_TEXTURE);

	if ((intptr_t)newTex == 0x00000001a0ebf8b0)
	{
		printf("");
	}
	newTex->guestResource.Common = texture->Common;
	newTex->guestResource.Data = texture->Data;
	newTex->guestResource.Format = texture->Format;
	newTex->guestResource.Lock = texture->Lock;
	newTex->guestResource.Size = texture->Size;
	ctx.eax.u32 = g_memory.MapVirtual(newTex);



	ctx.esp.u32 += 4;
}

//CreateStandAloneSurface
X86_FUNC_IMPL(__imp__sub_3DA0A0);
X86_FUNC(sub_3DA0A0)
{

	uint32_t Width = X86_LOAD_U32(ctx.esp.u32 + 4);
	uint32_t Height = X86_LOAD_U32(ctx.esp.u32 + 8);
	X_D3DFORMAT Format = (X_D3DFORMAT)X86_LOAD_U32(ctx.esp.u32 + 16);
	__imp__sub_3DA0A0(ctx, base);


	ctx.esp.u32 -= 4;

	X_D3DSurface* surface = (X_D3DSurface*)(g_memory.Translate(ctx.eax.u32));

	auto newSurface = CreateSurface(Width, Height, Format);

	newSurface->guestResource.Common = surface->Common;
	newSurface->guestResource.Data = surface->Data;
	newSurface->guestResource.Format = surface->Format;
	newSurface->guestResource.Lock = surface->Lock;
	newSurface->guestResource.Size = surface->Size;
	ctx.eax.u32 = g_memory.MapVirtual(newSurface);

	ctx.esp.u32 += 4;
}

//CreateVertexBuffer 
X86_FUNC_IMPL(__imp__sub_2E6750);
X86_FUNC(sub_2E6750)
{
	int size = X86_LOAD_U32(ctx.esp.u32 + 4);
	size = (size + 31) & 0xFFFFFFE0;
	__imp__sub_2E6750(ctx, base);
	auto addr = ctx.eax.u32;
	XD3DResource* buffer = (XD3DResource*)g_memory.Translate(addr);
	printf("Create VB %x - size: %d\n", (intptr_t)buffer, size);

	if (size == 2656)
	{
		printf("check that\n");
	}
	g_VBList.push_back({ buffer, nullptr, size });
}

//LockVertexBuffer
X86_FUNC_IMPL(__imp__sub_3D5FE0);
X86_FUNC(sub_3D5FE0)
{

	__imp__sub_3D5FE0(ctx, base);

	auto addr = ctx.eax.u32;
	addr = addr;
}


//GetViewportOFfsetAndScale
X86_FUNC_IMPL(__imp__sub_3D24B0);
X86_FUNC(sub_3D24B0)
{

	__imp__sub_3D24B0(ctx, base);

	if (LogVSConstants)
	{
		auto device = X86_LOAD_U32(GuestDevicePtrAddr);
		auto valA = X86_LOAD_F32(device + 0x95C);
		auto valB = X86_LOAD_F32(device + 0x960);
		auto padding636 = X86_LOAD_F32(device + 0x970);
		auto padding68 = X86_LOAD_U32(device + 0x954);
		auto padding7 = X86_LOAD_U32(device + 0xEF8);

		valB = valB;
		/**printf("GetViewportOffsetAndScale\n");
		printf("viewport Width %d\n", X86_LOAD_U32(device + 0xEE8));
		printf("viewport Height %d\n", X86_LOAD_U32(device + 0xEEC));
		printf("D3D_g_pDevice->padding6[8] %d\n", padding68);
		printf("D3D_g_pDevice->padding6[16] %f\n", valA);
		printf("D3D_g_pDevice->padding6[20] %f\n", valB);
		printf("D3D_g_pDevice->padding6[36] %f\n", padding636);
		printf("D3D_g_pDevice->padding7 %d\n", padding7)

		float v2 = (float)X86_LOAD_U32(device + 0xEE0) * (float)valA * (float)padding7;
		float scaleX = (float)X86_LOAD_U32(device + 0xEE8) * (float)valA * 0.5f;

		printf("v2 %f\n", v2);
		printf("scaleX %f\n", scaleX);
		printf("*offset res %f\n", scaleX + v2);


		float offset[4] = { X86_LOAD_F32(0x55B410), X86_LOAD_F32(0x55B410 + 4), X86_LOAD_F32(0x55B410 + 8), X86_LOAD_F32(0x55B410 + 12) };
		float scale[4] = { X86_LOAD_F32(ctx.eax.u32), X86_LOAD_F32(ctx.eax.u32 + 4), X86_LOAD_F32(ctx.eax.u32 + 8), X86_LOAD_F32(ctx.eax.u32 + 12) };
		//printf("D3D_g_pDevice->padding7[4] %f\n", *(float*)&g_pDevice->padding7[4]);
		printf("offset: %f, %f, %f, %f\n", offset[0], offset[1], offset[2], offset[3]);
		printf("scale: %f, %f, %f, %f\n", scale[0], scale[1], scale[2], scale[3]);

		printf("\n");;*/
	}

}

struct RwV3d
{
	RwReal x;
	RwReal y;
	RwReal z;
};

enum RwMatrixFlag
{
	rwMATRIXINTERNALIDENTITY = 0x20000,
};
#define RwUInt32 uint32_t
struct RwMatrixTag
{
	RwV3d right;
	RwMatrixFlag flags;
	RwV3d up;
	RwUInt32 pad1;
	RwV3d at;
	RwUInt32 pad2;
	RwV3d pos;
	RwUInt32 pad3;
};


typedef RwMatrixTag RwMatrix;

void logRwMatrix(uint32_t addr, const char* name)
{

	RwMatrix* src = (RwMatrix*)g_memory.Translate(addr);
	printf("%s->right %f %f %f\n", name, src->right.x, src->right.y, src->right.z);
	printf("%s->up %f %f %f\n", name, src->up.x, src->up.y, src->up.z);
	printf("%s->at %f %f %f\n", name, src->at.x, src->at.y, src->at.z);
	printf("%s->pos %f %f %f\n", name, src->pos.x, src->pos.y, src->pos.z);
	printf("%s->flag %d\n", name, src->flags);
	printf("\n");

}

void logMatrix(uint32_t addr)
{
	auto base = g_memory.base;
	auto add = X86_LOAD_U32(addr);


	printf("RwMatrixInvert\n");
	logRwMatrix(addr, "src");
	printf("");

}


extern bool logMatrixStart;
/**X86_FUNC_IMPL(__imp__sub_4025A0);
X86_FUNC(sub_4025A0)
{
	auto a1Addr = X86_LOAD_U32(ctx.esp.u32 + 4);
	float* a1 = (float*)g_memory.Translate(a1Addr);
	auto a2Addr = X86_LOAD_U32(ctx.esp.u32 + 8);
	float* a2 = (float*)g_memory.Translate(a2Addr);
	auto a3Addr = X86_LOAD_U32(ctx.esp.u32 + 12);
	float* a3 = (float*)g_memory.Translate(a3Addr);


	printf("D3DXMatrixMultiply\n");

	for (int i = 0; i < 16; i++)
	{
		printf("a2[%d] %f,\n", i, a2[i]);

	}

	for (int i = 0; i < 16; i++)
	{
		printf("a3[%d] %f,\n", i, a3[i]);

	}

	//logRwMatrix(a1Addr, "a1");
	//logRwMatrix(a2Addr, "a2");


	__imp__sub_4025A0(ctx, base);


	printf("res\n");
	for (int i = 0; i < 16; i++)
	{
		printf("constantData[%d] %f\n", i, a1[i]);
	}

	printf("\n");


}

//rwmatrixTransform
X86_FUNC_IMPL(__imp__sub_2E3520);
X86_FUNC(sub_2E3520)
{

	auto a1Addr = X86_LOAD_U32(ctx.esp.u32 + 4);
	auto a2Addr = X86_LOAD_U32(ctx.esp.u32 + 8);
	auto mode = X86_LOAD_U32(ctx.esp.u32 + 12);

	if (logMatrixStart)
	{
		printf("RwMatrixTransform\n");
		printf("Mode %d\n", mode);

		logRwMatrix(a1Addr, "a1");
		logRwMatrix(a2Addr, "a2");
	}


	__imp__sub_2E3520(ctx, base);

	if (logMatrixStart)
	{
		printf("res\n\n");


		logRwMatrix(a1Addr, "a1");
		logRwMatrix(a2Addr, "a2");
		printf("\n");
	}

}*/

//todo
static DWORD* g_beginPush = nullptr;
uint32_t D3DDevice_BeginPush(DWORD Count)
{
	auto pRet = (DWORD*)(g_userHeap.Alloc(sizeof(DWORD) * Count));
	g_beginPush = (DWORD*)pRet;
	return g_memory.MapVirtual(pRet);
}

void D3DDevice_EndPush(DWORD* pPush)
{
	if (pPush)
	{
		g_userHeap.Free(g_beginPush);
		g_beginPush = nullptr;
	}
}



BOOL D3DDevice_IsFencePending(int a1)
{
	return FALSE;
}

void D3DResource_BlockUntilNotBusy(DWORD* a1)
{
	;
}

void D3D_BlockOnTime(int32_t a1, int32_t a2)
{
	;
}

static std::thread g_renderThread([]
	{
#ifdef _WIN32
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		GuestThread::SetThreadName(GetCurrentThreadId(), "Render Thread");
#endif

		RenderCommand commands[32];

		while (true)
		{
			size_t count = g_renderQueue.wait_dequeue_bulk(commands, std::size(commands));

			for (size_t i = 0; i < count; i++)
			{
				auto& cmd = commands[i];
				switch (cmd.type)
				{
				case RenderCommandType::SetRenderState:                    ProcSetRenderState(cmd); break;
				case RenderCommandType::DestructResource:                  ProcDestructResource(cmd); break;
				case RenderCommandType::UnlockTextureRect:                 ProcUnlockTextureRect(cmd); break;
				case RenderCommandType::UnlockBuffer16:                    ProcUnlockBuffer16(cmd); break;
				case RenderCommandType::UnlockBuffer32:                    ProcUnlockBuffer32(cmd); break;
				case RenderCommandType::DrawImGui:                         ProcDrawImGui(cmd); break;
				case RenderCommandType::ExecuteCommandList:                ProcExecuteCommandList(cmd); break;
				case RenderCommandType::BeginCommandList:                  ProcBeginCommandList(cmd); break;
					//case RenderCommandType::StretchRect:                       ProcStretchRect(cmd); break;
				case RenderCommandType::SetRenderTarget:                   ProcSetRenderTarget(cmd); break;
				case RenderCommandType::SetDepthStencilSurface:            ProcSetDepthStencilSurface(cmd); break;
				case RenderCommandType::ExecutePendingCopyRectsCommands: ProcExecutePendingCopyRectsCommands(cmd); break;
				case RenderCommandType::Clear:                             ProcClear(cmd); break;
				case RenderCommandType::SetViewport:                       ProcSetViewport(cmd); break;
				case RenderCommandType::SetTexture:                        ProcSetTexture(cmd); break;
					//case RenderCommandType::SetScissorRect:                    ProcSetScissorRect(cmd); break;
				case RenderCommandType::SetTextureStageState:                   ProcSetTextureStageState(cmd); break;
					//case RenderCommandType::SetBooleans:                       ProcSetBooleans(cmd); break;
				case RenderCommandType::SetVertexShaderConstants:          ProcSetVertexShaderConstants(cmd); break;
					//case RenderCommandType::SetPixelShaderConstants:           ProcSetPixelShaderConstants(cmd); break;
					//case RenderCommandType::AddPipeline:                       ProcAddPipeline(cmd); break;
				case RenderCommandType::DrawPrimitive:                     ProcDrawPrimitive(cmd); break;
				case RenderCommandType::DrawIndexedVertices:              ProcDrawIndexedPrimitive(cmd); break;
				case RenderCommandType::DrawVerticesUP:                   ProcDrawPrimitiveUP(cmd); break;
				case RenderCommandType::SetVertexDeclaration:              ProcSetVertexDeclaration(cmd); break;
				case RenderCommandType::SetVertexShader:                   ProcSetVertexShader(cmd); break;
				case RenderCommandType::SetStreamSource:                   ProcSetStreamSource(cmd); break;
				case RenderCommandType::SetIndices:                        ProcSetIndices(cmd); break;
				case RenderCommandType::SetPixelShader:                    ProcSetPixelShader(cmd); break;
				default:
					assert(false && "Unrecognized render command type.");
					break;
				}
			}
		}
	});


//=============== DONE =========
GUEST_FUNCTION_HOOK_STDCALL(sub_3DDAB0, Direct3D_CreateDevice_r);
//GUEST_FUNCTION_HOOK(sub_2E6750, CreateVertexBuffer);
//GUEST_FUNCTION_HOOK_STDCALL(sub_3D5FE0, LockVertexBuffer);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1E10, SetStreamSource);

//vertex shader
//GUEST_FUNCTION_HOOK_STDCALL(sub_3D1BA0, CreateVertexShader);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2280, SetVertexShader);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1F70, SelectVertexShader);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1F00, LoadVertexShaderProgram);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1EA0, LoadVertexShader);
GUEST_FUNCTION_HOOK_FASTCALL(sub_3D1CE0, D3DDevice_SetVertexShaderConstant1Fast);
GUEST_FUNCTION_HOOK_FASTCALL(sub_3D1D30, D3DDevice_SetVertexShaderConstantNotInlineFast);

//pixel shader
GUEST_FUNCTION_HOOK_STDCALL(sub_3DC520, CreatePixelShader);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DC590, SetPixelShader);


GUEST_FUNCTION_HOOK_STDCALL(sub_3D0680, D3DDevice_SetViewport);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D6BF0, D3DDevice_Clear);

GUEST_FUNCTION_HOOK_STDCALL(sub_3D5DB0, DestructResource);


//GUEST_FUNCTION_HOOK_STDCALL(sub_82BE7780, UnlockTextureRect);


GUEST_FUNCTION_HOOK(sub_321040, SetRenderState);
GUEST_FUNCTION_HOOK_FASTCALL(sub_3D2420, D3DDevice_SetRenderState_Simple);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2450, D3DDevice_SetRenderStateNotInline);

//Resource
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD710, CreateTexture_); //not used for 2D stuff at least.
//GUEST_FUNCTION_HOOK_STDCALL(sub_3DD7C0, GetSurfaceLevel);
//GUEST_FUNCTION_HOOK_STDCALL(sub_3DA0A0, CreateStandAloneSurface); //DepthStencilSurface
GUEST_FUNCTION_HOOK_STDCALL(sub_3DA010, CreateSurfaceOfTexture);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0CD0, SetTexture);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0040, D3DDevice_SetRenderTarget);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD810, LockTextureRect); //todo cube version?


//Rendering
GUEST_FUNCTION_HOOK_STDCALL(sub_3DC1F0, DrawIndexedPrimitive);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DC150, DrawPrimitive);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DBFE0, DrawPrimitiveUP);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD560, D3DDevice_Swap); //Present

//TODO: the begin and end push are essentially command system like DX12, this can be used to draw so we will have to handle this 
GUEST_FUNCTION_HOOK_STDCALL(sub_3D4740, D3DDevice_BeginPush);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D4760, D3DDevice_EndPush);




//texture stuff
//GUEST_FUNCTION_HOOK_STDCALL(sub_3D2E90, D3DDevice_SetTextureStageStateNotInline);

//GUEST_FUNCTION_HOOK_STDCALL(sub_3CFCD0, D3DDevice_SetTransform);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0E80, D3DDevice_SetPalette);



//		 === UNCONFIRMED STUB == TODO CHECK IF THEY ARE ALL CORRECT
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD8B0, D3D_GetAdapterModeCount);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD950, D3D_EnumAdapterModes);
//GUEST_FUNCTION_HOOK_STDCALL(sub_3D4890, D3D_SetFence);
//GUEST_FUNCTION_HOOK_STDCALL(sub_3D86F0, D3D_LazySetPointParams);
//GUEST_FUNCTION_HOOK_STDCALL(sub_3DB990, RW_SetTextureStageStates);


//GUEST_FUNCTION_HOOK_THISCALL(sub_3D2190, D3DDevice_SetRenderStateSTUB); // D3DK::CommonSetPassthroughProgram
//GUEST_FUNCTION_HOOK_FASTCALL(sub_3D2FF0, sub_3D2FF0_r); //Called in many functions that will be replaced so may not be needed.

//GUEST_FUNCTION_HOOK(sub_3D4C00, D3D_MakeRequestedSpace_);


//				==== STUB CONFIRMED ====
GUEST_FUNCTION_HOOK(sub_3D3240, D3D_CommonSetDebugRegisters);
GUEST_FUNCTION_HOOK(sub_3D0F10, D3Device_IsBusy);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0F40, D3D_SetTileNoWait);

GUEST_FUNCTION_HOOK_STDCALL(sub_3D4720, D3DDevice_IsFencePending);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D5F60, D3DResource_BlockUntilNotBusy);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D4950, D3D_BlockOnTime);
//GUEST_FUNCTION_HOOK_THISCALL(sub_3D7AEA, D3DKCMiniportInitDMAChannel);
//GUEST_FUNCTION_HOOK_THISCALL(sub_3D7B4D, D3DKCMiniportBindToChannel);
//GUEST_FUNCTION_HOOK_THISCALL(sub_3D7E31, D3DKCMiniportCreateGrObject);
//GUEST_FUNCTION_HOOK_THISCALL(sub_3D4770, CDevice_KickOff);