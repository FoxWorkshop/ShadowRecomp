#pragma once

#include "rhi/plume_render_interface.h"

#define D3DCLEAR_TARGET  0x1
#define D3DCLEAR_ZBUFFER 0x10

constexpr DWORD X_D3DTS_STAGECOUNT = 4; // Dxbx addition

// Xbox vertex shader counts
#define X_VSH_MAX_ATTRIBUTES          16
#define X_VSH_MAX_STREAMS             16
#define X_VSH_MAX_INSTRUCTION_COUNT   136  // The maximum Xbox shader instruction count
#define TexSlotMax 16u

// pixel container "format" masks
#define X_D3DFORMAT_CUBEMAP               0x00000004      // Set if the texture if a cube map
#define X_D3DFORMAT_FORMAT_MASK           0x0000FF00    
#define X_D3DFORMAT_FORMAT_SHIFT          8
#define X_D3DFORMAT_MIPMAP_MASK           0x000F0000      // # mipmap levels (always 1 for surfaces)
#define X_D3DFORMAT_MIPMAP_SHIFT          16

#define PHYSICAL_MAP_BASE                   0x80000000

// NV2A Push buffer command masks
#define PUSH_METHOD_MASK       0x00001FFC // 12 bits
#define PUSH_METHOD_SHIFT      0 // Dxbx note : Not 2, because methods are actually DWORD offsets (and thus defined with increments of 4)
#define PUSH_METHOD(dwPushCommand) ((dwPushCommand & PUSH_METHOD_MASK) >> PUSH_METHOD_SHIFT)

#define X_D3DVSD_END() 0xFFFFFFFF // to detect when vertex declaration ends with CreateShader function

#define NV_PCRTC_INTR_0                                  0x00000100
#define NV_PCRTC_INTR_0_VBLANK                               (1 << 0)
#define NV_PMC_INTR_0_PCRTC                                (1 << 24)
#define NV_PCRTC_INTR_EN_0                               0x00000140

void CopyVertexShaderConstant(int32_t Register, DWORD* pConstantData, const uint32_t count);
float GetRenderTargetHeight();

inline bool g_needsResize;

using namespace plume;

struct Video
{
	static inline uint32_t s_viewportWidth;
	static inline uint32_t s_viewportHeight;

	static bool CreateHostDevice(const char* sdlVideoDriver, bool graphicsApiRetry);
	static void Present();
	static void StartPipelinePrecompilation();
	static void WaitOnSwapChain();
	static void WaitForGPU();
	static void ComputeViewportDimensions();
};

