#include "video.h"

#include "imgui/imgui_common.h"
#include "imgui/imgui_snapshot.h"
#include "imgui/imgui_font_builder.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "vertexdeclaration.h"
#include "vertexshader.h"
#include "pixelshader.h"
#include "nv2a.h"
#include "pushbuffer.h"
#include "swizzle.h"
#include "FixedFunction.h"
#include "renderstate.h"
#include <app.h>
#include <cpu/guest_thread.h>
#include <decompressor.h>

#include <res/font/im_font_atlas.png.h>

#include <kernel/function.h>
#include <kernel/heap.h>
#include <ui/game_window.h>
#include <user/config.h>
#include "x86_context.h"
#include <patches/aspect_ratio_patches.h>
#include <sdl_listener.h>
#include <memory>
#include <shader/shader_cache.h>
#include <ui/imgui_utils.h>
#include <ui/installer_wizard.h>
#include <xxHashMap.h>
#include <patches/multi_core_patches.h>



const uint32_t GuestDevicePtrAddr = 0x3E0A18;
const uint32_t GuestDeviceMemAddr = 0x3E0A20;
const uint32_t PresentMemAddr = 0x55F718;
const uint32_t AutoDepthStencilFormat = 0x57DD68;
const uint32_t RwD3D8RasterExtOffsetAddr = 0x57DD20;
const uint32_t g_DeferredTextureStateAddr = 0x3E0580;
const uint32_t g_RenderStateAddr = 0x3E0780;
const uint32_t g_cameraAddr = 0x540C40;

bool LogVSConstants = false;

#define DepthStencilFormat RenderFormat::D32_FLOAT

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

#include "shader/R2D_vs0.hlsl.dxil.h"
#include "shader/R2D_vs1.hlsl.dxil.h"
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

#include "shader/R2D_vs0.hlsl.spirv.h"
#include "shader/R2D_vs1.hlsl.spirv.h"
#include <os/logger.h>
#include <os/process.h>
#include <ui/fader.h>
#include <ui/message_window.h>
#include <ui/options_menu.h>
#include <ui/button_guide.h>



namespace plume
{
#ifdef SHADOW_RECOMP_D3D12
	extern std::unique_ptr<RenderInterface> CreateD3D12Interface();
#endif
#ifdef PLUME_SDL_VULKAN_ENABLED
	extern std::unique_ptr<RenderInterface> CreateVulkanInterface(RenderWindow sdlWindow);
#else
	extern std::unique_ptr<RenderInterface> CreateVulkanInterface();
#endif

	static std::unique_ptr<RenderInterface> CreateVulkanInterfaceWrapper() {
#ifdef PLUME_SDL_VULKAN_ENABLED
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
	bool zEnable = false;
	bool zWriteEnable = true;
	bool stencilEnable = false;
	RenderBlend srcBlend = RenderBlend::ONE;
	RenderBlend destBlend = RenderBlend::ZERO;
	RenderCullMode cullMode = RenderCullMode::NONE;
	RenderComparisonFunction zFunc = RenderComparisonFunction::LESS;
	RenderComparisonFunction stencilFunc = RenderComparisonFunction::ALWAYS;
	RenderStencilOp stencilFail = RenderStencilOp::KEEP;
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

	float alphaThreshold;
};

struct RenderViewport_r
{
	RenderViewport vp;
	bool isFixed = true;
	bool flushed = false;
};

static GuestSurface* g_renderTarget;
static GuestSurface* g_depthStencil;
static GuestSurface* g_next_RenderTarget; //because of how heavy and fast the game swap the render target, our queue system sometimes faces a race issue, this second pointer copy the current RT earlier as a workaround
static RenderFramebuffer* g_framebuffer;
static RenderViewport_r g_renderviewport = { { 0.0f, 0.0f, 640.0f, 480.0f }, true, false };

static PipelineState g_pipelineState;
static int32_t g_depthBias;
static float g_slopeScaledDepthBias;
int32_t g_RenderUpscaleFactor = 1;
static bool is_loadingThread = false;
static int loadingThreadID = -1;

GuestSurface* GetRenderTarget()
{
	return g_renderTarget;
}

uint32_t g_VertexShader_Handle = 0;
VertexShaderMode g_Xbox_VertexShaderMode = VertexShaderMode::FixedFunction;
static float g_ZScale = 1.0f;
DWORD g_Xbox_VertexShaderConstantMode = 1;

GuestPixelShader* g_pXbox_PixelShader = nullptr;

static uint32_t* g_currentPaletteData;

//constants are variable send to the GPU
static uint32_t g_vertexShaderConstants[HOST_D3DVS_CONSTREG_FULL * 4];
static uint32_t g_vertexShaderConstantsFixed[HOST_D3DVS_CONSTREG_FULL * 4];
static uint32_t g_pixelShaderConstants[PSH_XBOX_CONSTANT_MAX * 4];
static SharedConstants g_sharedConstants; //TextureStage operation data, indice etc. Used with texture descriptor

uint32_t vsh_constants[HOST_D3DVS_CONSTREG_FULL][4]; //xbox registers (192) + host registers (slot of 4 because one register = 4 bytes)
bool vsh_constants_dirty[HOST_D3DVS_CONSTREG_FULL];

static uint64_t g_lastVertexShaderConstants; //used to check for update so we don't upload every frame

uint32_t g_PixelShader;
static uint64_t g_lastPixelShaderConstants;

uint32_t psh_constants[PSH_XBOX_CONSTANT_MAX][4]; // (slot of 4 because one register = 4 bytes)
bool psh_constants_dirty[PSH_XBOX_CONSTANT_MAX];

std::array<D3DMATRIX, X_D3DTS_MAX> g_Transforms;


static GuestTexture* g_textures[TexSlotMax];
static RenderSamplerDesc g_samplerDescs[TexSlotMax];
static TextureCombinerState g_sharedSamplerDescs[TexSlotMax];
static GuestTexture* g_texturesPSDef[4];
static bool g_scissorTestEnable = false;
static RenderRect g_scissorRect;
//static GuestBuffer* g_vertexBuffers[16];

typedef struct _X_STREAMINPUT
{
	GuestBuffer* VertexBuffer;
	UINT                Stride;

} X_STREAMINPUT;

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

static constexpr size_t old_NUM_FRAMES = 2;
static constexpr size_t NUM_FRAMES = 1;
static constexpr size_t NUM_QUERIES = 2;

uint32_t g_frame = 0;
//static uint32_t g_nextFrame = 1;
static uint32_t g_nextFrame = 0;

static std::unique_ptr<RenderCommandQueue> g_queue;
static std::unique_ptr<RenderCommandList> g_commandLists[old_NUM_FRAMES];
static std::unique_ptr<RenderCommandFence> g_commandFences[old_NUM_FRAMES];
static std::unique_ptr<RenderQueryPool> g_queryPools[old_NUM_FRAMES];
static bool g_commandListStates[old_NUM_FRAMES];

static Mutex g_copyMutex;
static std::unique_ptr<RenderCommandQueue> g_copyQueue;
static std::unique_ptr<RenderCommandList> g_copyCommandList;
static std::unique_ptr<RenderCommandFence> g_copyCommandFence;


static std::unique_ptr<RenderSwapChain> g_swapChain;
static bool g_swapChainValid;

static constexpr RenderFormat BACKBUFFER_FORMAT = RenderFormat::B8G8R8A8_UNORM;

static std::unique_ptr<RenderCommandSemaphore> g_acquireSemaphores[old_NUM_FRAMES];
static std::unique_ptr<RenderCommandSemaphore> g_renderSemaphores[old_NUM_FRAMES];
static uint32_t g_backBufferIndex;
static std::unique_ptr<GuestSurface> g_backBufferHolder;
static GuestSurface* g_backBuffer;
static GuestSurface* g_defaultStencil;

static std::unique_ptr<RenderTexture> g_intermediaryBackBufferTexture;
static uint32_t g_intermediaryBackBufferTextureWidth;
static uint32_t g_intermediaryBackBufferTextureHeight;
static uint32_t g_intermediaryBackBufferTextureDescriptorIndex;

struct std::unique_ptr<RenderDescriptorSet> g_textureDescriptorSet;
struct std::unique_ptr<RenderDescriptorSet> g_samplerDescriptorSet;

static xxHashMap<std::pair<uint32_t, std::unique_ptr<RenderSampler>>> g_samplerStates;

static xxHashMap<GuestBuffer*> g_indexedVertexBufferCache;
static xxHashMap<GuestBuffer*> g_indexBufferCache;
static xxHashMap<uint8_t*> g_vertexCache; //Draw Primitive seems to only use dynamic VB for particles, we will keep that as a warning if it ever stop.

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

	template<typename T>
	UploadAllocation allocate(const T* memory, uint32_t size, uint32_t alignment)
	{
		auto result = allocate(size, alignment);
		memcpy(result.memory, memory, size);
		return result;
	}

	void reset()
	{
		index = 0;
		offset = 0;
	}
};

//static UploadAllocator g_uploadAllocators[NUM_FRAMES];
static UploadAllocator g_uploadAllocators[2];

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

static std::vector<GuestResource*> g_tempResources[old_NUM_FRAMES];
static std::vector<std::unique_ptr<RenderBuffer>> g_tempBuffers[old_NUM_FRAMES];


float GetRenderTargetHeight()
{
	if (g_renderTarget->height)
		return g_renderTarget->height;

	printf("ERROR no render target found\n");
	return 0.0f;
}

bool is_vulkan()
{
	return g_vulkan;
}


static std::thread::id g_presentThreadId = std::this_thread::get_id();
static std::atomic<bool> g_readyForCommands;

//experimental hack to see if this fix the random race issue in the loading thread
void Guard_Rendering()
{
	g_readyForCommands.wait(false);
	g_presentThreadId = std::this_thread::get_id();
}

static ankerl::unordered_dense::map<RenderTexture*, RenderTextureLayout> g_barrierMap;

const char* GetLayoutName(RenderTextureLayout layout)
{
	switch (layout)
	{
	default:
	case RenderTextureLayout::UNKNOWN:
		return "Unknown";
	case RenderTextureLayout::GENERAL:
		return "General";
	case RenderTextureLayout::SHADER_READ:
		return "Shader Read";
	case RenderTextureLayout::COLOR_WRITE:
		return "Color Write";
	case RenderTextureLayout::DEPTH_WRITE:
		return "Depth Write";
	case RenderTextureLayout::PRESENT:
		return "Present";
	}
}

static void AddBarrier(GuestBaseTexture* texture, RenderTextureLayout layout)
{
	if (texture != nullptr && texture->layout != layout)
	{
		//printf("AddBarrier called, texture %x, current layout %d (%s) new layout requeted %d (%s) \n", texture->texture, texture->layout, GetLayoutName(texture->layout), layout, GetLayoutName(layout));
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
		{
			//printf("FlushBarriers: Texture %x to layout %d (%s), count: %d\n", texture, layout, GetLayoutName(layout), ++countFail);
			g_barriers.emplace_back(texture, layout);
		}

		g_commandLists[g_frame]->barriers(RenderBarrierStage::GRAPHICS | RenderBarrierStage::COPY, g_barriers);

		g_barrierMap.clear();
		g_barriers.clear();
	}
}

static std::unique_ptr<uint8_t[]> g_shaderCache;
static std::unique_ptr<uint8_t[]> g_buttonBcDiff;

static void LoadEmbeddedResources()
{
	if (g_vulkan)
	{
		g_shaderCache = std::make_unique<uint8_t[]>(g_spirvCacheDecompressedSize);
		ZSTD_decompress(g_shaderCache.get(), g_spirvCacheDecompressedSize, g_compressedSpirvCache, g_spirvCacheCompressedSize);
	}
#ifdef SHADOW_RECOMP_D3D12
	else
	{
		g_shaderCache = std::make_unique<uint8_t[]>(g_dxilCacheDecompressedSize);
		ZSTD_decompress(g_shaderCache.get(), g_dxilCacheDecompressedSize, g_compressedDxilCache, g_dxilCacheCompressedSize);
	}
#endif

	//g_buttonBcDiff = decompressZstd(g_button_bc_diff, g_button_bc_diff_uncompressed_size);
}

static void PrecompileShaders()
{
	printf("Precompiling shaders...\n");

	for (size_t i = 0; i < g_shaderCacheEntryCount; i++)
	{
		auto& entry = g_shaderCacheEntries[i];

		GuestShader* guestShader = g_userHeap.AllocPhysical<GuestShader>(ResourceType::VertexShader);
		guestShader->shaderCacheEntry = &entry;
		entry.guestShader = guestShader;

		if (g_vulkan)
		{
			auto compressedSpirvData = g_shaderCache.get() + entry.spirvOffset;
			std::vector<uint8_t> decoded(smolv::GetDecodedBufferSize(compressedSpirvData, entry.spirvSize));
			bool result = smolv::Decode(compressedSpirvData, entry.spirvSize, decoded.data(), decoded.size());
			assert(result);
			guestShader->shader = g_device->createShader(decoded.data(), decoded.size(), "main", RenderShaderFormat::SPIRV);
		}
#ifdef SHADOW_RECOMP_D3D12
		else
		{
			guestShader->shader = g_device->createShader(
				g_shaderCache.get() + entry.dxilOffset, entry.dxilSize, "main", RenderShaderFormat::DXIL);
		}
#endif
	}
}

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

template<X_D3DPRIMITIVETYPE PrimitiveType>
struct PrimitiveIndexData
{
	std::vector<uint16_t> indexData;
	RenderBufferReference indexBuffer;
	uint32_t currentIndexCount = 0;

	uint32_t prepare(uint32_t guestPrimCount)
	{
		uint32_t primCount;
		uint32_t indexCountPerPrimitive;

		switch (PrimitiveType)
		{
		case X_D3DPT_TRIANGLEFAN:
			primCount = guestPrimCount - 2;
			indexCountPerPrimitive = 3;
			break;
		case X_D3DPT_QUADLIST:
			primCount = guestPrimCount / 4;
			indexCountPerPrimitive = 6;
			break;
		default:
			assert(false && "Unknown primitive type.");
			break;
		}

		uint32_t indexCount = primCount * indexCountPerPrimitive;

		if (indexData.size() < indexCount)
		{
			const size_t oldPrimCount = indexData.size() / indexCountPerPrimitive;
			indexData.resize(indexCount);

			for (size_t i = oldPrimCount; i < primCount; i++)
			{
				switch (PrimitiveType)
				{
				case X_D3DPT_TRIANGLEFAN:
				{
					indexData[i * 3 + 0] = 0;
					indexData[i * 3 + 1] = static_cast<uint16_t>(i + 1);
					indexData[i * 3 + 2] = static_cast<uint16_t>(i + 2);
					break;
				}
				case X_D3DPT_QUADLIST:
				{
					indexData[i * 6 + 0] = static_cast<uint16_t>(i * 4 + 0);
					indexData[i * 6 + 1] = static_cast<uint16_t>(i * 4 + 1);
					indexData[i * 6 + 2] = static_cast<uint16_t>(i * 4 + 2);

					indexData[i * 6 + 3] = static_cast<uint16_t>(i * 4 + 2); //was i + 4 +  0 //maintains consistent clockwise winding for both triangle
					indexData[i * 6 + 4] = static_cast<uint16_t>(i * 4 + 3); //was was i + 4 + 2
					indexData[i * 6 + 5] = static_cast<uint16_t>(i * 4 + 0); //was was i + 4 +  3
					break;
				}
				default:
					assert(false && "Unknown primitive type.");
					break;
				}
			}
		}

		if (indexBuffer == NULL || currentIndexCount < indexCount)
		{
			auto allocation = g_uploadAllocators[g_frame].allocate(indexData.data(), indexCount * 2, 2);
			indexBuffer = allocation.buffer->at(allocation.offset);
			currentIndexCount = indexCount;
		}

		SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.buffer, indexBuffer);
		SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.size, indexCount * 2);
		SetDirtyValue(g_dirtyStates.indices, g_indexBufferView.format, RenderFormat::R16_UINT);

		return indexCount;
	}

	void reset()
	{
		indexBuffer = {};
		currentIndexCount = 0;
	}
};

static PrimitiveIndexData<X_D3DPT_QUADLIST> g_quadIndexData;

struct DerivedVertexBuffer
{
	GuestBuffer* vertexBuffer;
	uint64_t hash;
};