//this varies per xbox sdk and therefore per game.
typedef enum GuestRenderState {


	// The set starts out with "pixel-shader" render states (all Xbox extensions) :
	X_D3DRS_PSALPHAINPUTS0 = 0,
	X_D3DRS_PSALPHAINPUTS1 = 1,
	X_D3DRS_PSALPHAINPUTS2 = 2,
	X_D3DRS_PSALPHAINPUTS3 = 3,
	X_D3DRS_PSALPHAINPUTS4 = 4,
	X_D3DRS_PSALPHAINPUTS5 = 5,
	X_D3DRS_PSALPHAINPUTS6 = 6,
	X_D3DRS_PSALPHAINPUTS7 = 7,
	X_D3DRS_PSFINALCOMBINERINPUTSABCD = 8,
	X_D3DRS_PSFINALCOMBINERINPUTSEFG = 9,
	X_D3DRS_PSCONSTANT0_0 = 10,
	X_D3DRS_PSCONSTANT0_1 = 11,
	X_D3DRS_PSCONSTANT0_2 = 12,
	X_D3DRS_PSCONSTANT0_3 = 13,
	X_D3DRS_PSCONSTANT0_4 = 14,
	X_D3DRS_PSCONSTANT0_5 = 15,
	X_D3DRS_PSCONSTANT0_6 = 16,
	X_D3DRS_PSCONSTANT0_7 = 17,
	X_D3DRS_PSCONSTANT1_0 = 18,
	X_D3DRS_PSCONSTANT1_1 = 19,
	X_D3DRS_PSCONSTANT1_2 = 20,
	X_D3DRS_PSCONSTANT1_3 = 21,
	X_D3DRS_PSCONSTANT1_4 = 22,
	X_D3DRS_PSCONSTANT1_5 = 23,
	X_D3DRS_PSCONSTANT1_6 = 24,
	X_D3DRS_PSCONSTANT1_7 = 25,
	X_D3DRS_PSALPHAOUTPUTS0 = 26,
	X_D3DRS_PSALPHAOUTPUTS1 = 27,
	X_D3DRS_PSALPHAOUTPUTS2 = 28,
	X_D3DRS_PSALPHAOUTPUTS3 = 29,
	X_D3DRS_PSALPHAOUTPUTS4 = 30,
	X_D3DRS_PSALPHAOUTPUTS5 = 31,
	X_D3DRS_PSALPHAOUTPUTS6 = 32,
	X_D3DRS_PSALPHAOUTPUTS7 = 33,
	X_D3DRS_PSRGBINPUTS0 = 34,
	X_D3DRS_PSRGBINPUTS1 = 35,
	X_D3DRS_PSRGBINPUTS2 = 36,
	X_D3DRS_PSRGBINPUTS3 = 37,
	X_D3DRS_PSRGBINPUTS4 = 38,
	X_D3DRS_PSRGBINPUTS5 = 39,
	X_D3DRS_PSRGBINPUTS6 = 40,
	X_D3DRS_PSRGBINPUTS7 = 41,
	X_D3DRS_PSCOMPAREMODE = 42,
	X_D3DRS_PSFINALCOMBINERCONSTANT0 = 43,
	X_D3DRS_PSFINALCOMBINERCONSTANT1 = 44,
	X_D3DRS_PSRGBOUTPUTS0 = 45,
	X_D3DRS_PSRGBOUTPUTS1 = 46,
	X_D3DRS_PSRGBOUTPUTS2 = 47,
	X_D3DRS_PSRGBOUTPUTS3 = 48,
	X_D3DRS_PSRGBOUTPUTS4 = 49,
	X_D3DRS_PSRGBOUTPUTS5 = 50,
	X_D3DRS_PSRGBOUTPUTS6 = 51,
	X_D3DRS_PSRGBOUTPUTS7 = 52,
	X_D3DRS_PSCOMBINERCOUNT = 53,
	X_D3DRS_PS_RESERVED = 54, // Dxbx note : This takes the slot of X_D3DPIXELSHADERDEF.PSTextureModes, set by D3DDevice_SetRenderState_LogicOp?
	X_D3DRS_PSDOTMAPPING = 55,
	X_D3DRS_PSINPUTTEXTURE = 56,
	// End of "pixel-shader" render states, continuing with "simple" render states :
	X_D3DRS_ZFUNC = 57, // D3DCMPFUNC
	X_D3DRS_ALPHAFUNC = 58, // D3DCMPFUNC
	X_D3DRS_ALPHABLENDENABLE = 59, // TRUE to enable alpha blending
	X_D3DRS_ALPHATESTENABLE = 60, // TRUE to enable alpha tests
	X_D3DRS_ALPHAREF = 61, // BYTE
	X_D3DRS_SRCBLEND = 62, // D3DBLEND
	X_D3DRS_DESTBLEND = 63, // D3DBLEND
	X_D3DRS_ZWRITEENABLE = 64, // TRUE to enable Z writes
	X_D3DRS_DITHERENABLE = 65, // TRUE to enable dithering
	X_D3DRS_SHADEMODE = 66, // D3DSHADEMODE
	X_D3DRS_COLORWRITEENABLE = 67, // D3DCOLORWRITEENABLE_ALPHA, etc. per-channel write enable
	X_D3DRS_STENCILZFAIL = 68, // D3DSTENCILOP to do if stencil test passes and Z test fails
	X_D3DRS_STENCILPASS = 69, // D3DSTENCILOP to do if both stencil and Z tests pass
	X_D3DRS_STENCILFUNC = 70, // D3DCMPFUNC
	X_D3DRS_STENCILREF = 71, // BYTE reference value used in stencil test
	X_D3DRS_STENCILMASK = 72, // BYTE mask value used in stencil test
	X_D3DRS_STENCILWRITEMASK = 73, // BYTE write mask applied to values written to stencil buffer
	X_D3DRS_BLENDOP = 74, // D3DBLENDOP setting
	X_D3DRS_BLENDCOLOR = 75, // D3DCOLOR for D3DBLEND_CONSTANTCOLOR
	X_D3DRS_SWATHWIDTH = 76, // D3DSWATHWIDTH (Xbox ext.)
	X_D3DRS_POLYGONOFFSETZSLOPESCALE = 77, // float Z factor for shadow maps (Xbox ext.)
	X_D3DRS_POLYGONOFFSETZOFFSET = 78, // Xbox ext.
	X_D3DRS_POINTOFFSETENABLE = 79, // Xbox ext.
	X_D3DRS_WIREFRAMEOFFSETENABLE = 80, // Xbox ext.
	X_D3DRS_SOLIDOFFSETENABLE = 81, // Xbox ext.
	X_D3DRS_DEPTHCLIPCONTROL = 82, // [4432+] Xbox ext.
	X_D3DRS_STIPPLEENABLE = 83, // [4627+] Xbox ext.
	X_D3DRS_SIMPLE_UNUSED8 = 84, // [4627+]
	X_D3DRS_SIMPLE_UNUSED7 = 85, // [4627+]
	X_D3DRS_SIMPLE_UNUSED6 = 86, // [4627+]
	X_D3DRS_SIMPLE_UNUSED5 = 87, // [4627+]
	X_D3DRS_SIMPLE_UNUSED4 = 88, // [4627+]
	X_D3DRS_SIMPLE_UNUSED3 = 89, // [4627+]
	X_D3DRS_SIMPLE_UNUSED2 = 90, // [4627+]
	X_D3DRS_SIMPLE_UNUSED1 = 91, // [4627+]
	// End of "simple" render states, continuing with "deferred" render states :
	X_D3DRS_FOGENABLE = 92,
	X_D3DRS_FOGTABLEMODE = 93,
	X_D3DRS_FOGSTART = 94,
	X_D3DRS_FOGEND = 95,
	X_D3DRS_FOGDENSITY = 96,
	X_D3DRS_RANGEFOGENABLE = 97,
	X_D3DRS_WRAP0 = 98,
	X_D3DRS_WRAP1 = 99,
	X_D3DRS_WRAP2 = 100, // Dxbx addition
	X_D3DRS_WRAP3 = 101, // Dxbx addition
	X_D3DRS_LIGHTING = 102,
	X_D3DRS_SPECULARENABLE = 103,
	X_D3DRS_LOCALVIEWER = 104, // Dxbx addition
	X_D3DRS_COLORVERTEX = 105,
	X_D3DRS_BACKSPECULARMATERIALSOURCE = 106, // Xbox ext. nsp.
	X_D3DRS_BACKDIFFUSEMATERIALSOURCE = 107, // Xbox ext. nsp.
	X_D3DRS_BACKAMBIENTMATERIALSOURCE = 108, // Xbox ext. nsp.
	X_D3DRS_BACKEMISSIVEMATERIALSOURCE = 109, // Xbox ext. nsp.
	X_D3DRS_SPECULARMATERIALSOURCE = 110,
	X_D3DRS_DIFFUSEMATERIALSOURCE = 111,
	X_D3DRS_AMBIENTMATERIALSOURCE = 112,
	X_D3DRS_EMISSIVEMATERIALSOURCE = 113,
	X_D3DRS_BACKAMBIENT = 114, // Xbox ext. nsp.
	X_D3DRS_AMBIENT = 115,
	X_D3DRS_POINTSIZE = 116,
	X_D3DRS_POINTSIZE_MIN = 117,
	X_D3DRS_POINTSPRITEENABLE = 118,
	X_D3DRS_POINTSCALEENABLE = 119,
	X_D3DRS_POINTSCALE_A = 120,
	X_D3DRS_POINTSCALE_B = 121,
	X_D3DRS_POINTSCALE_C = 122,
	X_D3DRS_POINTSIZE_MAX = 123,
	X_D3DRS_PATCHEDGESTYLE = 124, // Dxbx addition
	X_D3DRS_PATCHSEGMENTS = 125,
	X_D3DRS_SWAPFILTER = 126, // [4039+] Xbox ext. nsp. D3DTEXF_LINEAR etc. filter to use for Swap
	X_D3DRS_PRESENTATIONINTERVAL = 127, // [4627+] Xbox ext. nsp. TODO : Use 4361?
	X_D3DRS_DEFERRED_UNUSED8 = 128, // [4627+]
	X_D3DRS_DEFERRED_UNUSED7 = 129, // [4627+]
	X_D3DRS_DEFERRED_UNUSED6 = 130, // [4627+]
	X_D3DRS_DEFERRED_UNUSED5 = 131, // [4627+]
	X_D3DRS_DEFERRED_UNUSED4 = 132, // [4627+]
	X_D3DRS_DEFERRED_UNUSED3 = 133, // [4627+]
	X_D3DRS_DEFERRED_UNUSED2 = 134, // [4627+]
	X_D3DRS_DEFERRED_UNUSED1 = 135, // [4627+]
	// End of "deferred" render states, continuing with "complex" render states :
	X_D3DRS_PSTEXTUREMODES = 136, // Xbox ext.
	X_D3DRS_VERTEXBLEND = 137,
	X_D3DRS_FOGCOLOR = 138,
	X_D3DRS_FILLMODE = 139,
	X_D3DRS_BACKFILLMODE = 140, // Dxbx addition : Xbox ext. nsp.
	X_D3DRS_TWOSIDEDLIGHTING = 141, // Dxbx addition : Xbox ext. nsp.
	X_D3DRS_NORMALIZENORMALS = 142,
	X_D3DRS_ZENABLE = 143,
	X_D3DRS_STENCILENABLE = 144,
	X_D3DRS_STENCILFAIL = 145,
	X_D3DRS_FRONTFACE = 146, // Dxbx addition : Xbox ext. nsp.
	X_D3DRS_CULLMODE = 147,
	X_D3DRS_TEXTUREFACTOR = 148,
	X_D3DRS_ZBIAS = 149,
	X_D3DRS_LOGICOP = 150, // Xbox ext. nsp.
	X_D3DRS_EDGEANTIALIAS = 151, // Dxbx note : No Xbox ext. (according to Direct3D8) !
	X_D3DRS_MULTISAMPLEANTIALIAS = 152,
	X_D3DRS_MULTISAMPLEMASK = 153,
	//	X_D3DRS_MULTISAMPLETYPE = 15, // [-4039] Xbox ext.
	X_D3DRS_MULTISAMPLEMODE = 154, // [4361+] Xbox ext. // D3DMULTISAMPLEMODE for the backbuffer
	X_D3DRS_MULTISAMPLERENDERTARGETMODE = 155, // [4039+] Xbox ext.
	X_D3DRS_SHADOWFUNC = 156, // D3DCMPFUNC (Xbox extension)
	X_D3DRS_LINEWIDTH = 157,
	X_D3DRS_SAMPLEALPHA = 158,
	X_D3DRS_DXT1NOISEENABLE = 159,
	X_D3DRS_YUVENABLE = 160, // [3911+] Xbox ext.
	X_D3DRS_OCCLUSIONCULLENABLE = 161, // [3911+] Xbox ext.
	X_D3DRS_STENCILCULLENABLE = 162, // [3911+] Xbox ext.
	X_D3DRS_ROPZCMPALWAYSREAD = 163, // [3911+] Xbox ext.
	X_D3DRS_ROPZREAD = 164,
	X_D3DRS_DONOTCULLUNCOMPRESSED = 165,
	// End of "complex" render states.
	X_D3DRS_UNK = 0x7fffffff // deferred render state "unknown" flag
} X_D3DRENDERSTATETYPE;

#define X_D3DRS_PS_FIRST X_D3DRS_PSALPHAINPUTS0
#define X_D3DRS_LAST X_D3DRS_DONOTCULLUNCOMPRESSED