std::unordered_map<GuestBuffer*, std::vector<DerivedVertexBuffer>> VBsList;
std::vector<GuestResource*> g_tempUnreadyResource; // store VB that weren't ready to be deleted
std::atomic<bool> g_pendingSurfaceResizeFlag = false; // flag used to trigger RT and Stencil recreation.

//todo figure out this
static void DestructTempResources()
{
	bool valid = false;

	for (auto resource : g_tempResources[g_frame])
	{
		valid = false;

		switch (resource->type)
		{
		case ResourceType::Texture:
		case ResourceType::VolumeTexture:
		{
			/**const auto texture = reinterpret_cast<GuestTexture*>(resource);

			if (texture->mappedMemory != nullptr)
				g_userHeap.Free(texture->mappedMemory);
			
			g_textureDescriptorAllocator.free(texture->descriptorIndex);
			
			if (texture->patchedTexture != nullptr)
				g_textureDescriptorAllocator.free(texture->patchedTexture->descriptorIndex);
			
			texture->~GuestTexture();*/
			valid = false;
			break;
		}

		case ResourceType::VertexBuffer:
		case ResourceType::IndexBuffer:
		{
			const auto buffer = reinterpret_cast<GuestBuffer*>(resource);
			bool invalid = false;
			for (int i = 0; i < 16; i++)
			{
				if (buffer->buffer->at(0) == g_vertexBufferViews[i].buffer) // ensure Render Thread isn't about to use the VB while we try to delete it
				{
					g_tempUnreadyResource.push_back(resource); //we can store the resource to delete it later when the render thread isn't trying to use it
					invalid = true;
					break;
				}
			}

			if (invalid) //fine to not delete now, it will be deleted next time 
				continue;

			//printf("destruct buffer %x\n", buffer);
			if (buffer->mappedMemory != nullptr)
				g_userHeap.Free(buffer->mappedMemory);

			buffer->~GuestBuffer();

			valid = true;
			break;
		}

		case ResourceType::RenderTarget:
		case ResourceType::DepthStencil:
		{

			const auto surface = reinterpret_cast<GuestSurface*>(resource);

			if (surface->parentTexture != nullptr)
			{
				auto& vec = surface->parentTexture->surfaceRTs;
				vec.erase(std::remove(vec.begin(), vec.end(), surface), vec.end());
			}

			if (surface->descriptorIndex != NULL)
			{
				g_textureDescriptorSet->setTexture(surface->descriptorIndex, nullptr, {});
				g_textureDescriptorAllocator.free(surface->descriptorIndex);
			}

			surface->~GuestSurface();

			valid = true;
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
		}

		if (valid)
			g_userHeap.Free(resource);
	}

	g_tempResources[g_frame].clear();
	g_tempBuffers[g_frame].clear();
}

static ankerl::unordered_dense::set<GuestSurface*> g_pendingSurfaceCopies;
static ankerl::unordered_dense::set<GuestSurface*> g_pendingMsaaResolves;
static ankerl::unordered_dense::set<GuestTexture*> g_pendingTextureResizes;
static GuestSurface* g_pendingStencilResize = nullptr; // the game only use one stencil


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
	DrawIndexedPrimitive,
	DrawPrimitiveUP,
	SetVertexDeclaration,
	SetVertexShader,
	SetStreamSource,
	SetIndices,
	SetPixelShader,

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
			uint32_t stencil;
			bool isFixed;
		} clear;


		struct
		{
			float x;
			float y;
			float width;
			float height;
			float minDepth;
			float maxDepth;
			bool isFixed;
		} setViewport;

		struct
		{
			uint32_t index;
			GuestTexture* texture;
			bool isFixed;
		} setTexture;

		struct
		{
			uint32_t primitiveType;
			uint32_t startVertex;
			uint32_t primitiveCount;
			bool isFixed;
		} drawPrimitive;

		struct
		{
			uint32_t primitiveType;
			int32_t baseVertexIndex;
			uint32_t startIndex;
			uint32_t primCount;
			int16_t* pIndexData;
			GuestBuffer* indexBuffer;
			bool isFixed;
		} drawIndexedPrimitive;

		struct
		{
			uint32_t primitiveType;
			uint32_t primitiveCount;
			uint8_t* vertexStreamZeroData;
			uint32_t vertexStreamZeroSize;
			uint32_t vertexStreamZeroStride;
			bool isFixed;
			uint32_t ID;
			DrawCallCaller caller;
			uint32_t threadID;
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
			GuestShader* shader;
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
			bool isFixedFunc;
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

static RenderBlend ConvertAlphaBlendMode(uint32_t blendMode)
{
	switch (blendMode)
	{
	case D3DBLEND_ZERO:
		return RenderBlend::ZERO;
	case D3DBLEND_ONE:
		return RenderBlend::ONE;
	case D3DBLEND_SRCALPHA:
	case D3DBLEND_SRCCOLOR:
		return RenderBlend::SRC_ALPHA;
	case D3DBLEND_INVSRCALPHA:
	case D3DBLEND_INVSRCCOLOR:
		return RenderBlend::INV_SRC_ALPHA;
	case D3DBLEND_DESTALPHA:
	case D3DBLEND_DESTCOLOR:
		return RenderBlend::DEST_ALPHA;
	case D3DBLEND_INVDESTALPHA:
	case D3DBLEND_INVDESTCOLOR:
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
void SetRenderState(X_D3DRENDERSTATETYPE state, uint32_t value)
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
	int32_t GuestRenderStateIndex = -1;

	for (int32_t i = X_D3DRS_PS_FIRST; i <= X_D3DRS_LAST; i++)
	{
		if (GetDxbxRenderStateInfo(i).M == PUSH_METHOD(Method))
		{
			GuestRenderStateIndex = i;
			break;
		}
	}

	if (GuestRenderStateIndex == -1) {
		printf("RenderState_Simple could not be found in RenderState table Method %x, Value %x\n", Method, Value);
		return;
	}

	SetGuestRenderState(GuestRenderStateIndex, Value);
}


static void SetRenderStateUnimplemented(GuestDevice* device, uint32_t value)
{

}

static void ProcSetRenderState(const RenderCommand& cmd)
{
	uint32_t value = cmd.setRenderState.value;

	switch (cmd.setRenderState.type)
	{
	case X_D3DRS_VERTEXBLEND:
	{

		break;
	}
	case X_D3DRS_FOGCOLOR:
	{
		uint32_t fog = value;

		float r = ((fog >> 16) & 0xFF) / 255.0f;
		float g = ((fog >> 8) & 0xFF) / 255.0f;
		float b = ((fog >> 0) & 0xFF) / 255.0f;

		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogColor[0], r);
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogColor[1], g);
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogColor[2], b);

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
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilEnable, value != 0);
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
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilFail, ConvertStencilOp(value));
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
		uint32_t tf = value;


		float r = ((tf >> 16) & 0xFF) / 255.0f;
		float g = ((tf >> 8) & 0xFF) / 255.0f;
		float b = ((tf >> 0) & 0xFF) / 255.0f;
		float a = ((tf >> 24) & 0xFF) / 255.0f;

		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.TextureFactor[0], r);
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.TextureFactor[1], g);
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.TextureFactor[2], b);
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.TextureFactor[3], a);

		break;
	}
	case X_D3DRS_ZBIAS:
	{
		FLOAT Biased = static_cast<FLOAT>(value) * -0.000005f;
		value = *reinterpret_cast<const DWORD*>(&Biased);

		if (g_capabilities.dynamicDepthBias)
			SetDirtyValue(g_dirtyStates.depthBias, g_depthBias, int32_t(*reinterpret_cast<float*>(&value)));
		else
			SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthBias, int32_t(*reinterpret_cast<float*>(&value)));

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
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.srcBlendAlpha, ConvertAlphaBlendMode(value)); //DX8 originally does not have src blend alpha so we mirror "regular" blend.
		break;
	}
	case X_D3DRS_DESTBLEND:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlend, ConvertBlendMode(value));
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.destBlendAlpha, ConvertAlphaBlendMode(value)); //DX8 originally does not have dst blend alpha so we mirror "regular" blend.
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
	case X_D3DRS_STENCILZFAIL:
	{
		SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.stencilZFail, ConvertStencilOp(value));
		break;
	}
	case X_D3DRS_STENCILPASS:
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
	}
	break;
	//fog is handled in shader constant directly
	case X_D3DRS_FOGTABLEMODE:
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogTableMode, value);
		break;
	case X_D3DRS_FOGDENSITY:
		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogDensity, value);
		break;
	case X_D3DRS_FOGSTART:
	{
		float fogValue = *(float*)&value;
		if (fogValue < 0.0f) {
			fogValue = std::abs(fogValue);
			value = *(DWORD*)&fogValue;
		}

		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogStart, value);
	}
	break;
	case X_D3DRS_FOGEND:
	{
		float fogValue = *(float*)&value;
		if (fogValue < 0.0f) {
			fogValue = std::abs(fogValue);
			value = *(DWORD*)&fogValue;
		}

		SetDirtyValue(g_dirtyStates.sharedConstants, g_sharedConstants.FogEnd, value);
	}
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

	}*/

	//g_renderState[cmd.setRenderState.type] = value;
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

#define CREATE_RUNTIME_SHADER(data, size) \
    g_device->createShader(data, size, "main", \
	g_vulkan ? RenderShaderFormat::SPIRV : RenderShaderFormat::DXIL)

#else

#define CREATE_SHADER(NAME) \
    g_device->createShader(g_##NAME##_spirv, sizeof(g_##NAME##_spirv), "main", RenderShaderFormat::SPIRV)

#define CREATE_RUNTIME_SHADER(data, size) \
   g_device->createShader(data, size, "main", RenderShaderFormat::SPIRV)

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

// this new commandList is needed for the unitialized resource (RT / Depth) that need to be cleared before being used
// To avoid race issues with the main command list, we use a separate command list and queue.
static std::mutex g_initMutex;
static std::unique_ptr<RenderCommandList> g_initCommandList;
static std::unique_ptr<RenderCommandFence> g_initCommandFence;
static std::unique_ptr<RenderCommandQueue> g_initQueue;


template<typename T>
static void ExecuteInitCommandList(const T& function)
{
	std::lock_guard lock(g_initMutex);
	g_initCommandList->begin();
	function();
	g_initCommandList->end();
	g_initQueue->executeCommandLists(g_initCommandList.get(), g_initCommandFence.get());
	g_initQueue->waitForCommandFence(g_initCommandFence.get());
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
	LOG_UTILITY("Init ImGui, please wait...\n");
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

#ifdef ENABLE_IM_FONT_ATLAS_SNAPSHOT
	IM_DELETE(io.Fonts);
	io.Fonts = ImFontAtlasSnapshot::Load();

#else
	ImFontAtlasSnapshot::GenerateGlyphRanges();
	ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-DB.otf");
	ImFontAtlasSnapshot::GetFont("DFPZongYiW5-B5.ttc");

#endif


	/**AchievementMenu::Init();
	AchievementOverlay::Init();*/



	ButtonGuide::Init();
	InstallerWizard::Init();
	OptionsMenu::Init();
	MessageWindow::Init();


	ImGui_ImplSDL2_InitForOther(GameWindow::s_pWindow);


#ifdef ENABLE_IM_FONT_ATLAS_SNAPSHOT 
	uint8_t* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	g_imFontTexture = LoadCustomTexture(decompressZstd(g_im_font_atlas_texture, g_im_font_atlas_texture_uncompressed_size).get(), g_im_font_atlas_texture_uncompressed_size);
#else
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
#endif

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
	uint8_t* pixelsCopy;
	int atlasW = 0, atlasH = 0, bpp = 0;
	io.Fonts->GetTexDataAsRGBA32(&pixelsCopy, &atlasW, &atlasH, &bpp);

	stbi_write_png("im_font_atlas.png", atlasW, atlasH, bpp, pixelsCopy, atlasW * bpp);
#endif

	LOG_UTILITY("Done.\n");
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

// since the game has RT that don't always match full resolution (example 512x512 instead of 640x480)
// we can adapt the math to match this behaviour with user's resolution
void GetUpscaledSurfaceResolution(uint32_t widthOrigin, uint32_t heightOrigin,
	uint32_t* widthOut, uint32_t* heightOut)
{
	float percentW = (float)widthOrigin / 640.f;
	float percentH = (float)heightOrigin / 480.f;
	float resW = percentW * Video::s_viewportWidth * Config::ResolutionScale;
	float resH = percentH * Video::s_viewportHeight * Config::ResolutionScale;

	*widthOut = std::max((uint32_t)resW, widthOrigin);
	*heightOut = std::max((uint32_t)resH, heightOrigin);
}

// This is needed because uninitialized RT / depth can crash the game under specific windows version if (and only if) VS is installed.
void DiscardInitRenderResource(RenderTexture* texture, const bool isRT)
{

	auto targetLayout = (isRT == false)
		? RenderTextureLayout::DEPTH_WRITE
		: RenderTextureLayout::COLOR_WRITE;

	ExecuteInitCommandList([&]
		{
			g_initCommandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(texture, targetLayout));
			g_initCommandList->discardTexture(texture);
			g_initCommandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(texture, RenderTextureLayout::SHADER_READ));
		});
}

// This is used when user request new resolution / aspect ratio
// we have to remade all RT and Depth stencil texture 
void RecreateGuestSurfaceTexture(GuestBaseTexture* tex, bool isRT)
{

	auto targetLayout = (isRT == false)
		? RenderTextureLayout::DEPTH_WRITE
		: RenderTextureLayout::COLOR_WRITE;


	uint32_t newWidth, newHeight;

	GetUpscaledSurfaceResolution(tex->g_width, tex->g_height, &newWidth, &newHeight);
	tex->textureHolder = g_device->createTexture(RenderTextureDesc::Texture2D(newWidth, newHeight, 1, tex->format, isRT ? RenderTextureFlag::RENDER_TARGET : RenderTextureFlag::DEPTH_TARGET));
	tex->texture = tex->textureHolder.get();

	ExecuteInitCommandList([&] // Discard texture, this is needed, see the function DiscardInitRenderResource for more info. 
		{
			g_initCommandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(tex->textureHolder.get(), targetLayout));
			g_initCommandList->discardTexture(tex->textureHolder.get());

			RenderFramebufferDesc fbDesc;

			if (isRT == true)
			{
				fbDesc.colorAttachments = const_cast<const RenderTexture**>(&tex->texture);
				fbDesc.colorAttachmentsCount = 1;
			}
			else
			{
				fbDesc.depthAttachment = tex->textureHolder.get();

			}

			auto tempFB = g_device->createFramebuffer(fbDesc);
			g_initCommandList->setFramebuffer(tempFB.get());

			if (isRT == false)
				g_initCommandList->clearDepthStencil();
			else
				g_initCommandList->clearColor();

			g_initCommandList->setFramebuffer(nullptr);

			g_initCommandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(tex->textureHolder.get(), (isRT == false) ? RenderTextureLayout::DEPTH_WRITE : RenderTextureLayout::SHADER_READ));
		});



	tex->width = newWidth;
	tex->height = newHeight;

	if (!isRT)
	{
		g_textureDescriptorSet->setTexture(tex->descriptorIndex, tex->textureHolder.get(), RenderTextureLayout::DEPTH_WRITE, tex->textureView.get());
		AddBarrier(tex, RenderTextureLayout::DEPTH_READ);
	}
	else
	{
		g_textureDescriptorSet->setTexture(tex->descriptorIndex, tex->texture, RenderTextureLayout::SHADER_READ);
		AddBarrier(tex, RenderTextureLayout::SHADER_READ);
	}
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
			(abs(Config::Brightness.Value - 0.5f) > 0.001f) || g_pendingSurfaceResizeFlag;

		if (g_pendingSurfaceResizeFlag)
		{
			for (auto& g_tex : g_pendingTextureResizes)
			{
				g_barrierMap.erase(g_tex->texture);

				if (g_tex->isRenderTarget)
				{
					for (auto* s : g_tex->surfaceRTs)
					{
						s->framebuffers.clear();
						s->texture = nullptr;
					}
				}

				RecreateGuestSurfaceTexture(g_tex, g_tex->isRenderTarget);

				if (g_tex->isRenderTarget)
				{
					for (auto* s : g_tex->surfaceRTs)
					{
						s->texture = g_tex->texture;
						s->width = g_tex->width;
						s->height = g_tex->height;
					}
				}
			}

			if (g_pendingStencilResize)
			{
				g_barrierMap.erase(g_pendingStencilResize->texture);
				g_pendingStencilResize->framebuffers.clear();
				RecreateGuestSurfaceTexture(g_pendingStencilResize, false);
			}

			g_pendingSurfaceResizeFlag = false;
		}

		if (usingIntermediaryTexture)
		{
			if (g_intermediaryBackBufferTextureWidth != width ||
				g_intermediaryBackBufferTextureHeight != height)
			{
				if (g_intermediaryBackBufferTextureDescriptorIndex == NULL)
					g_intermediaryBackBufferTextureDescriptorIndex = g_textureDescriptorAllocator.allocate();


				Video::WaitForGPU(); // Fine to wait for GPU, this'll only happen during resize.

				g_intermediaryBackBufferTexture = g_device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET));

				ExecuteInitCommandList([&] // Discard texture, this is needed, see the function DiscardInitRenderResource for more info. 
					{
						g_initCommandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(g_intermediaryBackBufferTexture.get(), RenderTextureLayout::COLOR_WRITE));
						g_initCommandList->discardTexture(g_intermediaryBackBufferTexture.get());
						g_initCommandList->barriers(RenderBarrierStage::GRAPHICS, RenderTextureBarrier(g_intermediaryBackBufferTexture.get(), RenderTextureLayout::SHADER_READ));
					});

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

	// Below is commented out as it was causing Stage1 textures to render as null for a few frames under certain camera angles
	// See https://github.com/FoxWorkshop/ShadowRecomp/issues/31
	/*
	for (size_t i = 0; i < 16; i++)
	{
		g_sharedConstants.texture2DIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_2D;
		g_sharedConstants.texture3DIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_3D;
		g_sharedConstants.textureCubeIndices[i] = TEXTURE_DESCRIPTOR_NULL_TEXTURE_CUBE;
	}

	memset(g_textures, 0, sizeof(g_textures));
	*/
	
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
				os::process::StartProcess(os::process::GetExecutablePath(), { "--graphics-api-retry" });
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

	LoadEmbeddedResources();
	// PrecompileShaders(); // Temp disabled due until pipeline cache implemented properly

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

	// used for unitial resource (RT / Depth) as explained above
	g_initQueue = g_device->createCommandQueue(RenderCommandListType::DIRECT);
	g_initCommandList = g_initQueue->createCommandList();
	g_initCommandFence = g_device->createCommandFence();

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

	RenderSwapChainDesc swapChainDesc;
	swapChainDesc.renderWindow = GameWindow::s_renderWindow;
	swapChainDesc.textureCount = bufferCount;
	swapChainDesc.format = BACKBUFFER_FORMAT;
	swapChainDesc.maxFrameLatency = Config::MaxFrameLatency;
	swapChainDesc.enablePresentWait = g_capabilities.presentWait;

	g_swapChain = g_queue->createSwapChain(swapChainDesc);
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




	CreateImGuiBackend();

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
	g_backBuffer->g_width = g_backBuffer->width;
	g_backBuffer->g_height = g_backBuffer->height;
	g_backBuffer->format = BACKBUFFER_FORMAT;
	g_backBuffer->textureHolder = g_device->createTexture(RenderTextureDesc::Texture2D(1, 1, 1, BACKBUFFER_FORMAT, RenderTextureFlag::RENDER_TARGET));

	DiscardInitRenderResource(g_backBuffer->textureHolder.get(), true);
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