typedef enum _X_D3DPRIMITIVETYPE
{
	X_D3DPT_NONE = 0, // Dxbx addition

	X_D3DPT_POINTLIST = 1,
	X_D3DPT_LINELIST = 2,
	X_D3DPT_LINELOOP = 3,    // Xbox only
	X_D3DPT_LINESTRIP = 4,
	X_D3DPT_TRIANGLELIST = 5,
	X_D3DPT_TRIANGLESTRIP = 6,
	X_D3DPT_TRIANGLEFAN = 7,
	X_D3DPT_QUADLIST = 8,    // Xbox only
	X_D3DPT_QUADSTRIP = 9,    // Xbox only
	X_D3DPT_POLYGON = 10,   // Xbox only

	X_D3DPT_MAX = 11,
	X_D3DPT_INVALID = 0x7fffffff, /* force 32-bit size enum */
}
X_D3DPRIMITIVETYPE;

typedef enum _X_D3DFORMAT
{
	/*
		Xbox1 D3DFORMAT notes
		---------------------

		The Xbox1 D3DFORMAT type consists of 4 different format categories :
		1. Swizzled (improves data locality, incompatible with native Direct3D)
		2. Compressed (DXT compression, giving 4:1 reduction on 4x4 pixel blocks)
		3. Linear (compatible with native Direct3D)
		4. Depth (Fixed or Floating point, stored Linear or Swizzled)

		Requirements\Format      Swizzled  Compressed  Linear  Depth   Notes

		Power-of-two required ?  YES       YES         NO      NO
		Mipmap supported ?       YES       YES         NO      YES     Linear has MipmapLevels = 1
		CubeMaps supported ?     YES       YES         NO      NO      Cubemaps have 6 faces
		Supports volumes ?       YES       YES         NO      NO      Volumes have 3 dimensions, Textures have 2
		Can be a rendertarget ?  YES       YES         YES     LINEAR  Depth buffers can only be rendered to if stored Linear

		Implications :
		- CubeMaps must be square
		- Volumes cannot be cube mapped and vice versa

		Maximum dimensions :
		2D : 4096 x 4096 (12 mipmap levels)
		3D : 512 x 512 x 512 (9 mipmap levels)

	*/

	// Xbox D3DFORMAT types :
	// See https://wiki.beyondunreal.com/Legacy:Texture_Format

	// Swizzled Formats

	X_D3DFMT_L8 = 0x00,
	X_D3DFMT_AL8 = 0x01,
	X_D3DFMT_A1R5G5B5 = 0x02,
	X_D3DFMT_X1R5G5B5 = 0x03,
	X_D3DFMT_A4R4G4B4 = 0x04,
	X_D3DFMT_R5G6B5 = 0x05,
	X_D3DFMT_A8R8G8B8 = 0x06,
	X_D3DFMT_X8R8G8B8 = 0x07,
	X_D3DFMT_X8L8V8U8 = 0x07, // Alias

	X_D3DFMT_P8 = 0x0b, // 8-bit Palletized

	X_D3DFMT_A8 = 0x19,
	X_D3DFMT_A8L8 = 0x1a,
	X_D3DFMT_R6G5B5 = 0x27,
	X_D3DFMT_L6V5U5 = 0x27, // Alias

	X_D3DFMT_G8B8 = 0x28,
	X_D3DFMT_V8U8 = 0x28, // Alias

	X_D3DFMT_R8B8 = 0x29,
	X_D3DFMT_D24S8 = 0x2a,
	X_D3DFMT_F24S8 = 0x2b,
	X_D3DFMT_D16 = 0x2c,
	X_D3DFMT_D16_LOCKABLE = 0x2c, // Alias

	X_D3DFMT_F16 = 0x2d,
	X_D3DFMT_L16 = 0x32,
	X_D3DFMT_V16U16 = 0x33,
	X_D3DFMT_R5G5B5A1 = 0x38,
	X_D3DFMT_R4G4B4A4 = 0x39,
	X_D3DFMT_A8B8G8R8 = 0x3A,
	X_D3DFMT_Q8W8V8U8 = 0x3A, // Alias

	X_D3DFMT_B8G8R8A8 = 0x3B,
	X_D3DFMT_R8G8B8A8 = 0x3C,

	// YUV Formats

	X_D3DFMT_YUY2 = 0x24,
	X_D3DFMT_UYVY = 0x25,

	// Compressed Formats

	X_D3DFMT_DXT1 = 0x0C, // opaque/one-bit alpha
	X_D3DFMT_DXT2 = 0x0E, // Alias for X_D3DFMT_DXT3
	X_D3DFMT_DXT3 = 0x0E, // linear alpha
	X_D3DFMT_DXT4 = 0x0F, // Alias for X_D3DFMT_DXT5
	X_D3DFMT_DXT5 = 0x0F, // interpolated alpha

	// Linear Formats

	X_D3DFMT_LIN_A1R5G5B5 = 0x10,
	X_D3DFMT_LIN_R5G6B5 = 0x11,
	X_D3DFMT_LIN_A8R8G8B8 = 0x12,
	X_D3DFMT_LIN_L8 = 0x13,
	X_D3DFMT_LIN_R8B8 = 0x16,
	X_D3DFMT_LIN_G8B8 = 0x17,
	X_D3DFMT_LIN_V8U8 = 0x17, // Alias

	X_D3DFMT_LIN_AL8 = 0x1b,
	X_D3DFMT_LIN_X1R5G5B5 = 0x1c,
	X_D3DFMT_LIN_A4R4G4B4 = 0x1d,
	X_D3DFMT_LIN_X8R8G8B8 = 0x1e,
	X_D3DFMT_LIN_X8L8V8U8 = 0x1e, // Alias

	X_D3DFMT_LIN_A8 = 0x1f,
	X_D3DFMT_LIN_A8L8 = 0x20,
	X_D3DFMT_LIN_D24S8 = 0x2E,
	X_D3DFMT_LIN_F24S8 = 0x2f,
	X_D3DFMT_LIN_D16 = 0x30,
	X_D3DFMT_LIN_F16 = 0x31,
	X_D3DFMT_LIN_L16 = 0x35,
	X_D3DFMT_LIN_V16U16 = 0x36,
	X_D3DFMT_LIN_R6G5B5 = 0x37,
	X_D3DFMT_LIN_L6V5U5 = 0x37, // Alias

	X_D3DFMT_LIN_R5G5B5A1 = 0x3D,
	X_D3DFMT_LIN_R4G4B4A4 = 0x3e,
	X_D3DFMT_LIN_A8B8G8R8 = 0x3f,
	X_D3DFMT_LIN_B8G8R8A8 = 0x40,
	X_D3DFMT_LIN_R8G8B8A8 = 0x41,

	X_D3DFMT_VERTEXDATA = 0x64,

	X_D3DFMT_INDEX16 = 101/*=D3DFMT_INDEX16*/, // Dxbx addition : Not an Xbox format, used internally

	X_D3DFMT_UNKNOWN = 0xFFFFFFFF - 3,  // Unique declaration to make overloads possible
}
X_D3DFORMAT, * PX_D3DFORMAT;

typedef enum _X_D3DCULL
{
	X_D3DCULL_NONE = 0,
	X_D3DCULL_CW = 0x900,
	X_D3DCULL_CCW = 0x901,
	X_D3DCULL_FORCE_DWORD = 0x7fffffff
}
X_D3DCULL;

typedef enum _D3DCMPFUNC {
	D3DCMP_NEVER = 0x200,
	D3DCMP_LESS = 0x201,
	D3DCMP_EQUAL = 0x202,
	D3DCMP_LESSEQUAL = 0x203,
	D3DCMP_GREATER = 0x204,
	D3DCMP_NOTEQUAL = 0x205,
	D3DCMP_GREATEREQUAL = 0x206,
	D3DCMP_ALWAYS = 0x207,
	D3DCMP_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DCMPFUNC;

typedef enum _D3DSTENCILOP {
	D3DSTENCILOP_KEEP = 1,
	D3DSTENCILOP_ZERO = 2,
	D3DSTENCILOP_REPLACE = 3,
	D3DSTENCILOP_INCRSAT = 4,
	D3DSTENCILOP_DECRSAT = 5,
	D3DSTENCILOP_INVERT = 6,
	D3DSTENCILOP_INCR = 7,
	D3DSTENCILOP_DECR = 8,
	D3DSTENCILOP_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DSTENCILOP;

// Values for material source
typedef enum _D3DMATERIALCOLORSOURCE
{
	D3DMCS_MATERIAL = 0,            // Color from material is used
	D3DMCS_COLOR1 = 1,            // Diffuse vertex color is used
	D3DMCS_COLOR2 = 2,            // Specular vertex color is used
	D3DMCS_FORCE_DWORD = 0x7fffffff,   // force 32-bit size enum
} D3DMATERIALCOLORSOURCE;

// X_D3DTEXTUREFILTERTYPE Values
constexpr DWORD X_D3DTEXF_NONE = 0;
constexpr DWORD X_D3DTEXF_POINT = 1;
constexpr DWORD X_D3DTEXF_LINEAR = 2;
constexpr DWORD X_D3DTEXF_ANISOTROPIC = 3;
constexpr DWORD X_D3DTEXF_QUINCUNX = 4; // Xbox extension
constexpr DWORD X_D3DTEXF_GAUSSIANCUBIC = 5;

// X_D3DCLEAR values :
constexpr DWORD X_D3DCLEAR_ZBUFFER = 0x00000001;
constexpr DWORD X_D3DCLEAR_STENCIL = 0x00000002;
constexpr DWORD X_D3DCLEAR_TARGET_R = 0x00000010;  // Clear target surface R component (Xbox ext.)
constexpr DWORD X_D3DCLEAR_TARGET_G = 0x00000020;  // Clear target surface G component (Xbox ext.)
constexpr DWORD X_D3DCLEAR_TARGET_B = 0x00000040;  // Clear target surface B component (Xbox ext.)
constexpr DWORD X_D3DCLEAR_TARGET_A = 0x00000080;  // Clear target surface A component (Xbox ext.)
constexpr DWORD X_D3DCLEAR_TARGET = X_D3DCLEAR_TARGET_R | X_D3DCLEAR_TARGET_G | X_D3DCLEAR_TARGET_B | X_D3DCLEAR_TARGET_A;


enum class ResourceType
{
	Texture,
	VolumeTexture,
	VertexBuffer,
	IndexBuffer,
	RenderTarget,
	DepthStencil,
	VertexDeclaration,
	VertexShader,
	PixelShader
};

struct XD3DResource
{
	uint32_t Common;
	uint32_t Data;
	uint32_t Lock;
	uint32_t Format;
	uint32_t Size;
};

struct X_D3DResource
{
	DWORD Common;
	DWORD Data;
	DWORD Lock;
};


struct GuestResource
{
	XD3DResource guestResource;
	//uint8_t Guard[56];
	uint32_t refCount = 1;
	ResourceType type;

	GuestResource(ResourceType type) : type(type)
	{
	}

	void AddRef()
	{
		std::atomic_ref atomicRef(refCount);

		uint32_t originalValue, incrementedValue;
		do
		{
			originalValue = refCount;
			incrementedValue = ((originalValue)+1);
		} while (!atomicRef.compare_exchange_weak(originalValue, incrementedValue));
	}

	void Release()
	{
		std::atomic_ref atomicRef(refCount);

		uint32_t originalValue, decrementedValue;
		do
		{
			originalValue = refCount;
			decrementedValue = ((originalValue)-1);
		} while (!atomicRef.compare_exchange_weak(originalValue, decrementedValue));

		// Normally we are supposed to release here, so only use this
		// function when you know you won't be the one destructing it.
	}
};



struct GuestBaseTexture : GuestResource
{
	std::unique_ptr<RenderTexture> textureHolder;
	RenderTexture* texture = nullptr;
	std::unique_ptr<RenderTextureView> textureView;
	uint32_t width = 0;
	uint32_t height = 0;
	RenderFormat format = RenderFormat::UNKNOWN;
	uint32_t descriptorIndex = 0;
	RenderTextureLayout layout = RenderTextureLayout::UNKNOWN;

	GuestBaseTexture(ResourceType type) : GuestResource(type)
	{
	}
};

// Texture/VolumeTexture (will likely need to implement D3DResource)
struct GuestTexture : GuestBaseTexture
{
	uint32_t depth = 0;
	RenderTextureViewDimension viewDimension = RenderTextureViewDimension::UNKNOWN;
	void* mappedMemory = nullptr;
	std::unique_ptr<RenderFramebuffer> framebuffer;
	std::unique_ptr<GuestTexture> patchedTexture;
	std::unique_ptr<GuestTexture> recreatedCubeMapTexture;
	struct GuestSurface* sourceSurface = nullptr;
	bool isRenderTarget = false;
};

struct GuestLockedRect
{
	uint32_t pitch;
	uint32_t bits;
};


// VertexBuffer/IndexBuffer
struct GuestBuffer : GuestResource
{
	std::unique_ptr<RenderBuffer> buffer;
	XXH64_hash_t hash = 0;
	void* mappedMemory = nullptr;
	uint32_t dataSize = 0;
	RenderFormat format = RenderFormat::UNKNOWN;
	uint32_t guestFormat = 0;
	bool lockedReadOnly = false;
	bool isVertexBuffer = false;
	int32_t stride = 0;
};

// RenderTarget/DepthStencil
struct GuestSurface : GuestBaseTexture
{
	uint32_t guestFormat = 0;
	ankerl::unordered_dense::map<const RenderTexture*, std::unique_ptr<RenderFramebuffer>> framebuffers;
	RenderSampleCounts sampleCount = RenderSampleCount::COUNT_1;
	ankerl::unordered_dense::set<GuestTexture*> destinationTextures;
};

struct _D3DRECT
{
	LONG x1;
	LONG y1;
	LONG x2;
	LONG y2;
};

struct GuestVertexElement
{
	DWORD StreamIndex;
	DWORD Offset;
	DWORD Format;
	BYTE TessellationType;
	BYTE TessellationSource;
	BYTE Padding0;
	BYTE Padding1;
};

// This is used to initialize the last vertex element in a vertex declaration
// array
//
#define D3DDECL_END() {0xFF,0,D3DDECLTYPE_UNUSED,0,0,0}

// ******************************************************************
struct X_VERTEXATTRIBUTEFORMAT
{
	// Note : Alignment looks okay even without #pragma pack(1) / #include "AlignPrefix1.h" (and it's closure)
	GuestVertexElement Slots[X_VSH_MAX_ATTRIBUTES];
};

struct X_D3DVertexShader
{
#if 0 
	DWORD Signature; // Note : Debug XBE's have a 'Vshd' DWORD signature prefix
#endif
	DWORD RefCount; // Based on the observation this member is set to 1 in D3DDevice_CreateVertexShader and decreased in D3DDevice_DeleteVertexShader
	DWORD Flags; // Contains X_VERTEXSHADER_FLAG_* bits
	DWORD ProgramSize;
	DWORD ProgramAndConstantsDwords; // Sum of ProgramSize + constant count, expressed in instruction slots, taking 4 DWORD's per slot (see X_VSH_INSTRUCTION_SIZE)
	BYTE Dimensionality[4]; // Guesswork, since all 4 bytes (for all 4 textures) are most often set to 0 (or 2 when a texture isn't used) and 1, 3 and 4 also occur (and nothing else)
	X_VERTEXATTRIBUTEFORMAT VertexAttribute;
	DWORD ProgramAndConstants[X_VSH_MAX_INSTRUCTION_COUNT]; // The binary function data and constants (contents continues futher outside this struct, up to ProgramAndConstantsDwords * 4 (=X_VSH_INSTRUCTION_SIZE) DWORD's)
};

typedef struct _D3DVERTEXELEMENT9
{
	WORD    Stream;     // Stream index
	WORD    Offset;     // Offset in the stream in bytes
	BYTE    Type;       // Data type
	BYTE    Method;     // Processing method
	BYTE    Usage;      // Semantics
	BYTE    UsageIndex; // Semantic index
} D3DVERTEXELEMENT9, * LPD3DVERTEXELEMENT9;

struct GuestVertexDeclaration : GuestResource
{
	XXH64_hash_t hash = 0;
	std::unique_ptr<RenderInputElement[]> inputElements;
	std::unique_ptr<D3DVERTEXELEMENT9[]> vertexElements;
	uint32_t inputElementCount = 0;
	uint32_t vertexElementCount = 0;
	uint32_t swappedTexcoords = 0;
	bool hasR11G11B10Normal = false;
	bool vertexStreams[16]{};
	uint32_t indexVertexStream = 0;
#ifdef _DEBUG
	uint8_t uniqueID = 0;
#endif
};

// VertexShader/PixelShader
struct GuestShader : GuestResource
{
	Mutex mutex;
	std::unique_ptr<RenderShader> shader;
	struct ShaderCacheEntry* shaderCacheEntry = nullptr;
	ankerl::unordered_dense::map<uint32_t, std::unique_ptr<RenderShader>> linkedShaders;
#ifdef SHADOW_RECOMP_D3D12
	std::vector<ComPtr<IDxcBlob>> shaderBlobs;
	ComPtr<IDxcBlobEncoding> libraryBlob;
#endif
#ifdef ASYNC_PSO_DEBUG
	const char* name = "<unknown>";
#endif
};


//_XD3DVIEWPORT8
struct GuestViewport
{
	DWORD X;
	DWORD Y;
	DWORD Width;
	DWORD Height;
	float MinZ;
	float MaxZ;
};

struct _XMETAL_PushBuffer
{
	uint32_t m_pPut;
	uint32_t m_pThreshold;
};


struct GuestDevice
{
	_XMETAL_PushBuffer m_Pusher;
	uint32_t m_StateFlags;
	uint8_t padding2[1911];
	uint32_t m_pPixelShader;
	uint8_t padding3[11];
	uint32_t m_pVertexShader;
	uint32_t m_VertexShaderHandle;
	uint8_t padding4[412];
	uint32_t m_cRefs;
	uint8_t padding5[1444];
	GuestViewport viewport;
	uint8_t padding6[2608];
	uint32_t m_ConstantMode;
	uint8_t padding7[216];
	uint32_t m_pRenderTarget;
	uint8_t padding8[12];
	uint8_t m_FrameBufferSurfaces[0x48];
	uint8_t padding9[2625];
};


static_assert(sizeof(GuestDevice) == 0x24A0);



struct X_D3DPixelContainer : public X_D3DResource
{
	DWORD		Format; // Format information about the texture.
	DWORD       Size; // Size of a non power-of-2 texture, must be zero otherwise
};

typedef enum _X_D3DRESOURCETYPE
{
	X_D3DRTYPE_NONE = 0,
	X_D3DRTYPE_SURFACE = 1, // = D3DRESOURCETYPE.D3DRTYPE_SURFACE
	X_D3DRTYPE_VOLUME = 2, // = D3DRESOURCETYPE.D3DRTYPE_VOLUME
	X_D3DRTYPE_TEXTURE = 3, // = D3DRESOURCETYPE.D3DRTYPE_TEXTURE
	X_D3DRTYPE_VOLUMETEXTURE = 4, // = D3DRESOURCETYPE.D3DRTYPE_VOLUMETEXTURE
	X_D3DRTYPE_CUBETEXTURE = 5, // = D3DRESOURCETYPE.D3DRTYPE_CUBETEXTURE
	X_D3DRTYPE_VERTEXBUFFER = 6, // = D3DRESOURCETYPE.D3DRTYPE_VERTEXBUFFER
	X_D3DRTYPE_INDEXBUFFER = 7, // = D3DRESOURCETYPE.D3DRTYPE_INDEXBUFFER
	X_D3DRTYPE_PUSHBUFFER = 8,
	X_D3DRTYPE_PALETTE = 9,
	X_D3DRTYPE_FIXUP = 10,

	X_D3DRTYPE_FORCE_DWORD = 0x7fffffff
}
X_D3DRESOURCETYPE;

// Values, used with D3DTSS_TEXCOORDINDEX, to specify that the vertex data (position
// and normal in the camera space) should be taken as texture coordinates.
// Low 16 bits are used to specify texture coordinate index, to take the WRAP mode from.
#define X_D3DTSS_TCI_PASSTHRU                             0x00000000
#define X_D3DTSS_TCI_CAMERASPACENORMAL                    0x00010000
#define X_D3DTSS_TCI_CAMERASPACEPOSITION                  0x00020000
#define X_D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR          0x00030000
#define X_D3DTSS_TCI_OBJECT                               0x00040000 
#define X_D3DTSS_TCI_SPHEREMAP                            0x00050000

enum X_D3DTRANSFORMSTATETYPE {
	X_D3DTS_VIEW = 0,
	X_D3DTS_PROJECTION = 1,
	X_D3DTS_TEXTURE0 = 2,
	X_D3DTS_TEXTURE1 = 3,
	X_D3DTS_TEXTURE2 = 4,
	X_D3DTS_TEXTURE3 = 5,
	X_D3DTS_WORLD = 6,
	X_D3DTS_WORLD1 = 7,
	X_D3DTS_WORLD2 = 8,
	X_D3DTS_WORLD3 = 9,

	X_D3DTS_MAX = 10,
	X_D3DTS_FORCE_DWORD = 0x7FFFFFFF,
};

enum _D3DSWAPEFFECT : __int32
{
	D3DSWAPEFFECT_DISCARD = 0x1,
	D3DSWAPEFFECT_FLIP = 0x2,
	D3DSWAPEFFECT_COPY = 0x3,
	D3DSWAPEFFECT_COPY_VSYNC = 0x4,
	D3DSWAPEFFECT_FORCE_DWORD = 0x7FFFFFFF,
};



struct X_D3DPRESENT_PARAMETERS
{
	uint32_t BackBufferWidth;
	uint32_t BackBufferHeight;
	X_D3DFORMAT BackBufferFormat;
	uint32_t BackBufferCount;
	uint32_t MultiSampleType;
	_D3DSWAPEFFECT SwapEffect;
	uint32_t hDeviceWindow;
	int32_t Windowed;
	int32_t EnableAutoDepthStencil;
	X_D3DFORMAT AutoDepthStencilFormat;
	uint32_t Flags;
	uint32_t FullScreen_RefreshRateInHz;
	uint32_t FullScreen_PresentationInterval;
};



#ifndef D3DMATRIX_DEFINED
typedef struct _D3DMATRIX {
	union {
		struct {
			float        _11, _12, _13, _14;
			float        _21, _22, _23, _24;
			float        _31, _32, _33, _34;
			float        _41, _42, _43, _44;

		};
		float m[4][4];
	};
} D3DMATRIX;
typedef D3DMATRIX D3DXMATRIX;
#define D3DMATRIX_DEFINED
#endif

typedef enum _D3DVERTEXBLENDFLAGS
{
	X_D3DVBF_DISABLE = 0,              // 1 matrix,   0 weights => final weight effectively 1 (Disable vertex blending)
	X_D3DVBF_1WEIGHTS = 1,             // 2 matrices, 1 weights => final weight calculated
	X_D3DVBF_2WEIGHTS = 3,             // 3 matrices, 2 weights => final weight calculated
	X_D3DVBF_3WEIGHTS = 5,             // 4 matrices, 3 weights => final weight calculated
	X_D3DVBF_2WEIGHTS2MATRICES = 2,    // 2 matrices, 2 weights (Xbox ext.)
	X_D3DVBF_3WEIGHTS3MATRICES = 4,    // 3 matrices, 3 weights (Xbox ext.)
	X_D3DVBF_4WEIGHTS4MATRICES = 6,    // 4 matrices, 4 weights (Xbox ext.)
	X_D3DVBF_FORCE_DWORD = 0x7fffffff
} X_D3DVERTEXBLENDFLAGS;

// FVF Definitions
#define X_D3DFVF_RESERVED0        0x001
#define X_D3DFVF_XYZ              0x002
#define X_D3DFVF_XYZRHW           0x004
#define X_D3DFVF_XYZB1            0x006
#define X_D3DFVF_XYZB2            0x008
#define X_D3DFVF_XYZB3            0x00a
#define X_D3DFVF_XYZB4            0x00c
#define X_D3DFVF_POSITION_MASK    0x00E
#define X_D3DFVF_NORMAL           0x010
#define X_D3DFVF_RESERVED1        0x020
#define X_D3DFVF_DIFFUSE          0x040
#define X_D3DFVF_SPECULAR         0x080
#define X_D3DFVF_TEXCOUNT_MASK    0xf00
#define X_D3DFVF_TEXCOUNT_SHIFT   8
#define X_D3DFVF_TEX0             0x000
#define X_D3DFVF_TEX1             0x100
#define X_D3DFVF_TEX2             0x200
#define X_D3DFVF_TEX3             0x300
#define X_D3DFVF_TEX4             0x400
#define X_D3DFVF_TEX5             0x500
#define X_D3DFVF_TEX6             0x600
#define X_D3DFVF_TEX7             0x700
#define X_D3DFVF_TEX8             0x800
#define X_D3DFVF_TEXTUREFORMAT1   0x003
#define X_D3DFVF_TEXTUREFORMAT2   0x000
#define X_D3DFVF_TEXTUREFORMAT3   0x001
#define X_D3DFVF_TEXTUREFORMAT4   0x002

enum _D3DBLEND : __int32
{
	D3DBLEND_ZERO = 0x0,
	D3DBLEND_ONE = 0x1,
	D3DBLEND_SRCCOLOR = 0x300,
	D3DBLEND_INVSRCCOLOR = 0x301,
	D3DBLEND_SRCALPHA = 0x302,
	D3DBLEND_INVSRCALPHA = 0x303,
	D3DBLEND_DESTALPHA = 0x304,
	D3DBLEND_INVDESTALPHA = 0x305,
	D3DBLEND_DESTCOLOR = 0x306,
	D3DBLEND_INVDESTCOLOR = 0x307,
	D3DBLEND_SRCALPHASAT = 0x308,
	D3DBLEND_CONSTANTCOLOR = 0x8001,
	D3DBLEND_INVCONSTANTCOLOR = 0x8002,
	D3DBLEND_CONSTANTALPHA = 0x8003,
	D3DBLEND_INVCONSTANTALPHA = 0x8004,
	D3DBLEND_FORCE_DWORD = 0x7FFFFFFF,
};

enum _D3DBLENDOP : __int32
{
	D3DBLENDOP_ADD = 0x8006,
	D3DBLENDOP_SUBTRACT = 0x800A,
	D3DBLENDOP_REVSUBTRACT = 0x800B,
	D3DBLENDOP_MIN = 0x8007,
	D3DBLENDOP_MAX = 0x8008,
	D3DBLENDOP_ADDSIGNED = 0xF006,
	D3DBLENDOP_REVSUBTRACTSIGNED = 0xF005,
	D3DBLENDOP_FORCE_DWORD = 0x7FFFFFFF,
};

//this seems to vary per game, likely because of the Xbox SDK version.
enum X_D3DTEXTURESTAGESTATETYPE : __int32
{
	X_D3DTSS_ADDRESSU = 0,
	X_D3DTSS_ADDRESSV = 1,
	X_D3DTSS_ADDRESSW = 2,
	X_D3DTSS_MAGFILTER = 3,
	X_D3DTSS_MINFILTER = 4,
	X_D3DTSS_MIPFILTER = 5,
	X_D3DTSS_MIPMAPLODBIAS = 6,
	X_D3DTSS_MAXMIPLEVEL = 7,
	X_D3DTSS_MAXANISOTROPY = 8,
	X_D3DTSS_COLORKEYOP = 9, // Xbox ext.
	X_D3DTSS_COLORSIGN = 10, // Xbox ext.
	X_D3DTSS_ALPHAKILL = 11, // Xbox ext.
	X_D3DTSS_COLOROP = 12,
	X_D3DTSS_COLORARG0 = 13,
	X_D3DTSS_COLORARG1 = 14,
	X_D3DTSS_COLORARG2 = 15,
	X_D3DTSS_ALPHAOP = 16,
	X_D3DTSS_ALPHAARG0 = 17,
	X_D3DTSS_ALPHAARG1 = 18,
	X_D3DTSS_ALPHAARG2 = 19,
	X_D3DTSS_RESULTARG = 20,
	X_D3DTSS_TEXTURETRANSFORMFLAGS = 21,
	// End of "deferred" texture states, continuing with the rest :
	X_D3DTSS_BUMPENVMAT00 = 22,
	X_D3DTSS_BUMPENVMAT01 = 23,
	X_D3DTSS_BUMPENVMAT11 = 24,
	X_D3DTSS_BUMPENVMAT10 = 25,
	X_D3DTSS_BUMPENVLSCALE = 26,
	X_D3DTSS_BUMPENVLOFFSET = 27,
	X_D3DTSS_TEXCOORDINDEX = 28,
	X_D3DTSS_BORDERCOLOR = 29,
	X_D3DTSS_COLORKEYCOLOR = 30, // Xbox ext.
	X_D3DTSS_UNSUPPORTED = 31, // Note : Somehow, this one comes through D3DDevice_SetTextureStageStateNotInline sometimes

	X_D3DTSS_MAX = 0x20,
	X_D3DTSS_FORCE_DWORD = 0x7FFFFFFF,
};

enum _D3DTEXTUREADDRESS : __int32
{
	D3DTADDRESS_WRAP = 0x1,
	D3DTADDRESS_MIRROR = 0x2,
	D3DTADDRESS_CLAMP = 0x3,
	D3DTADDRESS_BORDER = 0x4,
	D3DTADDRESS_CLAMPTOEDGE = 0x5,
	D3DTADDRESS_MAX = 0x6,
	D3DTADDRESS_FORCE_DWORD = 0x7FFFFFFF,
};

// vertex shader input registers for fixed function vertex shader

//          Name                   Register number      D3DFVF
const int X_D3DVSDE_VERTEX = -1; // Xbox extension for Begin/End drawing - equivalent to POSITION
const int X_D3DVSDE_POSITION = 0; // Corresponds to X_D3DFVF_XYZ
const int X_D3DVSDE_BLENDWEIGHT = 1; // Corresponds to X_D3DFVF_XYZB1? (was X_D3DFVF_XYZRHW?)
const int X_D3DVSDE_NORMAL = 2; // Corresponds to X_D3DFVF_NORMAL
const int X_D3DVSDE_DIFFUSE = 3; // Corresponds to X_D3DFVF_DIFFUSE
const int X_D3DVSDE_SPECULAR = 4; // Corresponds to X_D3DFVF_SPECULAR
const int X_D3DVSDE_FOG = 5; // Xbox extension
const int X_D3DVSDE_POINTSIZE = 6; // Dxbx addition
const int X_D3DVSDE_BACKDIFFUSE = 7; // Xbox extension
const int X_D3DVSDE_BACKSPECULAR = 8; // Xbox extension
const int X_D3DVSDE_TEXCOORD0 = 9; // Corresponds to X_D3DFVF_TEX1 (not X_D3DFVF_TEX0, which means no textures are present)
const int X_D3DVSDE_TEXCOORD1 = 10; // Corresponds to X_D3DFVF_TEX2
const int X_D3DVSDE_TEXCOORD2 = 11; // Corresponds to X_D3DFVF_TEX3
const int X_D3DVSDE_TEXCOORD3 = 12; // Corresponds to X_D3DFVF_TEX4

//typedef X_D3DVSDE = X_D3DVSDE_POSITION..High(DWORD)-2; // Unique declaration to make overloads possible;

// bit declarations for _Type fields
const int X_D3DVSDT_FLOAT1 = 0x12; // 1D float expanded to (value, 0.0, 0.0, 1.0)
const int X_D3DVSDT_FLOAT2 = 0x22; // 2D float expanded to (value, value, 0.0, 1.0)
const int X_D3DVSDT_FLOAT3 = 0x32; // 3D float expanded to (value, value, value, 1.0) In double word format this is ARGB, or in byte ordering it would be B, G, R, A.
const int X_D3DVSDT_FLOAT4 = 0x42; // 4D float
const int X_D3DVSDT_D3DCOLOR = 0x40; // 4D packed unsigned bytes mapped to 0.0 to 1.0 range
//const int X_D3DVSDT_UBYTE4      = 0x05; // 4D unsigned byte   Dxbx note : Not supported on Xbox ?
const int X_D3DVSDT_SHORT2 = 0x25; // 2D signed short expanded to (value, value, 0.0, 1.0)
const int X_D3DVSDT_SHORT4 = 0x45; // 4D signed short

//  Xbox only declarations :
const int X_D3DVSDT_NORMSHORT1 = 0x11; // xbox ext. 1D signed, normalized short expanded to (value, 0.0, 0.0, 1.0). Signed, normalized shorts map from -1.0 to 1.0.
const int X_D3DVSDT_NORMSHORT2 = 0x21; // xbox ext. 2D signed, normalized short expanded to (value, value, 0.0, 1.0). Signed, normalized shorts map from -1.0 to 1.0.
const int X_D3DVSDT_NORMSHORT3 = 0x31; // xbox ext. 3D signed, normalized short expanded to (value, value, value, 1.0). Signed, normalized shorts map from -1.0 to 1.0.
const int X_D3DVSDT_NORMSHORT4 = 0x41; // xbox ext. 4D signed, normalized short expanded to (value, value, value, value). Signed, normalized shorts map from -1.0 to 1.0.
const int X_D3DVSDT_NORMPACKED3 = 0x16; // xbox ext. Three signed, normalized components packed in 32-bits. (11,11,10). Each component ranges from -1.0 to 1.0. Expanded to (value, value, value, 1.0).
const int X_D3DVSDT_SHORT1 = 0x15; // xbox ext. 1D signed short expanded to (value, 0., 0., 1). Signed shorts map to the range [-32768, 32767].
const int X_D3DVSDT_SHORT3 = 0x35; // xbox ext. 3D signed short expanded to (value, value, value, 1). Signed shorts map to the range [-32768, 32767].
const int X_D3DVSDT_PBYTE1 = 0x14; // xbox ext. 1D packed byte expanded to (value, 0., 0., 1). Packed bytes map to the range [0, 1].
const int X_D3DVSDT_PBYTE2 = 0x24; // xbox ext. 2D packed byte expanded to (value, value, 0., 1). Packed bytes map to the range [0, 1].
const int X_D3DVSDT_PBYTE3 = 0x34; // xbox ext. 3D packed byte expanded to (value, value, value, 1). Packed bytes map to the range [0, 1].
const int X_D3DVSDT_PBYTE4 = 0x44; // xbox ext. 4D packed byte expanded to (value, value, value, value). Packed bytes map to the range [0, 1].
const int X_D3DVSDT_FLOAT2H = 0x72; // xbox ext. 3D float that expands to (value, value, 0.0, value). Useful for projective texture coordinates.
const int X_D3DVSDT_NONE = 0x02; // xbox ext. nsp

//
// DeclTypes
//
#define D3DDTCAPS_UBYTE4     0x00000001L
#define D3DDTCAPS_UBYTE4N    0x00000002L
#define D3DDTCAPS_SHORT2N    0x00000004L
#define D3DDTCAPS_SHORT4N    0x00000008L
#define D3DDTCAPS_USHORT2N   0x00000010L
#define D3DDTCAPS_USHORT4N   0x00000020L
#define D3DDTCAPS_UDEC3      0x00000040L
#define D3DDTCAPS_DEC3N      0x00000080L
#define D3DDTCAPS_FLOAT16_2  0x00000100L
#define D3DDTCAPS_FLOAT16_4  0x00000200L

//maybe?
typedef enum _D3DDECLTYPE
{
	D3DDECLTYPE_FLOAT1 = 0,  // 1D float expanded to (value, 0., 0., 1.)
	D3DDECLTYPE_FLOAT2 = 1,  // 2D float expanded to (value, value, 0., 1.)
	D3DDECLTYPE_FLOAT3 = 2,  // 3D float expanded to (value, value, value, 1.)
	D3DDECLTYPE_FLOAT4 = 3,  // 4D float
	D3DDECLTYPE_D3DCOLOR = 4,  // 4D packed unsigned bytes mapped to 0. to 1. range
	// Input is in D3DCOLOR format (ARGB) expanded to (R, G, B, A)
	D3DDECLTYPE_UBYTE4 = 5,  // 4D unsigned byte
	D3DDECLTYPE_SHORT2 = 6,  // 2D signed short expanded to (value, value, 0., 1.)
	D3DDECLTYPE_SHORT4 = 7,  // 4D signed short

	// The following types are valid only with vertex shaders >= 2.0


	D3DDECLTYPE_UBYTE4N = 8,  // Each of 4 bytes is normalized by dividing to 255.0
	D3DDECLTYPE_SHORT2N = 9,  // 2D signed short normalized (v[0]/32767.0,v[1]/32767.0,0,1)
	D3DDECLTYPE_SHORT4N = 10,  // 4D signed short normalized (v[0]/32767.0,v[1]/32767.0,v[2]/32767.0,v[3]/32767.0)
	D3DDECLTYPE_USHORT2N = 11,  // 2D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,0,1)
	D3DDECLTYPE_USHORT4N = 12,  // 4D unsigned short normalized (v[0]/65535.0,v[1]/65535.0,v[2]/65535.0,v[3]/65535.0)
	D3DDECLTYPE_UDEC3 = 13,  // 3D unsigned 10 10 10 format expanded to (value, value, value, 1)
	D3DDECLTYPE_DEC3N = 14,  // 3D signed 10 10 10 format normalized and expanded to (v[0]/511.0, v[1]/511.0, v[2]/511.0, 1)
	D3DDECLTYPE_FLOAT16_2 = 15,  // Two 16-bit floating point values, expanded to (value, value, 0, 1)
	D3DDECLTYPE_FLOAT16_4 = 16,  // Four 16-bit floating point values
	D3DDECLTYPE_UNUSED = 17,  // When the type field in a decl is unused.
} D3DDECLTYPE;

typedef enum _D3DDECLUSAGE
{
	D3DDECLUSAGE_POSITION = 0,
	D3DDECLUSAGE_BLENDWEIGHT,   // 1
	D3DDECLUSAGE_BLENDINDICES,  // 2
	D3DDECLUSAGE_NORMAL,        // 3
	D3DDECLUSAGE_PSIZE,         // 4
	D3DDECLUSAGE_TEXCOORD,      // 5
	D3DDECLUSAGE_TANGENT,       // 6
	D3DDECLUSAGE_BINORMAL,      // 7
	D3DDECLUSAGE_TESSFACTOR,    // 8
	D3DDECLUSAGE_POSITIONT,     // 9
	D3DDECLUSAGE_COLOR,         // 10
	D3DDECLUSAGE_FOG,           // 11
	D3DDECLUSAGE_DEPTH,         // 12
	D3DDECLUSAGE_SAMPLE,        // 13
} D3DDECLUSAGE;

typedef enum _D3DDECLMETHOD
{
	D3DDECLMETHOD_DEFAULT = 0,
	D3DDECLMETHOD_PARTIALU,
	D3DDECLMETHOD_PARTIALV,
	D3DDECLMETHOD_CROSSUV,    // Normal
	D3DDECLMETHOD_UV,
	D3DDECLMETHOD_LOOKUP,               // Lookup a displacement map
	D3DDECLMETHOD_LOOKUPPRESAMPLED,     // Lookup a pre-sampled displacement map
} D3DDECLMETHOD;

// X_D3DVertexShader.Flags values :
#define X_VERTEXSHADER_FLAG_WRITE           (1 <<  0) // = 0x0001 // Set for Xbox ShaderType != X_VST_NORMAL 
#define X_VERTEXSHADER_FLAG_PASSTHROUGH     (1 <<  1) // = 0x0002
#define X_VERTEXSHADER_FLAG_STATE           (1 <<  3) // = 0x0008 // Set for Xbox ShaderType == X_VST_STATE
#define X_VERTEXSHADER_FLAG_PROGRAM         (1 <<  4) // = 0x0010 // Set when X_D3DVertexShader was created with assigned function data; introduced after XDK 3948; don't use directly, use g_X_VERTEXSHADER_FLAG_PROGRAM instead
#define X_VERTEXSHADER_FLAG_HASDIFFUSE      (1 << 10) // = 0x0400 Corresponds to X_D3DUSAGE_PERSISTENTDIFFUSE
#define X_VERTEXSHADER_FLAG_HASSPECULAR     (1 << 11) // = 0x0800 Corresponds to X_D3DUSAGE_PERSISTENTSPECULAR
#define X_VERTEXSHADER_FLAG_HASBACKDIFFUSE  (1 << 12) // = 0x1000 Corresponds to X_D3DUSAGE_PERSISTENTBACKDIFFUSE
#define X_VERTEXSHADER_FLAG_HASBACKSPECULAR (1 << 13) // = 0x2000 Corresponds to X_D3DUSAGE_PERSISTENTBACKSPECULAR

#define X_D3DFVF_TEXCOORDSIZE_SHIFT(Index) ((Index) * 2 + 16)


/* Flags to construct D3DRS_COLORWRITEENABLE */
#define D3DCOLORWRITEENABLE_RED     (1L<<0)
#define D3DCOLORWRITEENABLE_GREEN   (1L<<1)
#define D3DCOLORWRITEENABLE_BLUE    (1L<<2)
#define D3DCOLORWRITEENABLE_ALPHA   (1L<<3)

/*
 * Values for COLORARG0,1,2, ALPHAARG0,1,2, and RESULTARG texture blending
 * operations set in texture processing stage controls in D3DRENDERSTATE.
 */
#define D3DTA_SELECTMASK        0x0000000f  // mask for arg selector
#define D3DTA_DIFFUSE           0x00000000  // select diffuse color (read only)
#define D3DTA_CURRENT           0x00000001  // select stage destination register (read/write)
#define D3DTA_TEXTURE           0x00000002  // select texture color (read only)
#define D3DTA_TFACTOR           0x00000003  // select D3DRS_TEXTUREFACTOR (read only)
#define D3DTA_SPECULAR          0x00000004  // select specular color (read only)
#define D3DTA_TEMP              0x00000005  // select temporary register color (read/write)
#define D3DTA_COMPLEMENT        0x00000010  // take 1.0 - x (read modifier)
#define D3DTA_ALPHAREPLICATE    0x00000020  // replicate alpha to color components (read modifier)

#define X_D3D_RENDER_MEMORY_ALIGNMENT     64
#define X_D3DSURFACE_ALIGNMENT            X_D3D_RENDER_MEMORY_ALIGNMENT
#define X_D3DTEXTURE_ALIGNMENT            (2 * X_D3D_RENDER_MEMORY_ALIGNMENT)
#define X_D3DTEXTURE_CUBEFACE_ALIGNMENT   (2 * X_D3D_RENDER_MEMORY_ALIGNMENT)
#define X_D3DTEXTURE_PITCH_ALIGNMENT      X_D3D_RENDER_MEMORY_ALIGNMENT
#define X_D3DTEXTURE_PITCH_MIN            X_D3DTEXTURE_PITCH_ALIGNMENT

 // pixel container "size" masks
 // The layout of the size field, used for non swizzled or compressed textures.
 //
 // The Size field of a container will be zero if the texture is swizzled or compressed.
 // It is guarenteed to be non-zero otherwise because either the height/width will be
 // greater than one or the pitch adjust will be nonzero because the minimum texture
 // pitch is 8 bytes.
#define X_D3DSIZE_WIDTH_MASK              0x00000FFF   // Width of the texture - 1, in texels
//#define X_D3DSIZE_WIDTH_SHIFT           0
#define X_D3DSIZE_HEIGHT_MASK             0x00FFF000   // Height of the texture - 1, in texels
#define X_D3DSIZE_HEIGHT_SHIFT            12
#define X_D3DSIZE_PITCH_MASK              0xFF000000   // Pitch / 64 - 1
#define X_D3DSIZE_PITCH_SHIFT             24


typedef enum _D3DCUBEMAP_FACES
{
	D3DCUBEMAP_FACE_POSITIVE_X = 0,
	D3DCUBEMAP_FACE_NEGATIVE_X = 1,
	D3DCUBEMAP_FACE_POSITIVE_Y = 2,
	D3DCUBEMAP_FACE_NEGATIVE_Y = 3,
	D3DCUBEMAP_FACE_POSITIVE_Z = 4,
	D3DCUBEMAP_FACE_NEGATIVE_Z = 5,

	D3DCUBEMAP_FACE_FORCE_DWORD = 0x7fffffff
} D3DCUBEMAP_FACES;

typedef enum _D3DTEXTURETRANSFORMFLAGS {
	D3DTTFF_DISABLE = 0,    // texture coordinates are passed directly
	D3DTTFF_COUNT1 = 1,    // rasterizer should expect 1-D texture coords
	D3DTTFF_COUNT2 = 2,    // rasterizer should expect 2-D texture coords
	D3DTTFF_COUNT3 = 3,    // rasterizer should expect 3-D texture coords
	D3DTTFF_COUNT4 = 4,    // rasterizer should expect 4-D texture coords
	D3DTTFF_PROJECTED = 256,  // texcoords to be divided by COUNTth element
	D3DTTFF_FORCE_DWORD = 0x7fffffff,
} D3DTEXTURETRANSFORMFLAGS;

typedef float RwReal;
struct RwD3D8Vertex
{
	RwReal x;
	RwReal y;
	RwReal z;
	RwReal rhw;
	uint32_t emissiveColor;
	RwReal u;
	RwReal v;
};

// Xbox vertex shader constant modes
#define X_D3DSCM_96CONSTANTS                  0x00 // Enables constants 0..95
#define X_D3DSCM_192CONSTANTS                 0x01 // Enables constants -96..-1 on top of 0..95
#define X_D3DSCM_192CONSTANTSANDFIXEDPIPELINE 0x02 // Unsupported?
#define X_D3DSCM_NORESERVEDCONSTANTS          0x10  // Do not reserve constant -38 and -37

#define X_D3DSCM_RESERVED_CONSTANT_SCALE -38 // Becomes 58 after correction, contains Scale v
#define X_D3DSCM_RESERVED_CONSTANT_OFFSET -37 // Becomes 59 after correction, contains Offset

#define X_D3DSCM_CORRECTION                 96 // Add 96 to arrive at the range 0..191 (instead of -96..95)
#define X_D3DVS_CONSTREG_COUNT              192

// Special Registers, used to pass additional information to the shaders
const uint16_t HOST_D3DVS_CONSTREG_VREGDEFAULTS_BASE = X_D3DVS_CONSTREG_COUNT;
const uint16_t HOST_D3DVS_CONSTREG_VREGDEFAULTS_SIZE = 16u;
#define HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_BASE (HOST_D3DVS_CONSTREG_VREGDEFAULTS_BASE + HOST_D3DVS_CONSTREG_VREGDEFAULTS_SIZE)
#define HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_SIZE 4
#define HOST_D3DVS_SCREENSPACE_SCALE_BASE     (HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_BASE + HOST_D3DVS_CONSTREG_VREGDEFAULTS_FLAG_SIZE)
#define HOST_D3DVS_NORMALIZE_SCALE_SIZE     1
#define HOST_D3DVS_CONSTREG_FULL  (X_D3DVS_CONSTREG_COUNT + 26 + 4)

#define HOST_D3DVS_SCREENSPACE_OFFSET_BASE     (HOST_D3DVS_SCREENSPACE_SCALE_BASE + HOST_D3DVS_NORMALIZE_SCALE_SIZE)

#define HOST_D3DVS_NORMALIZE_OFFSET_SIZE     1
#define HOST_D3DVS_TEXTURES_SCALE_BASE             (HOST_D3DVS_SCREENSPACE_OFFSET_BASE + HOST_D3DVS_NORMALIZE_OFFSET_SIZE)
#define HOST_D3DVS_TEXTURES_SCALE_SIZE             4
#define HOST_D3DVS_CONSTREG_FOGINFO                (HOST_D3DVS_TEXTURES_SCALE_BASE + HOST_D3DVS_TEXTURES_SCALE_SIZE)

#define X_D3DSCM_RESERVED_CONSTANT_SCALE_CORRECTED (X_D3DSCM_RESERVED_CONSTANT_SCALE + X_D3DSCM_CORRECTION)
#define X_D3DSCM_RESERVED_CONSTANT_OFFSET_CORRECTED (X_D3DSCM_RESERVED_CONSTANT_OFFSET + X_D3DSCM_CORRECTION)