static GuestVertexDeclaration* CreateVertexDeclaration(D3DVERTEXELEMENT9* vertexElements, VertexShaderMode shaderMode)
{
	auto vertexDeclaration = CreateVertexDeclarationWithoutAddRef(vertexElements, shaderMode);
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
}


static ShaderCacheEntry* FindShaderCacheEntry(XXH64_hash_t hash)
{
	auto end = g_shaderCacheEntries + g_shaderCacheEntryCount;
	auto findResult = std::lower_bound(g_shaderCacheEntries, end, hash, [](ShaderCacheEntry& lhs, XXH64_hash_t rhs)
		{
			return lhs.hash < rhs;
		});

	return findResult != end && findResult->hash == hash ? findResult : nullptr;
}

X_D3DRESOURCETYPE GetXboxTextureType(const X_D3DResource* pXboxResource);
GuestShader* CreateShader(ResourceType resourceType)
{
	auto base = g_memory.base;
	const bool isVertexShader = resourceType == ResourceType::VertexShader;
	uint32_t psDefReg = 0;

	if (isVertexShader == false && g_pXbox_PixelShader)
	{
		psDefReg = g_memory.MapVirtual(GetPixelShaderRenderStatePointer());
	}

	uint64_t hash = psDefReg > 0 ? GetPixelShaderKey(psDefReg) : GetVertexShaderKey();
	auto findCache = FindShaderCacheEntry(hash);

	GuestShader* shader = nullptr;

	if (findCache != nullptr)
	{
		if (findCache->guestShader == nullptr)
		{

			shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);

			shader->shaderCacheEntry = findCache;

			findCache->guestShader = shader;
		}
		else
		{
			shader = findCache->guestShader;
		}
	}


	if (shader)
	{
		shader->AddRef();
	}
	else 	// old system, will be used if shader_cache.cpp is missing
	{
		shader = isVertexShader ? FindShaderCacheEntryNew() : FindPixelShaderCacheEntryNew(psDefReg);

		size_t hostVSHandle = (size_t)g_memory.Translate(g_VertexShader_Handle);

		if (shader == nullptr)
		{
			shader = g_userHeap.AllocPhysical<GuestShader>(resourceType);

			if (resourceType == ResourceType::PixelShader)
			{
				size_t sizeShader = 0;

				X_D3DPIXELSHADERDEF* pXboxPixelShaderDef = (X_D3DPIXELSHADERDEF*)(g_memory.Translate(psDefReg));
				pixelShader_GuestInfo psdGuestInfo;
				memset(&psdGuestInfo, 0, sizeof(psdGuestInfo));
				psdGuestInfo.PSTextureModes = GetGuestRenderState(X_D3DRS_PSTEXTUREMODES);
				psdGuestInfo.renderstateFogEnable = GetGuestRenderState(X_D3DRS_FOGENABLE) > 0;
				psdGuestInfo.renderStateSpecularEnable = GetGuestRenderState(X_D3DRS_SPECULARENABLE) > 0;

				for (int i = 0; i < 4; i++)
				{
					if (g_texturesPSDef[i])
					{
						unsigned long type = (unsigned long)GetXboxTextureType((X_D3DResource*)((size_t)&g_texturesPSDef[i]->guestResource));
						psdGuestInfo.ActiveTextureTypes[i] = (uint32_t)type;
					}
				}

				auto shaderData = CreateRecompiledPixelShader(pXboxPixelShaderDef, &psdGuestInfo, &sizeShader);
				shader->shader = CREATE_RUNTIME_SHADER(shaderData, sizeShader);

				UpdatePixelShaderCacheEntryNew(shader, hash);
			}
			else
			{

				if (hash == 0xb2b7a0a5e7d5805f)
				{
					shader->shader = CREATE_SHADER(R2D_vs0);
				}
				else if (hash == 0xcefa8ae484bff7a2)
				{
					shader->shader = CREATE_SHADER(R2D_vs1);
				}
				else
				{
					size_t sizeShader = 0;
					auto shaderData = CreateRecompiledShader(hostVSHandle, &sizeShader);
					shader->shader = CREATE_RUNTIME_SHADER(shaderData, sizeShader);
				}

				UpdateShaderCacheEntryNew(shader, hash);
			}
		}
	}

	return shader;
}

// Get viewport offset and scale values intended to match the XDK calculations
void GetXboxViewportOffsetAndScale(float(&vOffset)[4], float(&vScale)[4])
{
	// Antialiasing mode affects the viewport offset and scale
	float aaScaleX = 1.0f;
	float aaScaleY = 1.0f;
	float aaOffsetX = 0.0f;
	float aaOffsetY = 0.0f;
	//GetScreenScaleFactors(aaScaleX, aaScaleY); //todo check if this is needed

	auto g_viewport = g_renderviewport.vp;

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
	float xboxRenderTargetWidth = g_next_RenderTarget->g_width;
	float xboxRenderTargetHeight = g_next_RenderTarget->g_height;
	//GetRenderTargetBaseDimensions(xboxRenderTargetWidth, xboxRenderTargetHeight);

	float screenScaleX = 1.0f;
	float screenScaleY = 1.0f;
	float aaOffsetX = 0.0f; // used to be 0.5 but this creates visible white bar at the bottom and right side of the screen 
	float aaOffsetY = 0.0f;

	//GetScreenScaleFactors(screenScaleX, screenScaleY);
	//GetMultiSampleOffset(aaOffsetX, aaOffsetY);


	float xboxScreenspaceWidth = xboxRenderTargetWidth * screenScaleX;
	float xboxScreenspaceHeight = xboxRenderTargetHeight * screenScaleY;

	// Passthrough should range 0 to 1, instead of 0 to zbuffer depth
	float zOutputScale = g_Xbox_VertexShaderMode == VertexShaderMode::Passthrough ? 1 : g_ZScale;

	float screenspaceScale[4] = { xboxScreenspaceWidth / 2,  -xboxScreenspaceHeight / 2, zOutputScale, 1 };
	float screenspaceOffset[4] = { xboxScreenspaceWidth / 2 + aaOffsetX, xboxScreenspaceHeight / 2 + aaOffsetY, 0, 0 };

	CopyVertexShaderConstant(HOST_D3DVS_SCREENSPACE_SCALE_BASE, (DWORD*)screenspaceScale, HOST_D3DVS_NORMALIZE_SCALE_SIZE);
	CopyVertexShaderConstant(HOST_D3DVS_SCREENSPACE_OFFSET_BASE, (DWORD*)screenspaceOffset, HOST_D3DVS_NORMALIZE_OFFSET_SIZE);
	// Store viewport offset and scale in constant registers 58 (c-38) and
	// 59 (c-37) used for screen space transformation.
	// We only do this if X_D3DSCM_NORESERVEDCONSTANTS is not set,
	// since enabling this flag frees up these registers for shader use
	if (!(g_Xbox_VertexShaderConstantMode & X_D3DSCM_NORESERVEDCONSTANTS))
	{
		CopyVertexShaderConstant(X_D3DSCM_RESERVED_CONSTANT_SCALE_CORRECTED, reinterpret_cast<DWORD*>(vScaleOffset), 2);
	}
}

void UpdateVertexDeclaration(uint32_t Handle)
{
	size_t host_handle = (size_t)g_memory.Translate(Handle);

	//DWORD vshFlags = GetXboxVertexShaderForHandle(host_handle)->Flags;
	RenderCommand cmd;
	VertexShaderMode shaderMode = g_Xbox_VertexShaderMode;

	//as mentioned above, there is no separate function that create vertex declaration, so we have to manually extract and handle them.
	//we handle vertex declaration immediately to avoid a race issue.

	auto vertexElement = XSR_HostGetVertexDeclaration(host_handle, shaderMode == VertexShaderMode::FixedFunction);

	UpdateHostVertexDeclaration(vertexElement);
	auto vertexDeclaration = CreateVertexDeclaration(vertexElement->pHostVertexElements, shaderMode);
	SetVertexDeclaration(vertexDeclaration);
}

void UpdateVertexShader(uint32_t Handle)
{
	size_t host_handle = (size_t)g_memory.Translate(Handle);
	DWORD vshFlags = GetXboxVertexShaderForHandle(host_handle)->Flags;
	RenderCommand cmd;
	bool isFixed = false;

	if (vshFlags & g_X_VERTEXSHADER_FLAG_PROGRAM)
	{
		auto shader = CreateShader(ResourceType::VertexShader);
		if (shader == nullptr)
		{
			return;
		}
		else
		{
			cmd.setVertexShader.shader = shader;
		}
	}
	else
	{
		g_VertexShader_Handle = Handle;

		SetFixedFunctionDefaultVertexAttributes(host_handle);

		// Switch to passthrough program, if so required
		if (vshFlags & X_VERTEXSHADER_FLAG_PASSTHROUGH)
		{
			SetVertexShaderPassthroughProgram();
			g_Xbox_VertexShaderMode = VertexShaderMode::Passthrough;
		}
		else {

			g_Xbox_VertexShaderMode = VertexShaderMode::FixedFunction;

		}
	}



	cmd.type = RenderCommandType::SetVertexShader;
	cmd.setVertexShader.handle = Handle;
	cmd.setVertexShader.type = g_Xbox_VertexShaderMode;

	UpdateVertexDeclaration(Handle);

	g_renderQueue.enqueue(cmd);
}

// DX8 has a fixed pipeline, meaning we won't always have a shader from this function, sometimes it can be FVF too
// Since DX12 only support Shader, we will use a uber shader to emulate FVF stuff, see FixedFunctionVertexShader.hlsl
X86_FUNC_IMPL(__imp__sub_3D2280);
static void SetVertexShader(uint32_t Handle)
{

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
	g_Xbox_VertexShaderMode = VertexShaderMode::ShaderProgram;
	g_VertexShader_Handle = Handle;
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

// SetPixelShader
X86_FUNC_IMPL(__imp__sub_3DC590);
X86_FUNC(sub_3DC590)
{
	auto Handle = X86_LOAD_U32(ctx.esp.u32 + 4);


	__imp__sub_3DC590(ctx, base);

	RenderCommand cmd;
	cmd.type = RenderCommandType::SetPixelShader;
	g_pXbox_PixelShader = (GuestPixelShader*)g_memory.Translate(Handle);
	if (Handle)
	{
		// Copy the Pixel Shader data to our RenderState handler (this includes values for pixel shader constants)
		// This mirrors the fact that unpatched SetPixelShader does the same thing!

		auto pXboxPixelShaderDef = X86_LOAD_U32(Handle + 0x8);
		X_D3DPIXELSHADERDEF* pPSDef = (X_D3DPIXELSHADERDEF*)g_memory.Translate(pXboxPixelShaderDef);
		memcpy(GetPixelShaderRenderStatePointer(), pPSDef, sizeof(X_D3DPIXELSHADERDEF) - 3 * sizeof(DWORD));
		// Copy the PSDef.PSTextureModes field to it's dedicated slot, which lies outside the range of PixelShader render state slots
		// Note : This seems to be what XDK's do as well. Needs verification.
		SetGuestRenderState(X_D3DRS_PSTEXTUREMODES, pPSDef->PSTextureModes);

		auto shader = CreateShader(ResourceType::PixelShader);

		if (shader == nullptr)
		{
			return;
		}
		else
		{
			cmd.setPixelShader.shader = shader;
		}
	}
	else
	{
		cmd.setPixelShader.shader = nullptr;
	}

	//SetGuestRenderState(X_D3DRS_PSTEXTUREMODES, 

	cmd.setPixelShader.isFixed = Handle == 0;
	g_renderQueue.enqueue(cmd);
}

//SetPixelShaderConstant
X86_FUNC_IMPL(__imp__sub_3DC790);
X86_FUNC(sub_3DC790)
{
	__imp__sub_3DC790(ctx, base);
	UpdatePixelShaderConstant(psh_constants);

}
bool isNonFixedPS = false;


static void ProcSetPixelShader(const RenderCommand& cmd)
{
	GuestShader* shader = cmd.setPixelShader.shader;
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.pixelShader, shader);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.isFixedFunctionPS, cmd.setPixelShader.isFixed);
}

//Stub for now
//for what I saw on CXBX, there is no need to track them as they are simply regular RenderState which we already have
//the only advantage to track them would be for optimisation 
static void SetPixelShaderConstants(int32_t Register, __m128* pConstantData, int32_t ConstantCount)
{

}

//SetStreamSource
X86_FUNC_IMPL(__imp__sub_3D1E10);
X86_FUNC(sub_3D1E10) {

	auto StreamNumber = (uint32_t)GetHostParameter(1);
	auto buffer = (GuestBuffer*)GetHostParameter(2, CallConvention::Stdcall, ParameterType::Pointer);
	auto stride = (uint32_t)GetHostParameter(3);


	g_Xbox_SetStreamSource[StreamNumber].VertexBuffer = buffer;
	g_Xbox_SetStreamSource[StreamNumber].Stride = stride;

	//we need to let the original code run otherwise this preven't other vertex buffer to be used
	__imp__sub_3D1E10(ctx, base);
}


//offset might be 0 but I'm not sure, to do check
static void ProcSetStreamSource(const RenderCommand& cmd)
{

	const auto& args = cmd.setStreamSource;


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

	ImFont* font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-DB.otf"); //ImGui::GetIO().Fonts->Fonts[0];

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
		ImGui::Checkbox("Show FPS", &Config::ShowFPS.Value);
		ImGui::Checkbox("2P Lite", &Config::Enable2PLite.Value);
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
	auto font = ImFontAtlasSnapshot::GetFont("FOT-SeuratPro-DB.otf");
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

	OptionsMenu::Draw();
	/**AchievementMenu::Draw();
	AchievementOverlay::Draw();*/

	ButtonGuide::Draw();


	InstallerWizard::Draw();
	//BlackBar::Draw();
	MessageWindow::Draw();
	Fader::Draw();

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

		auto vertexBufferAllocation = g_uploadAllocators[g_frame].allocate(drawList->VtxBuffer.Data, drawList->VtxBuffer.Size * sizeof(ImDrawVert), alignof(ImDrawVert));
		auto indexBufferAllocation = g_uploadAllocators[g_frame].allocate(drawList->IdxBuffer.Data, drawList->IdxBuffer.Size * sizeof(uint16_t), alignof(uint16_t));

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
		if (g_swapChainValid && g_capabilities.presentWait)
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
static std::atomic<bool> presentExecuted(true);


static int copyCount;
static int g_drawPrimitiveUP_ID = 0;
bool PresentAllowed = true;
extern std::atomic<bool> aboutToDrawLoading;
void Video::Present()
{

	//printf("[GAME THREAD] Present thread id %d - g_frame=%u\n", GetCurrentThreadId(), g_frame);

	g_readyForCommands = false;

	RenderCommand cmd;
	cmd.type = RenderCommandType::ExecutePendingCopyRectsCommands;
	g_renderQueue.enqueue(cmd);

	DrawImGui();

	cmd.type = RenderCommandType::ExecuteCommandList;
	g_renderQueue.enqueue(cmd);


	if (g_shouldPrecompilePipelines)
	{
		//EnqueuePipelineTask(PipelineTaskType::PrecompilePipelines, {});
		g_shouldPrecompilePipelines = false;
	}

	//printf("[GAME THREAD] Enqueued ExecuteCommandList, waiting...\n");
	presentExecuted = false;
	g_executedCommandList.wait(false);
	g_executedCommandList = false;
	//printf("[GAME THREAD] ExecuteCommandList completed\n");


	if (g_swapChainValid)
	{
		if (g_pendingWaitOnSwapChain && g_capabilities.presentWait)
		{
			g_presentWaitProfiler.Begin();
			g_swapChain->wait(); // Never gonna happen outside loading threads as explained above.
			g_presentWaitProfiler.End();
		}

		RenderCommandSemaphore* signalSemaphores[] = { g_renderSemaphores[g_frame].get() };
		g_swapChainValid = g_swapChain->present(g_backBufferIndex, signalSemaphores, std::size(signalSemaphores));
	}
	presentExecuted = true;

	g_pendingWaitOnSwapChain = true;

	uint32_t oldFrame = g_frame;
	g_frame = g_nextFrame;
	g_nextFrame = (g_frame + 1) % NUM_FRAMES;

	//printf("[GAME THREAD] SWITCHED FRAME: %u -> %u\n", oldFrame, g_frame);


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
	/**g_triangleFanIndexData.reset();*/
	g_quadIndexData.reset();

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
	//printf("[GAME THREAD] Present END - g_frame=%u\n", g_frame);
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

			float offset = (Config::Brightness.Value - 0.5f) * 1.2f;

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

			SetRootDescriptor(g_uploadAllocators[g_frame].allocate(&constants, sizeof(constants), 0x100), 2);
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



//todo finish
void Video::ComputeViewportDimensions()
{
	uint32_t width = g_swapChain->getWidth();
	uint32_t height = g_swapChain->getHeight();
	float aspectRatio = float(width) / float(height);

	switch (Config::AspectRatio)
	{
	case EAspectRatio::Wide:
	{
		if (aspectRatio > WIDE_ASPECT_RATIO)
		{
			s_viewportWidth = height * 16 / 9;
			s_viewportHeight = height;
		}
		else
		{
			s_viewportWidth = width;
			s_viewportHeight = width * 9 / 16;
		}

		break;
	}

	case EAspectRatio::Narrow:
	case EAspectRatio::OriginalNarrow:
	{
		if (aspectRatio > NARROW_ASPECT_RATIO)
		{
			s_viewportWidth = height * 4 / 3;
			s_viewportHeight = height;
		}
		else
		{
			s_viewportWidth = width;
			s_viewportHeight = width * 3 / 4;
		}

		break;
	}

	default:
		s_viewportWidth = width;
		s_viewportHeight = height;
		break;
	}

	auto base = g_memory.base;

	//update rwRASTERTYPECAMERA resolution (main 640x480 rt)
	//X86_STORE_U32(0x4E5D3C, width);
	//X86_STORE_U32(0x4E5D40, height);

	//update rwRASTERTYPEZBUFFER and rwRASTERTYPECAMERATEXTURE resolution
	/**GetUpscaledRTResolution(512, 512, &width, &height);
	X86_STORE_U32(0x4E5D50, width);
	X86_STORE_U32(0x4E5D54, height);

	GetUpscaledRTResolution(512, 384, &width, &height);
	X86_STORE_U32(0x4E5D64, width);
	X86_STORE_U32(0x4E5D68, height);*/

	//X86_STORE_F32(0x4E5D44, aspectRatio);

	AspectRatioPatches::ComputeOffsets();
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

		return RenderFormat::D32_FLOAT;
	default:

		assert(false && "Unknown format");
		return RenderFormat::R8_UNORM;
	}
}


//todo check if this is all correct
static GuestTexture* CreateTexture(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, X_D3DFORMAT format, X_D3DRESOURCETYPE type)
{
	auto texture = g_userHeap.AllocPhysical<GuestTexture>(type == X_D3DRTYPE_VOLUMETEXTURE ? ResourceType::VolumeTexture : ResourceType::Texture);

	texture->g_width = width; // store original info for resolution scaling later 
	texture->g_height = height;

	RenderTextureDesc desc;
	desc.dimension = RenderTextureDimension::TEXTURE_2D;

	if (usage == 1) //will be RenderTarget
	{
		uint32_t newWidth, newHeight;
		GetUpscaledSurfaceResolution(width, height, &newWidth, &newHeight);
		width = newWidth;
		height = newHeight;
		g_pendingTextureResizes.insert(texture);
	}


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


	texture->isRenderTarget = (desc.flags & RenderTextureFlag::RENDER_TARGET) != 0;

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

	const bool isRT = ((desc.flags & RenderTextureFlag::RENDER_TARGET) != 0);
	const bool isStencil = ((desc.flags & RenderTextureFlag::DEPTH_TARGET) != 0);

	if (isStencil || isRT)
	{
		DiscardInitRenderResource(texture->texture, isRT);
	}


	g_textureDescriptorSet->setTexture(texture->descriptorIndex, texture->texture, RenderTextureLayout::SHADER_READ, texture->textureView.get());

	return texture;
}

static GuestTexture* CreateTexture_(uint32_t width, uint32_t height, uint32_t depth, uint32_t levels, uint32_t usage, X_D3DFORMAT format, X_D3DRESOURCETYPE type)
{
	return CreateTexture(width, height, depth, levels, usage, format, type);
}

static bool LoadTexture(GuestTexture& texture, const uint8_t* data, size_t dataSize, RenderComponentMapping componentMapping, bool forceCubeMap = false)
{

	int width, height;
	void* stbImage = stbi_load_from_memory(data, dataSize, &width, &height, nullptr, 4);

	if (stbImage != nullptr)
	{
		texture.width = width;
		texture.height = height;
		texture.textureHolder = g_device->createTexture(RenderTextureDesc::Texture2D(width, height, 1, RenderFormat::R8G8B8A8_UNORM));
		texture.texture = texture.textureHolder.get();
		texture.viewDimension = RenderTextureViewDimension::TEXTURE_2D;
		texture.layout = RenderTextureLayout::COPY_DEST;

		texture.descriptorIndex = g_textureDescriptorAllocator.allocate();
		g_textureDescriptorSet->setTexture(texture.descriptorIndex, texture.texture, RenderTextureLayout::SHADER_READ);

		uint32_t rowPitch = (width * 4 + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
		uint32_t slicePitch = rowPitch * height;

		auto uploadBuffer = g_device->createBuffer(RenderBufferDesc::UploadBuffer(slicePitch));
		uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(uploadBuffer->map());

		if (rowPitch == (width * 4))
		{
			memcpy(mappedMemory, stbImage, slicePitch);
		}
		else
		{
			auto data = reinterpret_cast<const uint8_t*>(stbImage);

			for (size_t i = 0; i < height; i++)
			{
				memcpy(mappedMemory, data, width * 4);
				data += width * 4;
				mappedMemory += rowPitch;
			}
		}

		uploadBuffer->unmap();

		stbi_image_free(stbImage);

		ExecuteCopyCommandList([&]
			{
				g_copyCommandList->barriers(RenderBarrierStage::COPY, RenderTextureBarrier(texture.texture, RenderTextureLayout::COPY_DEST));

				g_copyCommandList->copyTextureRegion(
					RenderTextureCopyLocation::Subresource(texture.texture, 0),
					RenderTextureCopyLocation::PlacedFootprint(uploadBuffer.get(), RenderFormat::R8G8B8A8_UNORM, width, height, 1, rowPitch / 4, 0));
			});

		return true;
	}

	return false;
}


std::unique_ptr<GuestTexture> LoadCustomTexture(const uint8_t* data, size_t dataSize, RenderComponentMapping componentMapping)
{
	GuestTexture texture(ResourceType::Texture);

	if (LoadTexture(texture, data, dataSize, componentMapping))
		return std::make_unique<GuestTexture>(std::move(texture));

	return nullptr;
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


	//if (ctx->eax.u32 != 0)
	{
		//auto guestBufferData = (X_D3DResource*)g_memory.Translate(ctx->eax.u32);

		auto buffer = g_userHeap.AllocPhysical<GuestBuffer>(ResourceType::VertexBuffer);
		buffer->buffer = g_device->createBuffer(RenderBufferDesc::VertexBuffer(length, GetBufferHeapType(), RenderBufferFlag::INDEX));
		buffer->dataSize = length;
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


static GuestSurface* CreateSurface(uint32_t width, uint32_t height, X_D3DFORMAT format, bool formatSet = false)
{
	auto ogFormat = formatSet ? (plume::RenderFormat)format : ConvertFormat(format);
	auto isStencil = ogFormat == DepthStencilFormat;
	uint32_t ogWidth, ogHeight;
	ogWidth = width;
	ogHeight = height;

	uint32_t newWidth, newHeight;

	GetUpscaledSurfaceResolution(width, height, &newWidth, &newHeight);

	width = newWidth;
	height = newHeight;

	RenderTextureDesc desc;
	desc.dimension = RenderTextureDimension::TEXTURE_2D;
	desc.width = width;
	desc.height = height;
	desc.depth = 1;
	desc.mipLevels = 1;
	desc.arraySize = 1;
	desc.multisampling.sampleCount = RenderSampleCount::COUNT_1; //todo add as an option(?)
	desc.format = ogFormat;
	desc.flags = desc.format == DepthStencilFormat ? RenderTextureFlag::DEPTH_TARGET : RenderTextureFlag::RENDER_TARGET;

	auto surface = g_userHeap.AllocPhysical<GuestSurface>(desc.format == DepthStencilFormat ?
		ResourceType::DepthStencil : ResourceType::RenderTarget);

	surface->textureHolder = g_device->createTexture(desc);
	surface->texture = surface->textureHolder.get();
	surface->width = width;
	surface->height = height;
	surface->g_width = ogWidth;
	surface->g_height = ogHeight;
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

	const bool isRT = ((desc.flags & RenderTextureFlag::RENDER_TARGET) != 0);

#ifdef _DEBUG 
	surface->texture->setName(fmt::format("{} {:X}", isRT ? "Render Target" : "Depth Stencil", g_memory.MapVirtual(surface)));
#endif

	DiscardInitRenderResource(surface->texture, isRT);

	surface->guestResource.Format = format;
	g_pendingStencilResize = surface;
	return surface;
}

static std::vector<GuestSurface*> g_surfaceCache;
static GuestSurface* GetSurfaceLevel(GuestTexture* texture, int32_t level, X_D3DFORMAT Format)
{
	uint32_t mipWidth = std::max(1u, texture->width >> level);
	uint32_t mipHeight = std::max(1u, texture->height >> level);

	GuestSurface* surface = g_userHeap.AllocPhysical<GuestSurface>(
		texture->format == DepthStencilFormat ?
		ResourceType::DepthStencil : ResourceType::RenderTarget
	);


	surface->texture = texture->texture;
	surface->width = mipWidth;
	surface->height = mipHeight;
	surface->g_width = texture->g_width;
	surface->g_height = texture->g_height;
	surface->format = texture->format;
	surface->guestFormat = Format;
	surface->sampleCount = RenderSampleCount::COUNT_1;

	surface->guestResource.Format = Format;
	surface->layout = texture->layout;

	surface->parentTexture = texture;
	texture->surfaceRTs.push_back(surface);

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
	g_userHeap.Free(gSurface);
	return newSurface;
	//return CreateSurface(pParent->width, pParent->height, (X_D3DFORMAT)pParent->format, true);
}

//CreateDepthStencilSurface
GuestSurface* CreateStandAloneSurface(int32_t Width, int32_t Height, int32_t a3, X_D3DFORMAT Format)
{
	return CreateSurface(Width, Height, Format);
}

bool viewPort_wasFixed = false;
static void FlushViewport(const bool isFixed)
{
	auto& commandList = g_commandLists[g_frame];
	auto hostWidth = g_renderTarget->width;
	auto hostHeight = g_renderTarget->height;
	auto viewport = g_renderviewport.vp;
	auto backupVP = viewport;

	if (isFixed != viewPort_wasFixed)
	{
		g_dirtyStates.viewport = true;
		g_dirtyStates.scissorRect = true;
	}


	viewPort_wasFixed = isFixed;

	float scaleX = (float)g_renderTarget->width / (float)g_renderTarget->g_width;
	float scaleY = (float)g_renderTarget->height / (float)g_renderTarget->g_height;
	float width = (float)std::min(viewport.width, (DWORD)hostWidth - viewport.x);
	float height = (float)std::min(viewport.height, (DWORD)hostHeight - viewport.y);


	width *= scaleX;
	height *= scaleY;
	viewport.x *= scaleX;
	viewport.y *= scaleY;

	// offset is needed for bloom, potentially whole game as well, even though we removed the Xbox 0.5 offset.
	viewport.x += 0.5f;
	viewport.y += 0.5f;

	if (isFixed == false)
	{
		width = (float)hostWidth;
		height = (float)hostHeight;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		viewport.x = 0.5f;
		viewport.y = 0.5f;
	}

	if (g_dirtyStates.viewport)
	{
		if (viewport.minDepth > viewport.maxDepth)
			std::swap(viewport.minDepth, viewport.maxDepth);

		viewport.width = width;
		viewport.height = height;

		commandList->setViewports(viewport);
		g_dirtyStates.viewport = false;
	}

	if (g_dirtyStates.scissorRect)
	{
		RenderRect scissorRect;
		int32_t left = (int32_t)(backupVP.x * scaleX);
		int32_t top = (int32_t)(backupVP.y * scaleY);
		int32_t right = left + (int32_t)(backupVP.width * scaleX);
		int32_t bottom = top + (int32_t)(backupVP.height * scaleY);

		if (isFixed)
		{
			right = (int32_t)hostWidth;
			bottom = (int32_t)hostHeight;
			left = 0;
			top = 0;
		}

		scissorRect = g_scissorTestEnable ? g_scissorRect : RenderRect(
			left,
			top,
			right,
			bottom);

		commandList->setScissors(scissorRect);
		g_dirtyStates.scissorRect = false;
	}
}


void SetResolution(int adapter, unsigned int a2, uint32_t a3)
{

	// Simulate format flags
	uint32_t formatFlag;
	switch (a2 & 3)
	{
	case 0: formatFlag = 30; break;
	case 1: formatFlag = 17; break;
	case 2: formatFlag = 28; break;
	case 3: formatFlag = 18; break;
	}

	Video::ComputeViewportDimensions();

	uint32_t width = uint32_t(round(Video::s_viewportWidth * Config::ResolutionScale));
	uint32_t height = uint32_t(round(Video::s_viewportHeight * Config::ResolutionScale));

	auto base = g_memory.base;

	// this does affect fixed function, such as text position on the screen
	X86_STORE_U32(a3, 640);
	X86_STORE_U32(a3 + (4 * 1), 480);
	X86_STORE_U32(a3 + (4 * 2), 60); //refresh rate in hz
	X86_STORE_U32(a3 + (4 * 3), 0x120);
	X86_STORE_U32(a3 + (4 * 4), formatFlag);
}

int D3D_GetAdapterModeCount(int a1)
{
	printf("STUB D3D_GetAdapterModeCount\n");
	return 4 * 1;
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

int32_t Direct3D_CreateDevice_r(int32_t Adapter, int32_t DeviceType, int32_t hFocusWindow, char Flags, X_D3DPRESENT_PARAMETERS* pPresentationParameters, uint32_t* device)
{
	LOG_UTILITY("CREATE GUEST D3D DEVICE!");

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

	//assign the guest array of render state to a pointer so we can get each render state in real time, this reduces number of hooks needed and bypass the inlined functions.
	InitGuestRenderStatePointer((uint32_t*)(base + g_RenderStateAddr));
	BuildRenderStateMappingTable();

	__imp__sub_3DDAB0(*ctx, base);
	ctx->esp.u32 -= 28; //readjust esp because of calling original function

	return 0; //success
}
static int desCount = 0;


void AddDestructResource(GuestResource* ressource)
{
	RenderCommand cmd;
	cmd.type = RenderCommandType::DestructResource;
	cmd.destructResource.resource = ressource;
	g_renderQueue.enqueue(cmd);
}

static void DestructResource(GuestResource* resource)
{
	AddDestructResource(resource);
	desCount++;
}

static void ProcDestructResource(const RenderCommand& cmd)
{
	desCount = desCount;
	const auto& args = cmd.destructResource;
	g_tempResources[g_frame].push_back(args.resource);
}


void DestructResourceMidAsmHook(X86Register& esi)
{
	if (esi.u32 == 0)
		return;


	auto resource = (GuestResource*)g_memory.Translate(esi.u32);

	if (resource->type == ResourceType::Texture || resource->type == ResourceType::VolumeTexture)
	{
		const auto texture = reinterpret_cast<GuestTexture*>(resource);
		g_pendingTextureResizes.erase(texture);
	}
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
	uint32_t pitch = ComputeTexturePitch(texture);
	uint32_t slicePitch = pitch * texture->height;

	if (texture->mappedMemory == nullptr)
	{
		texture->mappedMemory = g_userHeap.AllocPhysical(slicePitch, 0x10);
	}

	texture->sizeOfTextureRect = slicePitch;
	lockedRect->pitch = pitch;
	lockedRect->bits = g_memory.MapVirtual(texture->mappedMemory);
}

static void UnlockTextureRect(GuestTexture* texture)
{
	if (texture->isRenderTarget == true)
	{
		return;
	}

	RenderCommand cmd;
	cmd.type = RenderCommandType::UnlockTextureRect;
	cmd.unlockTextureRect.texture = texture;
	g_renderQueue.enqueue(cmd);
}

// this seems to be used by FMV

unsigned int GetPixelContainerMipMapLevels(X_D3DPixelContainer* pPixelContainer);
static void CopyDynamicTextureToGPU(GuestTexture* tex)
{
	auto base = g_memory.base;
	auto format = GetXboxPixelContainerFormat((X_D3DFORMAT)tex->guestResource.Format);
	bool isSwizzled = FormatIsSwizzled(format);
	bool bCompressed = XBFormatIsCompressed(format);
	bool bCubemap = tex->guestResource.Format & X_D3DFORMAT_CUBEMAP;

	struct Slice
	{
		uint32_t width;
		uint32_t height;
		uint32_t depth;
		uint32_t srcOffset;
		uint32_t dstOffset;
		uint32_t srcRowPitch;
		uint32_t dstRowPitch;
		uint32_t rowCount;
	};

	std::vector<Slice> slices;
	uint32_t curSrcOffset = 0;
	uint32_t curDstOffset = 0;
	uint32_t baseW = tex->width;
	uint32_t baseH = tex->height;
	uint32_t baseD = tex->depth == 0 ? 1 : tex->depth;

	UINT dwMipMapLevels = 1;
	D3DCUBEMAP_FACES last_face = (bCubemap) ? D3DCUBEMAP_FACE_NEGATIVE_Z : D3DCUBEMAP_FACE_POSITIVE_X;
	DWORD blockSize = 0;

	if (bCompressed)
		blockSize = format == X_D3DFMT_DXT1 ? 8 : 16;

	// Build slice descriptors 
	for (int32_t face = D3DCUBEMAP_FACE_POSITIVE_X; face <= last_face; face++)
	{
		for (uint32_t mipmap_level = 0; mipmap_level < dwMipMapLevels; mipmap_level++)
		{
			auto& slice = slices.emplace_back();
			slice.width = std::max(1u, baseW >> mipmap_level);
			slice.height = std::max(1u, baseH >> mipmap_level);
			slice.depth = std::max(1u, baseD >> mipmap_level);

			if (bCompressed)
			{
				uint32_t blockCols = (slice.width + 3) / 4;
				slice.srcRowPitch = blockCols * blockSize;
				slice.rowCount = (slice.height + 3) / 4;
			}
			else
			{
				uint32_t bpp = XBFormatBitsPerPixel(format);
				uint32_t srcBytesPerRow = (slice.width * bpp) / 8;
				slice.srcRowPitch = srcBytesPerRow;
				slice.rowCount = slice.height;
			}

			uint32_t mip2dSize = slice.srcRowPitch * slice.rowCount;
			uint32_t mipSlicePitch = mip2dSize * slice.depth;
			slice.srcOffset = curSrcOffset;

			uint32_t dstBpp = XBFormatBitsPerPixel(format);
			uint32_t rawDstRowPitch = (slice.width * dstBpp) / 8;
			slice.dstRowPitch = (rawDstRowPitch + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
			slice.dstOffset = curDstOffset;

			curSrcOffset += mipSlicePitch;
			uint32_t alignedSliceSize = (slice.dstRowPitch * slice.rowCount * slice.depth + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);
			curDstOffset += alignedSliceSize;
		}
	}


	// this function should only be used for FMV and nothing else, but just in case, we can have
	// a small heuristic to ensure it's an FMV texture.
	bool isFmv = isSwizzled && tex->depth == 1 && tex->width > 256 && tex->height > 256 && slices.size() == 1; // fmv tex should never use mipmap.


	auto allocation = g_uploadAllocators[g_frame].allocate(curDstOffset, PLACEMENT_ALIGNMENT);
	uint8_t* mappedMemory = reinterpret_cast<uint8_t*>(allocation.memory);

	// Copy/unswizzle data into ring buffer
	for (auto& slice : slices)
	{

		uint8_t* srcData = reinterpret_cast<uint8_t*>(tex->mappedMemory);
		uint8_t* dstData = mappedMemory + slice.dstOffset;


		if (isSwizzled)
		{
			uint32_t dstSlicePitch = slice.dstRowPitch * slice.rowCount;
			uint32_t srcBpp = bCompressed ? blockSize : (XBFormatBitsPerPixel(format) / 8);

			UnswizzleBox(srcData, slice.width, slice.height, slice.depth, srcBpp, dstData, slice.dstRowPitch, dstSlicePitch);
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



	auto copyTextureRegion = [&](Slice& slice, uint32_t subresourceIndex)
		{
			uint32_t uploadRowPitchInTexels = slice.dstRowPitch /
				(bCompressed ? blockSize : RenderFormatSize(tex->format));

			g_commandLists[g_frame]->copyTextureRegion(
				RenderTextureCopyLocation::Subresource(tex->texture,
					subresourceIndex % dwMipMapLevels, subresourceIndex % dwMipMapLevels),
				RenderTextureCopyLocation::PlacedFootprint(allocation.buffer, tex->format,
					slice.width, slice.height, slice.depth, uploadRowPitchInTexels,
					allocation.offset + slice.dstOffset));
		};

	for (size_t i = 0; i < slices.size(); i++)
		copyTextureRegion(slices[i], i);

	if (bCubemap)
	{
		for (size_t i = 1; i < 6; i++)
		{
			for (size_t j = 0; j < slices.size(); j++)
				copyTextureRegion(slices[j], (slices.size() * i) + j);
		}
	}

}


static void ProcUnlockTextureRect(const RenderCommand& cmd)
{
	const auto& args = cmd.unlockTextureRect;

	AddBarrier(args.texture, RenderTextureLayout::COPY_DEST);
	FlushBarriers();

	CopyDynamicTextureToGPU(args.texture);
}

// This function is needed for ADX and FMV decoding, as they rely on the VBlank timing to update their buffers.
// We fake vblank with a hybrid sleep + spin-wait to maintain precise timing across platforms.
void BlockUntilVerticalBlank()
{
	using namespace std::chrono;

	static constexpr auto VBLANK_DURATION = microseconds(16667);

	thread_local steady_clock::time_point lastCall = {};

	auto now = steady_clock::now();

	if (lastCall != steady_clock::time_point{})
	{
		auto target = lastCall + VBLANK_DURATION;
		auto remaining = target - now;
		if (remaining > microseconds(500))
		{
			auto sleepDur = duration_cast<milliseconds>(remaining - 2ms);
			if (sleepDur > 0ms)
				std::this_thread::sleep_for(sleepDur);
			while (steady_clock::now() < target)
				std::this_thread::yield();
		}
	}

	lastCall = steady_clock::now();
}


static void ProcSetViewport(const RenderCommand& cmd)
{
	const auto& args = cmd.setViewport;

	RenderViewport* VP = &g_renderviewport.vp;
	RenderViewport_r* VPR = &g_renderviewport;
	SetDirtyValue<float>(g_dirtyStates.viewport, VP->x, args.x);
	SetDirtyValue<float>(g_dirtyStates.viewport, VP->y, args.y);
	SetDirtyValue<float>(g_dirtyStates.viewport, VP->width, args.width);
	SetDirtyValue<float>(g_dirtyStates.viewport, VP->height, args.height);
	SetDirtyValue<float>(g_dirtyStates.viewport, VP->minDepth, args.minDepth);
	SetDirtyValue<float>(g_dirtyStates.viewport, VP->maxDepth, args.maxDepth);
	SetDirtyValue<bool>(g_dirtyStates.viewport, VPR->isFixed, args.isFixed);


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
	if (pSurface == nullptr)
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
	LOG_ERROR("Unknown Xbox Depth Format");
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

void SetTexture(uint32_t stage, GuestTexture* texture)
{
	RenderCommand cmd;
	cmd.type = RenderCommandType::SetTexture;
	cmd.setTexture.index = stage;
	cmd.setTexture.texture = texture;
	cmd.setTexture.isFixed = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;
	g_texturesPSDef[stage] = texture;
	g_renderQueue.enqueue(cmd);
}

static void SetTextureInRenderThread(uint32_t index, GuestTexture* texture, const bool isFixed = false)
{
	AddBarrier(texture, RenderTextureLayout::SHADER_READ);

	auto viewDimension = texture != nullptr ? texture->viewDimension : RenderTextureViewDimension::UNKNOWN;

	if (texture)
	{
		g_sharedConstants.TextureSampleType[index] = (uint32_t)(viewDimension)-1;
		UpdateHostTextureScaling(texture, isFixed);
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


static void SetViewport(GuestViewport* viewport, GuestSurface* surface)
{
	if (surface != nullptr && viewport != nullptr)
	{
		const bool isFixed = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;

		uint32_t rtWidth = surface->width;
		uint32_t rtHeight = surface->height;

		//for some reason, og xbox can send int_max for width and height, likely to trigger a reset(?)
		float width = (float)std::min(viewport->Width, (DWORD)rtWidth - viewport->X);
		float height = (float)std::min(viewport->Height, (DWORD)rtHeight - viewport->Y);


		RenderCommand cmd;
		cmd.type = RenderCommandType::SetViewport;
		cmd.setViewport.x = viewport->X;
		cmd.setViewport.y = viewport->Y;
		cmd.setViewport.width = width;
		cmd.setViewport.height = height;
		cmd.setViewport.minDepth = viewport->MinZ;
		cmd.setViewport.maxDepth = viewport->MaxZ;
		cmd.setViewport.isFixed = isFixed;
		g_renderQueue.enqueue(cmd);
	}
}


X86_FUNC_IMPL(__imp__sub_3D0040);


//Specify where it should draws (Either framebuffer surface or texture surface)
//on DX8 Xbox, SetDepthStencilSurface is merged with SetRenderTarget.
void D3DDevice_SetRenderTarget(GuestSurface* surface, GuestSurface* pNewZStencil)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	auto device = X86_LOAD_U32(GuestDevicePtrAddr);


	GuestSurface* newSurface = nullptr;
	GuestSurface* newDepthStencil = nullptr;

	if (!g_XboxBackBuffer)
	{
		g_XboxBackBuffer = surface;
		g_XboxDefaultDepthStencilSurface = pNewZStencil;
		g_defaultStencil = CreateSurface(512, 512, X_D3DFMT_LIN_D24S8);
		newSurface = g_backBuffer;
		newDepthStencil = g_defaultStencil;
	}
	else
	{
		if (g_XboxBackBuffer && g_XboxBackBuffer == surface)
		{
			newSurface = g_backBuffer;
		}
		else
		{
			newSurface = surface;
		}

		if (g_XboxDefaultDepthStencilSurface && g_XboxDefaultDepthStencilSurface == pNewZStencil)
		{
			newDepthStencil = g_defaultStencil;
		}
		else
		{
			newDepthStencil = pNewZStencil;
		}
	}


	//printf("SetRenderTarget surface %x texture %x, width %d, height %d\n", newSurface, newSurface->texture, newSurface->width, newSurface->height);
	// The currenct depth stencil is always replaced by whats passed in here (even a null)
	g_ZScale = GetZScaleForPixelContainer(pNewZStencil != nullptr ? (X_D3DPixelContainer*)&pNewZStencil->guestResource : nullptr);

	__imp__sub_3D0040(*ctx, base);
	ctx->esp.u32 -= 12;

	RenderCommand cmd;
	cmd.type = RenderCommandType::SetRenderTarget;
	cmd.setRenderTarget.renderTarget = newSurface;
	g_renderQueue.enqueue(cmd);

	static GuestViewport defaultViewport;
	defaultViewport.X = 0;
	defaultViewport.Y = 0;
	defaultViewport.Width = INT32_MAX;
	defaultViewport.Height = INT32_MAX;
	defaultViewport.MinZ = 0;
	defaultViewport.MaxZ = 1;

	SetViewport(&defaultViewport, newSurface);

	RenderCommand cmd2;
	cmd2.type = RenderCommandType::SetDepthStencilSurface;
	cmd2.setDepthStencilSurface.depthStencil = newDepthStencil;
	g_renderQueue.enqueue(cmd2);

	//SetDefaultViewport((GuestDevice*)g_memory.Translate(device), newDepthStencil);
	g_next_RenderTarget = newSurface;

	//X86_STORE_U32(device + 0x954, surface->width);
	//X86_STORE_U32(device + 0x958, surface->height);
}


static void ProcSetRenderTarget(const RenderCommand& cmd)
{
	const auto& args = cmd.setRenderTarget;

	if (g_renderTarget != args.renderTarget) //due to extensive ping pong with re use of previous RTs, we need to force shader_read to avoid race issues.
	{
		if (g_renderTarget->layout == RenderTextureLayout::COLOR_WRITE)
		{
			AddBarrier(g_renderTarget, RenderTextureLayout::SHADER_READ);
		}
	}

	SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_renderTarget, args.renderTarget);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.renderTargetFormat, args.renderTarget != nullptr ? args.renderTarget->format : RenderFormat::UNKNOWN);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.sampleCount, args.renderTarget != nullptr ? args.renderTarget->sampleCount : RenderSampleCount::COUNT_1);


	// TODO: We can probably scrap this if dropping MSAA
	// Original line: When alpha to coverage is enabled, update the alpha test mode as it's dependent on sample count.
	// We disabled this for Shadow - specConstants never has SPEC_CONSTANT_ALPHA_TEST set (line 1372 is commented out),
	// so this always calls SetAlphaTestMode(false), silently disabling alpha test mid-frame. This broke Elevator transparency at certain camera angles
	// SetAlphaTestMode((g_pipelineState.specConstants & (SPEC_CONSTANT_ALPHA_TEST | SPEC_CONSTANT_ALPHA_TO_COVERAGE)) != 0);
}

static void ProcSetDepthStencilSurface(const RenderCommand& cmd)
{
	const auto& args = cmd.setDepthStencilSurface;

	SetDirtyValue(g_dirtyStates.renderTargetAndDepthStencil, g_depthStencil, args.depthStencil);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.depthStencilFormat, args.depthStencil != nullptr ? args.depthStencil->format : RenderFormat::UNKNOWN);
}

X86_FUNC_IMPL(__imp__sub_3D0680);
void D3DDevice_SetViewport(GuestViewport* viewport)
{

	//call original func first
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	__imp__sub_3D0680(*ctx, base);
	ctx->esp.u32 -= 8;

	if (viewport == nullptr) //they can call this with a null viewport for some reason
	{
		return;
	}

	SetViewport(viewport, g_next_RenderTarget);
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
		//printf("SetFramebuffer %x %x\n", renderTarget, depthStencil);
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
	{
		SetFramebuffer(g_renderTarget, g_depthStencil, true);
	}

	auto& commandList = g_commandLists[g_frame];

	if (g_renderTarget != nullptr && (args.flags & X_D3DCLEAR_TARGET) != 0)
	{
		if (!canClearInOnePass) {
			SetFramebuffer(g_renderTarget, nullptr, true);
		}

		commandList->clearColor(0, RenderColor(args.color[0], args.color[1], args.color[2], args.color[3]));
	}

	const bool clearDepth = (args.flags & X_D3DCLEAR_ZBUFFER) != 0;
	const bool clearStencil = (args.flags & X_D3DCLEAR_STENCIL) != 0;
	if (g_depthStencil != nullptr && (clearDepth || clearStencil))
	{
		if (!canClearInOnePass) {
			SetFramebuffer(nullptr, g_depthStencil, true);
		}

		commandList->clearDepthStencil(clearDepth, clearStencil, args.z, args.stencil);
	}
}


//Clears one or more surfaces such as a render target, multiple render targets, a stencil buffer, and a depth buffer.
void D3DDevice_Clear(uint32_t count, _D3DRECT* pRects, uint32_t flags, uint32_t Color, float Z, uint32_t Stencil)
{
	RenderCommand cmd;
	cmd.type = RenderCommandType::Clear;
	cmd.clear.flags = flags;
	cmd.clear.isFixed = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;

	//extract the RGBA components to convert the color to float
	float color[4] = { ((Color >> 16) & 0xFF) / 255.f,
				  ((Color >> 8) & 0xFF) / 255.f, (Color & 0xFF) / 255.f,
				  ((Color >> 24) & 0xFF) / 255.f };

	cmd.clear.color[0] = color[0];
	cmd.clear.color[1] = color[1];
	cmd.clear.color[2] = color[2];
	cmd.clear.color[3] = color[3];


	cmd.clear.z = float(Z);
	cmd.clear.stencil = Stencil;

	const bool clearDepth = (flags & X_D3DCLEAR_ZBUFFER) != 0;
	const bool clearStencil = (flags & X_D3DCLEAR_STENCIL) != 0;


	g_renderQueue.enqueue(cmd);
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

void SetFixedFunctionDefaultVertexAttributes(size_t Handle)
{

	DWORD vshFlags = GetXboxVertexShaderForHandle(Handle)->Flags;

	auto decl = XSR_HostGetVertexDeclaration(Handle, true); //HostGetVertexDeclaration(Handle, true);
	for (int i = 0; i < X_D3DVSDE_TEXCOORD3; i++)
	{
		if (decl->vRegisterInDeclaration[i])
		{
			continue; // only reset missing attributes
		}

		const float white[4] = { 1, 1, 1, 1 };
		const float black[4] = { 0, 0, 0, 0 };
		const float unset[4] = { 0, 0, 0, 1 };
		const float* value = unset;

		// Account for flags that override this reset behaviour
		if (i == X_D3DVSDE_DIFFUSE && !(vshFlags & X_VERTEXSHADER_FLAG_HASDIFFUSE) ||
			i == X_D3DVSDE_BACKDIFFUSE && !(vshFlags & X_VERTEXSHADER_FLAG_HASBACKDIFFUSE)) {
			value = white;
		}
		else if (i == X_D3DVSDE_SPECULAR && !(vshFlags & X_VERTEXSHADER_FLAG_HASSPECULAR) ||
			i == X_D3DVSDE_BACKSPECULAR && !(vshFlags & X_VERTEXSHADER_FLAG_HASBACKSPECULAR)) {
			value = black;
		}

		SetVertexAttribute(i, value[0], value[1], value[2], value[3]);
	}
}


int D3DDevice_Swap(int32_t a1)
{
	Video::Present();
	return 0;
}

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
				//UpdateHostTextureScaling(args.texture, g_pipelineState.vertexShaderType == VertexShaderMode::FixedFunction);
				SetSurface(args.index, args.texture->sourceSurface);

				shouldSetTexture = false;
			}
		}
	}

	if (shouldSetTexture)
		SetTextureInRenderThread(args.index, args.texture, args.isFixed);

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

			if (g_vulkan)
			{
				auto compressedSpirvData = g_shaderCache.get() + guestShader->shaderCacheEntry->spirvOffset;

				std::vector<uint8_t> decoded(smolv::GetDecodedBufferSize(compressedSpirvData, guestShader->shaderCacheEntry->spirvSize));
				bool result = smolv::Decode(compressedSpirvData, guestShader->shaderCacheEntry->spirvSize, decoded.data(), decoded.size());
				assert(result);

				guestShader->shader = g_device->createShader(decoded.data(), decoded.size(), "main", RenderShaderFormat::SPIRV);
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


	desc.pixelShader = pipelineState.isFixedFunctionPS ? FixedFuncPS.get() : GetOrLinkShader(pipelineState.pixelShader, pipelineState.specConstants);//pipelineState.pixelShader != nullptr ? GetOrLinkShader(pipelineState.pixelShader, pipelineState.specConstants) : nullptr;
	desc.depthFunction = pipelineState.zFunc;
	desc.depthEnabled = pipelineState.zEnable; //Xbox likes to enable ZEnable without a depth stencil
	desc.depthWriteEnabled = pipelineState.zWriteEnable;
	desc.depthBias = pipelineState.depthBias;
	desc.stencilEnabled = pipelineState.stencilEnable;
	desc.stencilReadMask = pipelineState.stencilMask;
	desc.stencilWriteMask = pipelineState.stencilWriteMask;
	desc.stencilReference = pipelineState.stencilRef;
	desc.stencilFrontFace.compareFunction = pipelineState.stencilFunc;
	desc.stencilFrontFace.failOp = pipelineState.stencilFail;
	desc.stencilFrontFace.depthFailOp = pipelineState.stencilZFail;
	desc.stencilFrontFace.passOp = pipelineState.stencilPass;
	desc.stencilBackFace = desc.stencilFrontFace;
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
	return (xbox::addr_xt*)pData;
}

inline int GetXboxPixelContainerDimensionCount(const X_D3DPixelContainer* pXboxPixelContainer)
{
	// Don't pass in unassigned Xbox pixel container
	assert(pXboxPixelContainer != xbox::zeroptr);

	return (X_D3DFORMAT)((pXboxPixelContainer->Format & X_D3DFORMAT_DIMENSION_MASK) >> X_D3DFORMAT_DIMENSION_SHIFT);
}

X_D3DRESOURCETYPE GetXboxTextureType(const X_D3DResource* pXboxResource)
{
	DWORD Format = ((X_D3DPixelContainer*)pXboxResource)->Format;

	if (Format & X_D3DFORMAT_CUBEMAP)
		return X_D3DRTYPE_CUBETEXTURE;

	if (GetXboxPixelContainerDimensionCount((X_D3DPixelContainer*)pXboxResource) > 2)
		return X_D3DRTYPE_VOLUMETEXTURE;

	return X_D3DRTYPE_TEXTURE;
}

void UpdateUserConfigConstant(bool toggle)
{
	// upload user resolution to add HD resolution 
	float userConfig[4];

	userConfig[0] = toggle ? (float)Video::s_viewportWidth : 0.f;
	userConfig[1] = toggle ? (float)Video::s_viewportHeight : 0.f;
	userConfig[2] = toggle ? 1.f : 0.f;
	userConfig[3] = 0.f;
	CopyVertexShaderConstant(HOST_USER_CONFIG, (DWORD*)userConfig, 1);
}

static void FlushRenderStateForMainThread(LocalRenderCommandQueue& queue)
{
	UpdateAllRenderState();

	if (g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction)
	{
		UpdateFixedFunctionVertexShaderState((size_t)g_memory.Translate(g_VertexShader_Handle), g_pipelineState.isFixedFunctionPS);
	}
	else
	{
		UpdateHostViewPortOffsetAndScaleConstants();
	}

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
	}


	XXH64_hash_t hash = XXH3_64bits(&vsh_constants[0][0], sizeof(vsh_constants));
	bool isDirty = g_lastVertexShaderConstants != hash;

	if (isDirty)
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
				cmd.setVertexShaderConstants.isFixedFunc = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;

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
				g_intermediaryUploadAllocator.allocate(&vsh_constants[batchStartIndex][0], count * sizeof(float) * 4);
			cmd.setVertexShaderConstants.index = batchStartIndex;
			cmd.setVertexShaderConstants.size = count;
			cmd.setVertexShaderConstants.isFixedFunc = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;
		}

		g_lastVertexShaderConstants = hash;
	}

	//SetPixelShaderConstants
	hash = XXH3_64bits(&psh_constants[0][0], sizeof(psh_constants));
	isDirty = g_lastPixelShaderConstants != hash;

	if (isDirty)
	{
		auto& cmd = queue.enqueue();
		cmd.type = RenderCommandType::SetPixelShaderConstants;
		cmd.setPixelShaderConstants.memory =
			g_intermediaryUploadAllocator.allocate(&psh_constants[0][0], PSH_XBOX_CONSTANT_MAX * sizeof(float) * 4);
		cmd.setPixelShaderConstants.index = 0;
		cmd.setPixelShaderConstants.size = PSH_XBOX_CONSTANT_MAX;
		g_lastPixelShaderConstants = hash;
	}
}

static bool isFixedForRenderThread = false;
static constexpr int32_t COMMON_DEPTH_BIAS_VALUE = int32_t((1 << 24) * 0.002f);
static constexpr float COMMON_SLOPE_SCALED_DEPTH_BIAS_VALUE = 1.0f;
static void FlushRenderStateForRenderThread()
{
	const bool isFixed = isFixedForRenderThread;
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

	if (renderTarget->wasResized)
	{
		printf("");
	}

	AddBarrier(renderTarget, RenderTextureLayout::COLOR_WRITE);
	AddBarrier(depthStencil, RenderTextureLayout::DEPTH_WRITE);

	FlushBarriers();

	SetFramebuffer(renderTarget, depthStencil, false);
	FlushViewport(isFixed);


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
		if (isFixed)
		{
			auto vertexShaderConstants = g_uploadAllocators[g_frame].allocate(g_vertexShaderConstantsFixed, sizeof(g_vertexShaderConstantsFixed), 0x100);
			SetRootDescriptor(vertexShaderConstants, 0);
		}
		else
		{

			auto vertexShaderConstants = g_uploadAllocators[g_frame].allocate(g_vertexShaderConstants, sizeof(g_vertexShaderConstants), 0x100);
			SetRootDescriptor(vertexShaderConstants, 0);
		}


	}

	if (g_dirtyStates.pixelShaderConstants)
	{
		auto pixelShaderConstants = g_uploadAllocators[g_frame].allocate(g_pixelShaderConstants, sizeof(g_pixelShaderConstants), 0x100);
		SetRootDescriptor(pixelShaderConstants, 1);
	}


	if (g_dirtyStates.sharedConstants)
	{
		auto sharedConstants = g_uploadAllocators[g_frame].allocate(&g_sharedConstants, sizeof(g_sharedConstants), 0x100);
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

//D3D12 requires vertex buffer strides to be aligned to 4 bytes unlike Xbox, so we need to fix that when needed
uint32_t AlignStride(uint32_t stride, uint32_t alignment = 4) {
	return (stride + alignment - 1) & ~(alignment - 1);
}


static inline FLOAT PackedIntToFloat(const int32_t value, const FLOAT PosFactor, const FLOAT NegFactor)
{
	if (value >= 0) {
		return ((FLOAT)value) / PosFactor;
	}
	else {
		return ((FLOAT)value) / NegFactor;
	}
}

//handle vertex format difference such as normalized short format
uint8_t* ApplyVertexPatch(uint8_t* vertexData, const uint8_t indexStream, const uint32_t VertexCount, const uint32_t originalStride, uint32_t alignedStride, bool useHostMemory = false)
{
	size_t host_handle = (size_t)g_memory.Translate(g_VertexShader_Handle);
	auto vertexElement = XSR_HostGetVertexDeclaration(host_handle, g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction);

	auto pVertexShaderStreamInfo = &(vertexElement->VertexStreams[indexStream]);
	UINT XboxStreamNumber = pVertexShaderStreamInfo->XboxStreamIndex;
	bool bNeedVertexPatching = (pVertexShaderStreamInfo != nullptr && pVertexShaderStreamInfo->NeedPatch);

	if (!bNeedVertexPatching)
		return nullptr;

	uint8_t* pHostVertexData = useHostMemory ? g_intermediaryUploadAllocator.allocate(VertexCount * alignedStride) : (uint8_t*)g_userHeap.AllocPhysical(VertexCount * alignedStride, 0x10);

	for (uint32_t uiVertex = 0; uiVertex < VertexCount; uiVertex++)
	{
		uint8_t* pXboxVertexAsByte = &vertexData[uiVertex * originalStride];
		uint8_t* pHostVertexAsByte = &pHostVertexData[uiVertex * alignedStride];

		for (UINT uiElement = 0; uiElement < pVertexShaderStreamInfo->NumberOfVertexElements; uiElement++)
		{
			FLOAT* pXboxVertexAsFloat = (FLOAT*)pXboxVertexAsByte;
			SHORT* pXboxVertexAsShort = (SHORT*)pXboxVertexAsByte;
			const int32_t XboxElementByteSize = pVertexShaderStreamInfo->VertexElements[uiElement].XboxByteSize;
			FLOAT* pHostVertexAsFloat = (FLOAT*)pHostVertexAsByte;
			SHORT* pHostVertexAsShort = (SHORT*)pHostVertexAsByte;
			// Dxbx note : The following code handles only the D3DVSDT enums that need conversion;
			// All other cases are catched by the memcpy in the default-block.

			switch (pVertexShaderStreamInfo->VertexElements[uiElement].XboxType)
			{
			case X_D3DVSDT_NORMSHORT1: { // 0x11:
				pHostVertexAsShort[0] = pXboxVertexAsShort[0];
				pHostVertexAsShort[1] = 0;
				break;
			}
			case X_D3DVSDT_NORMSHORT2: { // 0x21:
				pHostVertexAsShort[0] = pXboxVertexAsShort[0];
				pHostVertexAsShort[1] = pXboxVertexAsShort[1];
				break;
			}
			case X_D3DVSDT_NORMSHORT3: { // 0x31:
				pHostVertexAsShort[0] = pXboxVertexAsShort[0];
				pHostVertexAsShort[1] = pXboxVertexAsShort[1];
				pHostVertexAsShort[2] = pXboxVertexAsShort[2];
				pHostVertexAsShort[3] = 32767; // TODO : verify
				break;
			}
			case X_D3DVSDT_NORMSHORT4: { // 0x41:
				pHostVertexAsShort[0] = pXboxVertexAsShort[0];
				pHostVertexAsShort[1] = pXboxVertexAsShort[1];
				pHostVertexAsShort[2] = pXboxVertexAsShort[2];
				pHostVertexAsShort[3] = pXboxVertexAsShort[3];
				break;
			}
			case X_D3DVSDT_NORMPACKED3:
			{

				// Make it FLOAT3
				union {
					int32_t value;
					struct {
						int32_t x : 11;
						int32_t y : 11;
						int32_t z : 10;
					};
				} NormPacked3;

				NormPacked3.value = ((int32_t*)pXboxVertexAsByte)[0];

				pHostVertexAsFloat[0] = PackedIntToFloat(NormPacked3.x, 1023.0f, 1024.f);
				pHostVertexAsFloat[1] = PackedIntToFloat(NormPacked3.y, 1023.0f, 1024.f);
				pHostVertexAsFloat[2] = PackedIntToFloat(NormPacked3.z, 511.0f, 512.f);
				break;
			}
			case X_D3DVSDT_SHORT1: { // 0x15:
				// Make it SHORT2 and set the second short to 0
				pHostVertexAsShort[0] = pXboxVertexAsShort[0];
				pHostVertexAsShort[1] = 0;
				break;
			}
			case X_D3DVSDT_SHORT3: { // 0x35:
				// Make it a SHORT4 and set the fourth short to 1
				pHostVertexAsShort[0] = pXboxVertexAsShort[0];
				pHostVertexAsShort[1] = pXboxVertexAsShort[1];
				pHostVertexAsShort[2] = pXboxVertexAsShort[2];
				pHostVertexAsShort[3] = 1; // Turok verified (character disappears when this is 32767)
				break;
			}
			case X_D3DVSDT_PBYTE1: { // 0x14:
				// Make it UBYTE4N
				pHostVertexAsByte[0] = pXboxVertexAsByte[0];
				pHostVertexAsByte[1] = 0;
				pHostVertexAsByte[2] = 0;
				pHostVertexAsByte[3] = 255; // TODO : Verify
				break;
			}
			case X_D3DVSDT_PBYTE2: { // 0x24:
				pHostVertexAsByte[0] = pXboxVertexAsByte[0];
				pHostVertexAsByte[1] = pXboxVertexAsByte[1];
				pHostVertexAsByte[2] = 0;
				pHostVertexAsByte[3] = 255; // TODO : Verify
				break;
			}
			case X_D3DVSDT_PBYTE3: { // 0x34:
				// Make it UBYTE4N
				pHostVertexAsByte[0] = pXboxVertexAsByte[0];
				pHostVertexAsByte[1] = pXboxVertexAsByte[1];
				pHostVertexAsByte[2] = pXboxVertexAsByte[2];
				pHostVertexAsByte[3] = 255; // TODO : Verify
				break;
			}
			case X_D3DVSDT_PBYTE4: { // 0x44:
				pHostVertexAsByte[0] = pXboxVertexAsByte[0];
				pHostVertexAsByte[1] = pXboxVertexAsByte[1];
				pHostVertexAsByte[2] = pXboxVertexAsByte[2];
				pHostVertexAsByte[3] = pXboxVertexAsByte[3];
				break;
			}
			case X_D3DVSDT_FLOAT2H: { // 0x72:
				// Make it FLOAT4 and set the third float to 0.0
				pHostVertexAsFloat[0] = pXboxVertexAsFloat[0];
				pHostVertexAsFloat[1] = pXboxVertexAsFloat[1];
				pHostVertexAsFloat[2] = 0.0f;
				pHostVertexAsFloat[3] = pXboxVertexAsFloat[2];
				break;
			}
			case X_D3DVSDT_NONE: { // 0x02:

				LOG_WARNING("X_D3DVSDT_NONE\n");
				// No host element data (but Xbox size can be above zero, when used for X_D3DVSD_MASK_SKIP*
				break;
			}
			default: {
				// Generic 'conversion' - just make a copy :
				memcpy(pHostVertexAsByte, pXboxVertexAsByte, XboxElementByteSize);
				break;
			}
			} // switch

			// Increment the Xbox pointer :
			pXboxVertexAsByte += XboxElementByteSize;
			// Increment the host pointer :
			pHostVertexAsByte += pVertexShaderStreamInfo->VertexElements[uiElement].HostByteSize;
		} // for NumberOfVertexElements
	} // for uiVertexCount

	return pHostVertexData;
}


// D3D_DrawVertices originally
// this seems to be used only for particles with a dynamic vertex buffer
// in that case we can redirect it to DrawPrimitiveUP to use our memory allocator
bool reUsedVertex = false;
void DrawPrimitive(X_D3DPRIMITIVETYPE PrimitiveType, uint32_t StartVertex, uint32_t VertexCount)
{

	Guard_Rendering();

	LocalRenderCommandQueue queue;

	FlushRenderStateForMainThread(queue);


	for (int i = 0; i < 16; i++)
	{
		if (g_Xbox_SetStreamSource[i].VertexBuffer)
		{
			auto alignedStride = AlignStride(g_Xbox_SetStreamSource[i].Stride);
			size_t host_handle = (size_t)g_memory.Translate(g_VertexShader_Handle);
			auto vertexElement = XSR_HostGetVertexDeclaration(host_handle, g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction);

			auto pVertexShaderStreamInfo = &(vertexElement->VertexStreams[i]);
			bool bNeedVertexPatching = (pVertexShaderStreamInfo != nullptr && pVertexShaderStreamInfo->NeedPatch);

			//if patching is needed the stride may be bigger so update it from the Vertex Declaration that we got earlier
			alignedStride = pVertexShaderStreamInfo->HostVertexStride;

			unsigned long dataAddr = (unsigned long)GetDataFromXboxResource((X_D3DResource*)(g_Xbox_SetStreamSource[i].VertexBuffer));
			uint8_t* pXboxVertexData = reinterpret_cast<uint8_t*>(g_memory.Translate(dataAddr));

			XXH64_hash_t hash = XXH3_64bits(pXboxVertexData, g_Xbox_SetStreamSource[i].Stride);

			uint8_t* vertexParticles = nullptr;

			// particles allocate memory every frame and we never pass our VB to the guest memory
			// thus, we can use our dynamic allocator with host memory for better use without worrying about guest physical memory cap.

			// so far testing shows that patching is always needed
			if (bNeedVertexPatching)
			{
				const uint32_t originalStride = g_Xbox_SetStreamSource[i].Stride;
				vertexParticles = ApplyVertexPatch(pXboxVertexData, i, VertexCount, originalStride, alignedStride, true);
			}
			else
			{
				const size_t originalSize = VertexCount * alignedStride;
				vertexParticles = (uint8_t*)g_intermediaryUploadAllocator.allocate(VertexCount * alignedStride);
				memcpy(vertexParticles, pXboxVertexData, originalSize);
			}


			if (vertexParticles)
			{
				auto& cmd = queue.enqueue();
				cmd.type = RenderCommandType::DrawPrimitiveUP;
				cmd.drawPrimitiveUP.primitiveType = PrimitiveType;
				cmd.drawPrimitiveUP.vertexStreamZeroData = vertexParticles;
				cmd.drawPrimitiveUP.vertexStreamZeroSize = VertexCount * alignedStride;
				cmd.drawPrimitiveUP.vertexStreamZeroStride = alignedStride;
				cmd.drawPrimitiveUP.primitiveCount = VertexCount;
				cmd.drawPrimitiveUP.isFixed = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;

				queue.submit();
			}

		}
	}
}

static void ProcDrawPrimitive(const RenderCommand& cmd)
{
	const auto& args = cmd.drawPrimitive;

	//convert primitive
	SetPrimitiveType(args.primitiveType);
	isFixedForRenderThread = args.isFixed;
	FlushRenderStateForRenderThread();

	uint32_t primitiveCount = args.primitiveCount;
	auto& commandList = g_commandLists[g_frame];
	commandList->drawInstanced(primitiveCount, 1, args.startVertex, 0);
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

GuestBuffer* GetGuestVertexBufferIndexed(int i, uint32_t minIdx, uint32_t maxIdx, uint32_t alignedStride)
{
	uint32_t usedVertices = maxIdx - minIdx + 1;
	const uint32_t originalStride = g_Xbox_SetStreamSource[i].Stride;

	unsigned long dataAddr = (unsigned long)GetDataFromXboxResource((X_D3DResource*)(g_Xbox_SetStreamSource[i].VertexBuffer));
	uint8_t* pXboxVertexData = reinterpret_cast<uint8_t*>(g_memory.Translate(dataAddr));

	pXboxVertexData += (minIdx * originalStride); //we only want to hash and copy the used vertices, the unused can be skipped.

	const size_t originalSize = usedVertices * originalStride;
	XXH64_hash_t hash = XXH3_64bits(pXboxVertexData, originalSize);


	auto& bufferCache = g_indexedVertexBufferCache[hash];
	if (bufferCache == nullptr)
	{
		uint8_t* HostVertexData = ApplyVertexPatch(pXboxVertexData, i, usedVertices, originalStride, alignedStride);
		const size_t size = usedVertices * alignedStride;
		bufferCache = CreateVertexBuffer(size);

		bufferCache->hash = hash;
		if (HostVertexData == nullptr) //if patch didn't happen simply copy the vertex as they are
		{
			bufferCache->mappedMemory = g_userHeap.AllocPhysical(bufferCache->dataSize, 0x10);
			memcpy(bufferCache->mappedMemory, pXboxVertexData, originalSize);
		}
		else
		{
			bufferCache->mappedMemory = HostVertexData; //we already have the vertex copied from the patch earlier so just assign them
		}


		auto& vbDerived = VBsList[g_Xbox_SetStreamSource[i].VertexBuffer];
		DerivedVertexBuffer newDerivedVB = { bufferCache, hash };
		vbDerived.push_back(newDerivedVB);
		UnlockVertexBuffer(bufferCache); //send our VB to the GPU
	}

	return bufferCache;
}


//D3DDevice_DrawIndexedVertices (seems to be used to display message)
//On Xbox, VertexCount = IndexCount, both are the same
void DrawIndexedPrimitive(X_D3DPRIMITIVETYPE PrimitiveType, uint32_t VertexCount, int16_t* pIndexData)
{
#ifdef _DEBUG //this is a breakpoint spot for debugging the light issue with tails muzzle in circus park.
	if (VertexCount == 45)
	{
		printf("");
	}
	if (VertexCount == 417)
	{
		printf("");
	}
#endif


	Guard_Rendering();

	//shadow = 2192 vertex = stride 24 (32 with conversion)
	//object (shadow eyes) = 91 vertex =  stride 24 (32 with conversion)

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


	SetIndices(bufferIndex);

	uint32_t minIdx = 0, maxIdx = 0;
	ComputeIndexRange(pIndexData, 0, VertexCount, minIdx, maxIdx);

	bool patched = false;

	bool drawShadow = false;

	for (int i = 0; i < 16; i++)
	{
		if (VertexCount == 91)
		{
			printf("");
		}

		if (g_Xbox_SetStreamSource[i].VertexBuffer == nullptr)
			continue;

		auto alignedStride = AlignStride(g_Xbox_SetStreamSource[i].Stride);

		size_t host_handle = (size_t)g_memory.Translate(g_VertexShader_Handle);
		auto vertexElement = XSR_HostGetVertexDeclaration(host_handle, g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction);

		auto pVertexShaderStreamInfo = &(vertexElement->VertexStreams[i]);
		bool bNeedVertexPatching = (pVertexShaderStreamInfo != nullptr && pVertexShaderStreamInfo->NeedPatch);
		patched = bNeedVertexPatching;
		//if patching is needed the stride may be bigger so update it from the Vertex Declaration that we got earlier
		if (bNeedVertexPatching)
			alignedStride = pVertexShaderStreamInfo->HostVertexStride;

		auto bufferCache = GetGuestVertexBufferIndexed(i, minIdx, maxIdx, alignedStride);

		if (!bufferCache)
			return;


		RenderCommand cmd;
		cmd.type = RenderCommandType::SetStreamSource;
		cmd.setStreamSource.StreamNumber = i;
		cmd.setStreamSource.vertexBuffer = bufferCache;
		cmd.setStreamSource.stride = alignedStride;

		g_renderQueue.enqueue(cmd);
	}


	LocalRenderCommandQueue queue;
	FlushRenderStateForMainThread(queue);

	auto& cmd = queue.enqueue();
	cmd.type = RenderCommandType::DrawIndexedPrimitive;
	cmd.drawIndexedPrimitive.primitiveType = PrimitiveType;
	cmd.drawIndexedPrimitive.baseVertexIndex = -(int32_t)minIdx;
	cmd.drawIndexedPrimitive.startIndex = 0;
	cmd.drawIndexedPrimitive.primCount = VertexCount;
	cmd.drawIndexedPrimitive.isFixed = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;

	queue.submit();
}

static void ProcDrawIndexedPrimitive(const RenderCommand& cmd)
{
	const auto& args = cmd.drawIndexedPrimitive;
	SetPrimitiveType(args.primitiveType);
	isFixedForRenderThread = args.isFixed;
	FlushRenderStateForRenderThread();
	g_commandLists[g_frame]->drawIndexedInstanced(args.primCount, 1, args.startIndex, args.baseVertexIndex, 0);
}
extern bool captureNow;
extern bool drawCompositeRT;


//D3DDevice_DrawVerticesUP - used for 2D Stuff, does not require a vertex buffer (originally)
void DrawPrimitiveUP(X_D3DPRIMITIVETYPE PrimitiveType, uint32_t VertexCount, void* pVertexStreamZeroData, uint32_t VertexStreamZeroStride)
{
	// dirty hack to attempt to fix barrier crash,
	// the crash happens when the main thread is trying to draw while the loading thread is calling present
	// the render thread does not like getting new command queue while processing Present.
	static int failSafeCounter = 0;
	while (aboutToDrawLoading && GuestThread::GetCurrentThreadId() != GetLoadingThreadID())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(16));

		if (++failSafeCounter > 200)
		{
			failSafeCounter = 0;
			break;
		}
	}


	Guard_Rendering();



	LocalRenderCommandQueue queue;
	FlushRenderStateForMainThread(queue);


	UpVertex* vertexData = (UpVertex*)(pVertexStreamZeroData);

	auto& cmd = queue.enqueue();
	cmd.type = RenderCommandType::DrawPrimitiveUP;
	cmd.drawPrimitiveUP.ID = g_drawPrimitiveUP_ID++;
	cmd.drawPrimitiveUP.primitiveType = PrimitiveType;
	cmd.drawPrimitiveUP.primitiveCount = VertexCount;
	cmd.drawPrimitiveUP.vertexStreamZeroData = g_intermediaryUploadAllocator.allocate(vertexData, VertexCount * VertexStreamZeroStride);
	cmd.drawPrimitiveUP.vertexStreamZeroSize = VertexCount * VertexStreamZeroStride;
	cmd.drawPrimitiveUP.vertexStreamZeroStride = VertexStreamZeroStride;
	cmd.drawPrimitiveUP.isFixed = g_Xbox_VertexShaderMode == VertexShaderMode::FixedFunction;
	cmd.drawPrimitiveUP.threadID = GuestThread::GetCurrentThreadId();

	if (drawCompositeRT)
		cmd.drawPrimitiveUP.caller = CompositeRT;
	else if (aboutToDrawLoading)
		cmd.drawPrimitiveUP.caller = LoadingLogo;
	else
		cmd.drawPrimitiveUP.caller = Others;

	queue.submit();
	//	printf("Game called Draw Primitive UP - ID: %d, thread id %d - g_frame=%u\n", g_drawPrimitiveUP_ID, GetCurrentThreadId(), g_frame);

}
extern bool readyToDebug;

static void ProcDrawPrimitiveUP(const RenderCommand& cmd)
{

	// this is needed to avoid random crash if a resize is requested while 
	// rt are doing a composite using DrawPrimitiveUP
	if (g_pendingSurfaceResizeFlag)
	{
		return;
	}

	const auto& args = cmd.drawPrimitiveUP;

	//	printf("RenderThread is now treating DrawPrimitiveUP ID %d - g_frame=%u\n", args.ID, g_frame);
	X_D3DPRIMITIVETYPE XboxPrimitive = (X_D3DPRIMITIVETYPE)args.primitiveType;

	SetPrimitiveType(args.primitiveType);
	SetDirtyValue(g_dirtyStates.pipelineState, g_pipelineState.vertexStrides[0], uint8_t(args.vertexStreamZeroStride));

	auto allocation = g_uploadAllocators[g_frame].allocate(reinterpret_cast<const uint32_t*>(args.vertexStreamZeroData), args.vertexStreamZeroSize, 0x4);

	auto& vertexBufferView = g_vertexBufferViews[0];
	vertexBufferView.size = args.primitiveCount * args.vertexStreamZeroStride;
	vertexBufferView.buffer = allocation.buffer->at(allocation.offset);
	g_inputSlots[0].stride = args.vertexStreamZeroStride;
	g_dirtyStates.vertexStreamFirst = 0;


	uint32_t indexCount = 0;

	if (args.primitiveType == X_D3DPT_QUADLIST)
	{
		indexCount = g_quadIndexData.prepare(args.primitiveCount);
	}
	/**else if (!g_capabilities.triangleFan && args.primitiveType == X_D3DPT_TRIANGLEFAN)
		indexCount = g_triangleFanIndexData.prepare(args.primitiveCount);*/

	isFixedForRenderThread = args.isFixed;
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

	if (Config::TextureFiltering == ETextureFiltering::Linear)
	{
		mipFilter = RenderFilter::LINEAR;
		magFilter = RenderFilter::LINEAR;
		minFilter = RenderFilter::LINEAR;
	}
	else if (Config::TextureFiltering == ETextureFiltering::Nearest)
	{
		mipFilter = RenderFilter::NEAREST;
		magFilter = RenderFilter::NEAREST;
		minFilter = RenderFilter::NEAREST;
	}


	bool anisotropyEnabled = Config::AnisotropicFiltering > 0 && mipFilter == RenderFilter::LINEAR;
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
	SetDirtyValue(dirtySampler, samplerDesc.maxAnisotropy, anisotropyEnabled ? Config::AnisotropicFiltering : 16u); // anisotropyEnabled ? Config::AnisotropicFiltering : 16u);
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

	assert((args.index + args.size) * 4 <= HOST_D3DVS_CONSTREG_FULL * 4);
	if (args.isFixedFunc)
		memcpy(&g_vertexShaderConstantsFixed[args.index * 4], args.memory, args.size * 4 * sizeof(float));
	else
		memcpy(&g_vertexShaderConstants[args.index * 4], args.memory, args.size * 4 * sizeof(float));
	g_dirtyStates.vertexShaderConstants = true;
}


static void ProcSetPixelShaderConstants(const RenderCommand& cmd)
{
	auto& args = cmd.setPixelShaderConstants;

	assert((args.index + args.size) * 4 <= PSH_XBOX_CONSTANT_MAX * 4);

	int sizeToCopy = args.size * 4 * sizeof(float);
	sizeToCopy = sizeToCopy;
	memcpy(&g_pixelShaderConstants[args.index * 4], args.memory, args.size * 4 * sizeof(float));
	g_dirtyStates.pixelShaderConstants = true;
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


//Free Vertex Buffer
X86_FUNC_IMPL(__imp__sub_2E6140);
X86_FUNC(sub_2E6140)
{
	auto resource = X86_LOAD_U32(ctx.esp.u32 + 4);
	if (resource)
	{
		GuestBuffer* resourcePtr = (GuestBuffer*)(base + resource);

		auto it = VBsList.find(resourcePtr);
		if (it != VBsList.end())
		{
			for (auto& vec : it->second)
			{
				g_vertexCache.erase(vec.hash);
				g_indexedVertexBufferCache.erase(vec.hash);
				AddDestructResource(vec.vertexBuffer);
			}

			VBsList.erase(resourcePtr);
		}

		for (auto& temp : g_tempUnreadyResource) //if previous VB wasn't ready to be deleted before, try again now
		{
			AddDestructResource(temp);
		}

		g_tempUnreadyResource.clear();
	}

	__imp__sub_2E6140(ctx, base);
}


void CopyVertexShaderConstant(int32_t Register, DWORD* pConstantData, const uint32_t count)
{
	float* float_vsh_constants = (float*)&(vsh_constants[Register][0]);
	memcpy(float_vsh_constants, pConstantData, count * sizeof(float) * 4);

	for (DWORD i = 0; i < count; i++)
		vsh_constants_dirty[Register + i] = true;
}

void LogFloat4(const char* name, const float* values, int count = 1) {
	for (int i = 0; i < count; i++) {
		printf("%s[%d]: (%f, %f, %f, %f)\n",
			name, i,
			values[i * 4 + 0], values[i * 4 + 1],
			values[i * 4 + 2], values[i * 4 + 3]);
	}
}

// Function to log packed flags (treating as uint32 for bit inspection)
void LogPackedFlags(const char* name, const float* values, int count = 4) {
#ifdef _DEBUG
	printf("%s (as float):\n", name);
	LogFloat4(name, values, count);

	printf("%s (as uint32 bits):\n", name);
	for (int i = 0; i < count * 4; i++) {
		int intValue = *(int*)&values[i];
		printf("  [%d]: 0x%08X\n", i, intValue);
	}
#endif
}


void D3DDevice_SetVertexShaderConstant1Fast(int32_t Register, DWORD* pConstantData)
{
	float* constants = (float*)pConstantData;
	CopyVertexShaderConstant(Register, pConstantData, 1);
}


void D3DDevice_SetVertexShaderConstantNotInlineFast(int32_t Register, DWORD* pConstantData, uint32_t ConstantCount)
{
	float* constants = (float*)pConstantData;
	CopyVertexShaderConstant(Register, pConstantData, ConstantCount / 4);
}

//D3DDevice_LightEnable
X86_FUNC_IMPL(__imp__sub_3D0B70);
X86_FUNC(sub_3D0B70)
{
	uint32_t Index = GetHostParameter(1);
	uint32_t bEnable = GetHostParameter(2);
	__imp__sub_3D0B70(ctx, base);
	LightEnable(Index, bEnable);
}

//D3DDevice_SetLight
X86_FUNC_IMPL(__imp__sub_3D08B0);
X86_FUNC(sub_3D08B0)
{
	uint32_t Index = GetHostParameter(1);
	X_D3DLIGHT8* pLight = (X_D3DLIGHT8*)GetHostParameter(2, CallConvention::Stdcall, ParameterType::Pointer);
	__imp__sub_3D08B0(ctx, base);
	UpdateLightData(Index, pLight);
}

void SetMaterial(Material* pMaterial)
{
	UpdateMaterial(pMaterial);
}



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

//rwD3D8RasterLockPalette
X86_FUNC_IMPL(__imp__sub_2DF9F0);
X86_FUNC(sub_2DF9F0)
{
	__imp__sub_2DF9F0(ctx, base);
	if (ctx.eax.u32 == 1)
	{
		auto paletteData = X86_LOAD_U32(ctx.ecx.u32);
		g_currentPaletteData = (uint32_t*)(base + paletteData);
	}
}

void CopyTextureToGPU(GuestTexture* tex)
{
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
	{
		blockSize = format == X_D3DFMT_DXT1 ? 8 : 16;
	}

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

			if (bCompressed)
			{
				uint32_t blockCols = (slice.width + 3) / 4;
				rawDstRowPitch = blockCols * blockSize;
			}

			slice.dstRowPitch = (rawDstRowPitch + PITCH_ALIGNMENT - 1) & ~(PITCH_ALIGNMENT - 1);
			slice.dstOffset = curDstOffset;

			uint32_t alignedSliceSize = (slice.dstRowPitch * slice.rowCount * slice.depth + PLACEMENT_ALIGNMENT - 1) & ~(PLACEMENT_ALIGNMENT - 1);

			curSrcOffset += mipSlicePitch;
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
				dstData, slice.dstRowPitch, slice.dstRowPitch * slice.rowCount, slice.depth, (void*)g_currentPaletteData);
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
					uint32_t mipIndex = subresourceIndex % dwMipMapLevels;
					uint32_t arrayIndex = subresourceIndex / dwMipMapLevels;
					uint32_t uploadRowPitchInTexels = slice.dstRowPitch / RenderFormatSize(tex->format);

					g_copyCommandList->copyTextureRegion(
						RenderTextureCopyLocation::Subresource(tex->texture, mipIndex, arrayIndex),
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

// rwD3D8NativeTextureRead
X86_FUNC_IMPL(__imp__sub_2ED0D0);
X86_FUNC(sub_2ED0D0)
{
	// call original function first
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


GuestTexture* g_currentTextureRect = nullptr;
// rwD3D8RasterUnlock
X86_FUNC_IMPL(__imp__sub_2DF870);
X86_FUNC(sub_2DF870)
{
	// Texture data aren't stored in the raster, instead they store the image in another raster using the extension offset to reach it
	auto rasterAddr = X86_LOAD_U32(ctx.esp.u32 + 8); // get raster param addr 
	auto RasterExtOffsetAddr = (uint32_t)(X86_LOAD_U32(rasterAddr) + X86_LOAD_U32(RwD3D8RasterExtOffsetAddr));
	auto TexAddr = X86_LOAD_U32(RasterExtOffsetAddr);
	g_currentTextureRect = (GuestTexture*)(base + TexAddr);
	__imp__sub_2DF870(ctx, base);
}

// XGSwizzleRect - This is the final step where texture are ready for Xbox to use, we can upload safely here. (Used during FMV.)
X86_FUNC_IMPL(__imp__sub_40B363);
X86_FUNC(sub_40B363)
{
	auto textureData = GetHostParameter(4, CallConvention::Stdcall, ParameterType::Pointer);
	auto bytesPerPixel = GetHostParameter(8, CallConvention::Stdcall, ParameterType::Integer);

	__imp__sub_40B363(ctx, base);

	if (g_currentTextureRect)
	{
		auto tex = g_currentTextureRect;

		uint32_t size = tex->width * tex->height * bytesPerPixel;

		if (size > tex->sizeOfTextureRect) // this should never pass, but just in case.
			size = tex->sizeOfTextureRect;

		memcpy(tex->mappedMemory, (void*)textureData, size);
		UnlockTextureRect(g_currentTextureRect);
	}
}

// CreateTexture
X86_FUNC_IMPL(__imp__sub_2DFE20);
X86_FUNC(sub_2DFE20)
{
	RwRaster* info = (RwRaster*)g_memory.Translate(ctx.ecx.u32);
	uint8_t level = X86_LOAD_U8(ctx.eax.u32 + 19);
	const uint32_t usage = (info->cType & 0xFF) == rwRASTERTYPECAMERATEXTURE ? 1 : 0;


	X_D3DFORMAT format = (X_D3DFORMAT)ctx.esi.u32;
	__imp__sub_2DFE20(ctx, base);

	if (ctx.eax.u32 != 0)
	{
		//RwRaster* rwM = (RwRaster*)g_memory.Translate(second);
		XD3DResource* texture = (XD3DResource*)(g_memory.Translate(ctx.eax.u32));

		X_D3DRESOURCETYPE textureType = GetXboxTextureType((X_D3DResource*)texture);

		auto newTex = CreateTexture(info->width, info->height, 1, level, usage, format, textureType);

		newTex->guestResource.Common = texture->Common;
		newTex->guestResource.Data = texture->Data;
		newTex->guestResource.Format = texture->Format;
		newTex->guestResource.Lock = texture->Lock;
		newTex->guestResource.Size = texture->Size;

		g_userHeap.Free(g_memory.Translate(ctx.eax.u32)); // free the original texture memory as it's not needed anymore 
		ctx.eax.u32 = g_memory.MapVirtual(newTex);
	}
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

	if (surface)
	{
		auto newSurface = CreateSurface(Width, Height, Format);

		newSurface->guestResource.Common = surface->Common;
		newSurface->guestResource.Data = surface->Data;
		newSurface->guestResource.Format = surface->Format;
		newSurface->guestResource.Lock = surface->Lock;
		newSurface->guestResource.Size = surface->Size;
		newSurface->padding = surface->Parent;
		g_userHeap.Free(g_memory.Translate(ctx.eax.u32)); // free the original surface memory as it's not needed anymore
		ctx.eax.u32 = g_memory.MapVirtual(newSurface);
	}

	ctx.esp.u32 += 4;
}

// left over debugging

//CreateVertexBuffer 
X86_FUNC_IMPL(__imp__sub_2E6750);
X86_FUNC(sub_2E6750)
{
	int size = X86_LOAD_U32(ctx.esp.u32 + 4);
	size = (size + 31) & 0xFFFFFFE0;
	__imp__sub_2E6750(ctx, base);
	auto addr = ctx.eax.u32;
	XD3DResource* buffer = (XD3DResource*)g_memory.Translate(addr);
	//printf("Create VB %x - size: %d\n", (intptr_t)buffer, size);
	//g_VBList.push_back({ buffer, nullptr, size });
}


//LockVertexBuffer
X86_FUNC_IMPL(__imp__sub_3D5FE0);
X86_FUNC(sub_3D5FE0)
{
	__imp__sub_3D5FE0(ctx, base);
}


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

// Stubbed DX8 functions

DWORD* D3D_CommonSetDebugRegisters()
{
	return 0;
}

BOOL D3Device_IsBusy()
{
	return FALSE;
}

int D3D_SetTileNoWait(int a1, int* a2)
{
	return 0;
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

void VideoConfigValueChangedCallback(IConfigDef* config)
{
	// Config options that require internal resolution resize
	g_needsResize |=
		config == &Config::AspectRatio ||
		config == &Config::ResolutionScale ||
		config == &Config::AntiAliasing;

	if (config == &Config::ResolutionScale)
	{
		g_pendingSurfaceResizeFlag = true;
	}

	if (g_needsResize)
	{
		Video::ComputeViewportDimensions();
	}
}

static std::thread g_renderThread([]
	{
#ifdef _WIN32
		SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
		GuestThread::SetThreadName(GetCurrentThreadId(), "Render Thread");
#endif

		RenderCommand commands[30]; //update when adding missing stuff

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
				case RenderCommandType::SetVertexShaderConstants:          ProcSetVertexShaderConstants(cmd); break;
				case RenderCommandType::SetPixelShaderConstants:           ProcSetPixelShaderConstants(cmd); break;
				case RenderCommandType::DrawPrimitive:                     ProcDrawPrimitive(cmd); break;
				case RenderCommandType::DrawIndexedPrimitive:              ProcDrawIndexedPrimitive(cmd); break;
				case RenderCommandType::DrawPrimitiveUP:                   ProcDrawPrimitiveUP(cmd); break;
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



GUEST_FUNCTION_HOOK_STDCALL(sub_3DDAB0, Direct3D_CreateDevice_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD950, SetResolution);

//vertex shader
GUEST_FUNCTION_HOOK_STDCALL(sub_3D2280, SetVertexShader);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1F70, SelectVertexShader);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1F00, LoadVertexShaderProgram);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1EA0, LoadVertexShader);
GUEST_FUNCTION_HOOK_FASTCALL(sub_3D1CE0, D3DDevice_SetVertexShaderConstant1Fast);
GUEST_FUNCTION_HOOK_FASTCALL(sub_3D1D30, D3DDevice_SetVertexShaderConstantNotInlineFast);


GUEST_FUNCTION_HOOK_STDCALL(sub_3D0680, D3DDevice_SetViewport);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D6BF0, D3DDevice_Clear);

GUEST_FUNCTION_HOOK_STDCALL(sub_3D5DB0, DestructResource);

GUEST_FUNCTION_HOOK_FASTCALL(sub_3D2420, D3DDevice_SetRenderState_Simple);

//Resource
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD710, CreateTexture_); //not used for 2D stuff at least.
GUEST_FUNCTION_HOOK_STDCALL(sub_3DA010, CreateSurfaceOfTexture);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0CD0, SetTexture);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0040, D3DDevice_SetRenderTarget);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD810, LockTextureRect); //todo cube version?

GUEST_FUNCTION_HOOK_STDCALL(sub_3CFDF0, SetMaterial);


//Rendering
GUEST_FUNCTION_HOOK_STDCALL(sub_3DC1F0, DrawIndexedPrimitive);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DC150, DrawPrimitive);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DBFE0, DrawPrimitiveUP);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD560, D3DDevice_Swap); //Present


// Misc

// Begin and End push are essentially command system like DX12, this can be used for many things including draw. 
// see pushbuffer.cpp for more info, this patch here just make it so it won't crash when the game calls that.
GUEST_FUNCTION_HOOK_STDCALL(sub_3D4740, D3DDevice_BeginPush);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D4760, D3DDevice_EndPush);

// adx and fmv decoding requires vblank signal
GUEST_FUNCTION_HOOK(sub_3CFE70, BlockUntilVerticalBlank);


//texture stuff
//GUEST_FUNCTION_HOOK_STDCALL(sub_3D2E90, D3DDevice_SetTextureStageStateNotInline);

//GUEST_FUNCTION_HOOK_STDCALL(sub_3CFCD0, D3DDevice_SetTransform);


//			==== STUB  ====
GUEST_FUNCTION_HOOK(sub_3D3240, D3D_CommonSetDebugRegisters);
GUEST_FUNCTION_HOOK(sub_3D0F10, D3Device_IsBusy);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D4720, D3DDevice_IsFencePending);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D5F60, D3DResource_BlockUntilNotBusy);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D4950, D3D_BlockOnTime);
GUEST_FUNCTION_HOOK_STDCALL(sub_3DD8B0, D3D_GetAdapterModeCount);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D0F40, D3D_SetTileNoWait);