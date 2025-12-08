#include "video.h"
#include "vertexdeclaration.h"
#include <xxHashMap.h>
#include <kernel/heap.h>
#include <algorithm>
#include <DirectXMath.h>

#ifndef D3DCOLORVALUE_DEFINED
typedef struct _D3DCOLORVALUE {
	float r;
	float g;
	float b;
	float a;
} D3DCOLORVALUE;
#define D3DCOLORVALUE_DEFINED
#endif

//original code (mostly) re used from CXBX Reloaded


extern uint32_t g_renderState[255];

// Combines world/view matrices
static std::array<D3DMATRIX, 4> g_WorldView;
// World/view inverse transpose for lighting calculations
static std::array<D3DMATRIX, 4> g_WorldViewInverseTranspose;

std::array<bool, 4> bWorldViewDirty;

struct Matrix4x4
{
	float m[4][4];

	Matrix4x4 Transpose() const {
		Matrix4x4 result;
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				result.m[i][j] = m[j][i];
			}
		}
		return result;
	}
};

namespace FixedFunctionVertexShader {
	// Fog depth is taken from the vertex, rather than generated
	const float FOG_DEPTH_NONE = 0;
	// Fog depth is the output Z coordinate
	const float FOG_DEPTH_Z = 1;
	// Fog depth is based on the output W coordinate (1 / W)
	const float FOG_DEPTH_W = 2;
	// Fog depth is based distance of the vertex from the eye position
	const float FOG_DEPTH_RANGE = 3;

	// https://docs.microsoft.com/en-us/windows/win32/direct3d9/fog-formulas
	const float FOG_TABLE_NONE = 0;
	const float FOG_TABLE_EXP = 1;
	const float FOG_TABLE_EXP2 = 2;
	const float FOG_TABLE_LINEAR = 3;
}

typedef enum _D3DFOGMODE {
	D3DFOG_NONE = 0,
	D3DFOG_EXP = 1,
	D3DFOG_EXP2 = 2,
	D3DFOG_LINEAR = 3,
	D3DFOG_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DFOGMODE;

typedef struct _D3DVECTOR {
	float x;
	float y;
	float z;
} D3DVECTOR;

typedef struct _D3DVECTOR4 {
	float x;
	float y;
	float z;
	float w;
} D3DVECTOR4;


struct Transforms
{
	alignas(16) D3DMATRIX View;
	alignas(16) D3DMATRIX Projection;
	alignas(16) D3DMATRIX Texture[4]; // c8-c23 (4 matrices × 4 regs each)
	alignas(16) D3DMATRIX WorldView[4];
	alignas(16) D3DMATRIX WorldViewInverseTranspose[4];
};

struct Light {

	alignas(16) D3DCOLORVALUE Diffuse;
	alignas(16) D3DCOLORVALUE Specular;

	// Viewspace light position Range in posW
	alignas(16) D3DVECTOR4 PositionV;
	// alignas(16) float Range;

	 // Viewspace light direction (normalized) - type in directionW
	alignas(16) D3DVECTOR4 DirectionVN;
	// alignas(16) float Type; // 1=Point, 2=Spot, 3=Directional

	alignas(16) D3DVECTOR4 Attenuation; //fall off in W
	//alignas(16) float Falloff;

	alignas(16) D3DVECTOR4 SpotParams; // cN+5: x=CosHalfPhi, y=SpotIntensityDivisor, zw unused
};



// Vertex lighting
// Both frontface and backface lighting can be calculated
struct TwoSidedColor
{
	alignas(16) D3DVECTOR4 Front; // c104.xyz
	alignas(16) D3DVECTOR4 Back; // c105.xyz
};

struct Material
{
	alignas(16) D3DCOLORVALUE Diffuse; // base for Materials[0], these will be array-offsetted
	alignas(16) D3DCOLORVALUE Ambient;
	alignas(16) D3DCOLORVALUE Specular;
	alignas(16) D3DCOLORVALUE Emissive;
	alignas(16) D3DCOLORVALUE Power; // c110.w/y/z free
};

struct Modes
{
	// c116
	float AmbientMaterialSource;
	float DiffuseMaterialSource;
	float SpecularMaterialSource;
	float EmissiveMaterialSource;

	// c117
	float BackAmbientMaterialSource;
	float BackDiffuseMaterialSource;
	float BackSpecularMaterialSource;
	float BackEmissiveMaterialSource;

	// c118
	float Lighting;
	float TwoSidedLighting;
	// float SpecularEnable; 
	float LocalViewer;
	// c118.w unused

	// c119
	// float ColorVertex;
	float VertexBlend_NrOfMatrices;
	float VertexBlend_CalcLastWeight;
	float NormalizeNormals;
	// c119.w unused
};

struct Fog
{
	// c120
	float Enable;
	float DepthMode;
	float TableMode;
	float Density; // EXP density
	// c121
	float Start; // LINEAR start
	float End; // LINEAR end
};

struct TextureState
{
	float TextureTransformFlagsCount;
	float TextureTransformFlagsProjected;
	float TexCoordIndex;
	float TexCoordIndexGen;
};

struct PointSprite
{
	// c126
	float PointSize;
	float PointSize_Min;
	float PointSize_Max;
	//  float PointScaleEnable;
	float XboxRenderTargetHeight;
	// c127
	D3DVECTOR4 PointScaleABC; //xyz PointScaleABC + RenderUpscaleFactor in .w
	//alignas(16) float RenderUpscaleFactor;
};

struct FixedFunctionVertexShaderState
{
	alignas(16) Transforms Transforms;
	alignas(16) Light Lights[8];
	alignas(16) TwoSidedColor TotalLightsAmbient; // c104 - c105
	alignas(16) Material Materials[2]; // c106 - c115
	alignas(16) Modes Modes; // c116 - c119
	alignas(16) Fog Fog; // c120 - c121
	alignas(16) TextureState TextureStates[4]; // c122 - c125;
	alignas(16) PointSprite PointSprite; // c126 - c128
	alignas(16) float TexCoordComponentCount[4]; // c129
};

FixedFunctionVertexShaderState ffShaderState = { 0 };
extern int32_t* g_texture_stage_states;
extern int32_t g_RenderUpscaleFactor;
extern std::array<D3DMATRIX, X_D3DTS_MAX> g_Transforms;



D3DCOLORVALUE colorValue(float r, float g, float b, float a) {
	auto value = D3DCOLORVALUE();
	value.r = r;
	value.g = g;
	value.b = b;
	value.a = a;
	return value;
}

DirectX::XMFLOAT4 toVector(DWORD color) {
	DirectX::XMFLOAT4  v;
	// ARGB to XYZW
	v.w = (color >> 24 & 0xFF) / 255.f;
	v.x = (color >> 16 & 0xFF) / 255.f;
	v.y = (color >> 8 & 0xFF) / 255.f;
	v.z = (color >> 0 & 0xFF) / 255.f;
	return v;
}

D3DVECTOR toVector(float x, float y, float z) {
	auto value = D3DVECTOR();
	value.x = x;
	value.y = y;
	value.z = z;
	return value;
}

DirectX::XMFLOAT4 toVector(D3DCOLORVALUE val) {
	return DirectX::XMFLOAT4(val.r, val.g, val.b, val.a);
}

static uint32_t GetRenderState(uint32_t State)
{
	return g_renderState[State];
}

static float GetXboxRenderStateAsFloat(uint32_t State)
{


	return *reinterpret_cast<float*>(&(g_renderState[State]));
}


void RecalculateDependentMatrices(uint32_t i)
{
	auto worldState = X_D3DTS_WORLD + i;

	using namespace DirectX;
	// Load world and view matrices
	XMMATRIX world = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[worldState]);
	XMMATRIX view = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[X_D3DTS_VIEW]);

	// Calculate WorldView = World * View
	XMMATRIX worldView = XMMatrixMultiply(world, view);
	XMStoreFloat4x4((XMFLOAT4X4*)&g_WorldView[i], worldView);

	// Calculate WorldViewInverseTranspose
	XMVECTOR det = XMMatrixDeterminant(worldView);
	XMMATRIX worldViewInverse = XMMatrixInverse(&det, worldView);
	XMMATRIX worldViewInverseTranspose = XMMatrixTranspose(worldViewInverse);
	XMStoreFloat4x4((XMFLOAT4X4*)&g_WorldViewInverseTranspose[i], worldViewInverseTranspose);
}


D3DMATRIX* GetWorldView(unsigned i)
{
	assert(i < 4);

	if (bWorldViewDirty[i])
	{
		RecalculateDependentMatrices(i);
		bWorldViewDirty[i] = false;
	}

	return &g_WorldView[i];

}

D3DMATRIX* GetWorldViewInverseTranspose(unsigned i)
{
	assert(i < 4);

	if (bWorldViewDirty[i])
	{
		RecalculateDependentMatrices(i);
		bWorldViewDirty[i] = false;
	}

	return &g_WorldViewInverseTranspose[i];
}


int GetXboxVertexDataComponentCount(int32_t d3dvsdt)
{

	switch (d3dvsdt) {
	case X_D3DVSDT_NORMPACKED3:
		return 3;
	case X_D3DVSDT_FLOAT2H:
		printf("Attempting to use component count for X_D3DVSDT_FLOAT2H, which uses an odd (value, value, 0, value) layout\n");
		// This is a bit of an odd case. Will call it 4 since it writes a value to the 4th component...
		return 4;
	default:
		// Most data types have a representation consistent with the number of components
		const int countMask = 0x7;
		const int countShift = 4;
		return (d3dvsdt >> countShift) & countMask;
	}
}


bool logIndexedPrimitive = false;
void UpdateFixedFunctionVertexShaderState(size_t Handle, bool isFixed)
{

	using namespace DirectX;
	// Vertex blending
	// Prepare vertex blending mode variables used in transforms, below
	auto VertexBlend = g_renderState[X_D3DRS_VERTEXBLEND];
	// Xbox and host D3DVERTEXBLENDFLAGS :
	//     D3DVBF_DISABLE           = 0 : 1 matrix,   0 weights => final weight 1
	//     D3DVBF_1WEIGHTS          = 1 : 2 matrices, 1 weights => final weight calculated
	//     D3DVBF_2WEIGHTS          = 3 : 3 matrices, 2 weights => final weight calculated
	//     D3DVBF_3WEIGHTS          = 5 : 4 matrices, 3 weights => final weight calculated
	// Xbox X_D3DVERTEXBLENDFLAGS :
	//   X_D3DVBF_2WEIGHTS2MATRICES = 2 : 2 matrices, 2 weights
	//   X_D3DVBF_3WEIGHTS3MATRICES = 4 : 3 matrices, 3 weights
	//   X_D3DVBF_4WEIGHTS4MATRICES = 6 : 4 matrices, 4 weights
	//
	if (VertexBlend > X_D3DVBF_4WEIGHTS4MATRICES)
		printf("X_D3DRS_VERTEXBLEND out of range\n");

	// Calculate the number of matrices, by adding the LSB to turn (0,1,3,5) and (0,2,4,6) into (0,2,4,6); Then divide by 2 to get (0,1,2,3), and add 1 to get 1, 2, 3 or 4 matrices :
	auto NrBlendMatrices = ((VertexBlend + (VertexBlend & 1)) / 2) + 1;
	// Looking at the above values, 0 or the LSB of VertexBlend signals that the final weight needs to be calculated from all previous weigths (deducting them all from an initial 1) :
	auto CalcLastBlendWeight = (VertexBlend == X_D3DVBF_DISABLE) || (VertexBlend & 1);
	// Copy the resulting values over to shader state :
	ffShaderState.Modes.VertexBlend_NrOfMatrices = (float)NrBlendMatrices;
	ffShaderState.Modes.VertexBlend_CalcLastWeight = (float)CalcLastBlendWeight;

	// Transforms
	// Transpose row major to column major for HLSL
	XMMATRIX projection = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[X_D3DTS_PROJECTION]); //first we read the projection matrix
	XMMATRIX projectionT = XMMatrixTranspose(projection); //then we transpose it
	XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.Projection, projectionT); //finally we store it (overwrite previous data if any)

	XMMATRIX view = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[X_D3DTS_VIEW]);
	XMMATRIX viewT = XMMatrixTranspose(view);
	XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.View, viewT);

	for (unsigned i = 0; i < 4; i++)
	{ // TODO : Would it help to limit this to just the active texture channels?

		XMMATRIX transfoTexture = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[X_D3DTS_TEXTURE0 + i]);
		XMMATRIX transfoTextureT = XMMatrixTranspose(transfoTexture);
		XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.Texture[i], transfoTextureT);
	}

	for (unsigned i = 0; i < ffShaderState.Modes.VertexBlend_NrOfMatrices; i++)
	{

		XMMATRIX transfoWorldView = XMLoadFloat4x4((XMFLOAT4X4*)GetWorldView(i));
		XMMATRIX transfoWorldViewT = XMMatrixTranspose(transfoWorldView);
		XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.WorldView[i], transfoWorldViewT);

		XMMATRIX WorldViewInverseTranspose = XMLoadFloat4x4((XMFLOAT4X4*)GetWorldViewInverseTranspose(i));
		XMMATRIX WorldViewInverseTransposeT = XMMatrixTranspose(WorldViewInverseTranspose);
		XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.WorldViewInverseTranspose[i], WorldViewInverseTransposeT);
	}

	// Lighting
	// Point sprites aren't lit - 'each point is always rendered with constant colors.'
	// https://docs.microsoft.com/en-us/windows/win32/direct3d9/point-sprites
	bool PointSpriteEnable = g_renderState[X_D3DRS_POINTSPRITEENABLE];
	bool LightingEnable = g_renderState[X_D3DRS_LIGHTING];
	ffShaderState.Modes.Lighting = LightingEnable && !PointSpriteEnable;

	ffShaderState.Modes.TwoSidedLighting = (float)GetRenderState(X_D3DRS_TWOSIDEDLIGHTING);
	ffShaderState.Modes.LocalViewer = (float)GetRenderState(X_D3DRS_LOCALVIEWER);

	// Material sources
	bool ColorVertex = GetRenderState(X_D3DRS_COLORVERTEX) != FALSE;
	ffShaderState.Modes.AmbientMaterialSource = (float)(ColorVertex ? GetRenderState(X_D3DRS_AMBIENTMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.DiffuseMaterialSource = (float)(ColorVertex ? GetRenderState(X_D3DRS_DIFFUSEMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.SpecularMaterialSource = (float)(ColorVertex ? GetRenderState(X_D3DRS_SPECULARMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.EmissiveMaterialSource = (float)(ColorVertex ? GetRenderState(X_D3DRS_EMISSIVEMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.BackAmbientMaterialSource = (float)(ColorVertex ? GetRenderState(X_D3DRS_BACKAMBIENTMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.BackDiffuseMaterialSource = (float)(ColorVertex ? GetRenderState(X_D3DRS_BACKDIFFUSEMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.BackSpecularMaterialSource = (float)(ColorVertex ? GetRenderState(X_D3DRS_BACKSPECULARMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.BackEmissiveMaterialSource = (float)(ColorVertex ? GetRenderState(X_D3DRS_BACKEMISSIVEMATERIALSOURCE) : D3DMCS_MATERIAL);

	// Point sprites; Fetch required variables
	float pointSize = GetXboxRenderStateAsFloat(X_D3DRS_POINTSIZE);
	float pointSize_Min = GetXboxRenderStateAsFloat(X_D3DRS_POINTSIZE_MIN);
	float pointSize_Max = GetXboxRenderStateAsFloat(X_D3DRS_POINTSIZE_MAX);
	bool PointScaleEnable = GetRenderState(X_D3DRS_POINTSCALEENABLE);
	float pointScale_A = GetXboxRenderStateAsFloat(X_D3DRS_POINTSCALE_A);
	float pointScale_B = GetXboxRenderStateAsFloat(X_D3DRS_POINTSCALE_B);
	float pointScale_C = GetXboxRenderStateAsFloat(X_D3DRS_POINTSCALE_C);
	float renderTargetHeight = GetRenderTargetHeight();
	// Make sure to disable point scaling when point sprites are not enabled
	PointScaleEnable &= PointSpriteEnable;
	// Set variables in shader state
	ffShaderState.PointSprite.PointSize = PointSpriteEnable ? pointSize : 1.0f;
	ffShaderState.PointSprite.PointSize_Min = PointSpriteEnable ? pointSize_Min : 1.0f;
	ffShaderState.PointSprite.PointSize_Max = PointSpriteEnable ? pointSize_Max : 1.0f;
	ffShaderState.PointSprite.PointScaleABC.x = PointScaleEnable ? pointScale_A : 1.0f;
	ffShaderState.PointSprite.PointScaleABC.y = PointScaleEnable ? pointScale_B : 0.0f;
	ffShaderState.PointSprite.PointScaleABC.z = PointScaleEnable ? pointScale_C : 0.0f;
	ffShaderState.PointSprite.XboxRenderTargetHeight = PointScaleEnable ? renderTargetHeight : 1.0f;
	ffShaderState.PointSprite.PointScaleABC.w = g_RenderUpscaleFactor; //renderUpscale in in pos w

	// Fog
	// Determine how the fog depth is transformed into the fog factor
	auto fogEnable = GetRenderState(X_D3DRS_FOGENABLE);
	auto fogTableMode = GetRenderState(X_D3DRS_FOGTABLEMODE);
	ffShaderState.Fog.Enable = fogEnable;
	// FIXME remove when fixed function PS is implemented
	// Note if we are using the fixed function pixel shader
	// We only want to produce the fog depth value in the VS, not the fog factor
	ffShaderState.Fog.TableMode = !isFixed ? D3DFOG_NONE : fogTableMode;

	// Determine how fog depth is calculated
	if (fogEnable && fogTableMode != D3DFOG_NONE)
	{
		auto proj = &ffShaderState.Transforms.Projection;

		if (GetRenderState(X_D3DRS_RANGEFOGENABLE))
		{
			printf("Using RANGE fog\n");
			ffShaderState.Fog.DepthMode = FixedFunctionVertexShader::FOG_DEPTH_RANGE;
		}
		else if (proj->_14 == 0 &&
			proj->_24 == 0 &&
			proj->_34 == 0 &&
			proj->_44 == 1) {
			printf("Using Z fog\n");
			ffShaderState.Fog.DepthMode = FixedFunctionVertexShader::FOG_DEPTH_Z;
		}
		else {
			// Fog sample

			ffShaderState.Fog.DepthMode = FixedFunctionVertexShader::FOG_DEPTH_W;
		}

		auto density = GetRenderState(X_D3DRS_FOGDENSITY);
		auto fogStart = GetRenderState(X_D3DRS_FOGSTART);
		auto fogEnd = GetRenderState(X_D3DRS_FOGEND);
		ffShaderState.Fog.Density = *reinterpret_cast<float*>(&density);
		ffShaderState.Fog.Start = *reinterpret_cast<float*>(&fogStart);
		ffShaderState.Fog.End = *reinterpret_cast<float*>(&fogEnd);
	}
	else {
		ffShaderState.Fog.DepthMode = FixedFunctionVertexShader::FOG_DEPTH_NONE;
	}

	// Texture state
	if (g_texture_stage_states)
	{
		for (int i = 0; i < X_D3DTS_STAGECOUNT; i++)
		{
			auto transformFlags = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_TEXTURETRANSFORMFLAGS];
			ffShaderState.TextureStates[i].TextureTransformFlagsCount = (float)(transformFlags & ~D3DTTFF_PROJECTED);
			ffShaderState.TextureStates[i].TextureTransformFlagsProjected = (float)(transformFlags & D3DTTFF_PROJECTED);

			auto texCoordIndex = (uint32_t)g_texture_stage_states[i * X_D3DTSS_MAX + X_D3DTSS_TEXCOORDINDEX];
			ffShaderState.TextureStates[i].TexCoordIndex = (float)(texCoordIndex & 0x7); // 8 coords
			ffShaderState.TextureStates[i].TexCoordIndexGen = (float)(texCoordIndex >> 16); // D3DTSS_TCI flags
		}
	}

	// Read current TexCoord component counts
	X_VERTEXATTRIBUTEFORMAT* pXboxVertexAttributeFormat = GetXboxVertexAttributeFormat(Handle);
	// Note : There seem to be other ways to access this, but we can use only this one;
	// This, because HostGetVertexDeclaration() can't be used, since it doesn't track VertexAttributes
	// (plus, it contains the overhead of shader lookup).
	for (int i = 0; i < X_D3DTS_STAGECOUNT; i++)
	{
		auto vertexDataFormat = pXboxVertexAttributeFormat->Slots[X_D3DVSDE_TEXCOORD0 + i].Format;
		ffShaderState.TexCoordComponentCount[i] = (float)GetXboxVertexDataComponentCount(vertexDataFormat);

	}

	// Update lights (TODO)
	/**auto LightAmbient = D3DXVECTOR4(0.f, 0.f, 0.f, 0.f);
	for (size_t i = 0; i < 8; i++)
	{
		UpdateFixedFunctionShaderLight(d3d8LightState.EnabledLights[i], &ffShaderState.Lights[i], &LightAmbient);
	}

	D3DXVECTOR4 Ambient = toVector(GetRenderState(X_D3DRS_AMBIENT));
	D3DXVECTOR4 BackAmbient = toVector(GetRenderState(X_D3DRS_BACKAMBIENT));

	ffShaderState.TotalLightsAmbient.Front = (D3DXVECTOR3)(LightAmbient + Ambient);
	ffShaderState.TotalLightsAmbient.Back = (D3DXVECTOR3)(LightAmbient + BackAmbient);*/


	// Misc flags
	ffShaderState.Modes.NormalizeNormals = (float)GetRenderState(X_D3DRS_NORMALIZENORMALS);

	// Write fixed function state to shader constants
	const int slotSize = 16;
	const int fixedFunctionStateSize = (sizeof(FixedFunctionVertexShaderState) + slotSize - 1) / slotSize;
	CopyVertexShaderConstant(0, (DWORD*)&ffShaderState, fixedFunctionStateSize);

}


//shader debug

#include <iostream>
struct float4 {
	float x, y, z, w;

	float4() : x(0), y(0), z(0), w(0) {}
	float4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
	float4(float val) : x(val), y(val), z(val), w(val) {}

	float& operator[](int index) {
		switch (index) {
		case 0: return x;
		case 1: return y;
		case 2: return z;
		case 3: return w;
		default: assert(false); return x;
		}
	}
};


struct int1 {
	int x;
	int1(int val = 0) : x(val) {}
};

// Helper functions matching the shader macros
float4 _tof4(float src) { return float4(src, src, src, src); }
float4 _tof4(const float4& src) { return src; }
float4 _ssss(float s) { return float4(s, s, s, s); }
float _scalar(const float4& src) { return src.x; }

float4 x_floor(const float4& src) {
	return float4(std::floor(src.x), std::floor(src.y), std::floor(src.z), std::floor(src.w));
}

// Shader instruction implementations
void x_arl(int1& dest, char mask, const float4& src0) {
	float4 floored = x_floor(_tof4(src0));
	if (strchr(&mask, 'x')) dest.x = (int)floored.x;
	// Add other mask components if needed
}

void x_mov(float4& dest, const char* mask, const float4& src0) {
	float4 src = _tof4(src0);
	for (int i = 0; i < strlen(mask); i++) {
		switch (mask[i]) {
		case 'x': dest.x = src.x; break;
		case 'y': dest.y = src.y; break;
		case 'z': dest.z = src.z; break;
		case 'w': dest.w = src.w; break;
		}
	}
}

void x_mul(float4& dest, const char* mask, const float4& src0, const float4& src1) {
	float4 result;
	float4 s0 = _tof4(src0);
	float4 s1 = _tof4(src1);
	result.x = s0.x * s1.x;
	result.y = s0.y * s1.y;
	result.z = s0.z * s1.z;
	result.w = s0.w * s1.w;

	for (int i = 0; i < strlen(mask); i++) {
		switch (mask[i]) {
		case 'x': dest.x = result.x; break;
		case 'y': dest.y = result.y; break;
		case 'z': dest.z = result.z; break;
		case 'w': dest.w = result.w; break;
		}
	}
}

void x_mad(float4& dest, const char* mask, const float4& src0, const float4& src1, const float4& src2) {
	float4 result;
	float4 s0 = _tof4(src0);
	float4 s1 = _tof4(src1);
	float4 s2 = _tof4(src2);
	result.x = s0.x * s1.x + s2.x;
	result.y = s0.y * s1.y + s2.y;
	result.z = s0.z * s1.z + s2.z;
	result.w = s0.w * s1.w + s2.w;

	for (int i = 0; i < strlen(mask); i++) {
		switch (mask[i]) {
		case 'x': dest.x = result.x; break;
		case 'y': dest.y = result.y; break;
		case 'z': dest.z = result.z; break;
		case 'w': dest.w = result.w; break;
		}
	}
}

void x_dp3(float4& dest, const char* mask, const float4& src0, const float4& src1) {
	float4 s0 = _tof4(src0);
	float4 s1 = _tof4(src1);
	float dot_result = s0.x * s1.x + s0.y * s1.y + s0.z * s1.z;
	float4 result = _ssss(dot_result);

	for (int i = 0; i < strlen(mask); i++) {
		switch (mask[i]) {
		case 'x': 
			dest.x = result.x; 
			break;
		case 'y': 
			dest.y = result.y;
			break;
		case 'z': 
			dest.z = result.z; 
			break;
		case 'w': 
			dest.w = result.w; 
			break;
		}
	}
}

void x_dp4(float4& dest, const char* mask, const float4& src0, const float4& src1) {
	float4 s0 = _tof4(src0);
	float4 s1 = _tof4(src1);
	float dot_result = s0.x * s1.x + s0.y * s1.y + s0.z * s1.z + s0.w * s1.w;
	float4 result = _ssss(dot_result);

	for (int i = 0; i < strlen(mask); i++) {
		switch (mask[i]) {
		case 'x': dest.x = result.x; break;
		case 'y': dest.y = result.y; break;
		case 'z': dest.z = result.z; break;
		case 'w': dest.w = result.w; break;
		}
	}
}

float _rsq(float src) {
	float a = std::abs(src);
	std::cout << "[RSQ] Input: " << src << ", abs: " << a << std::endl;

	// TODO: Enable special cases if needed
	// if (a == 1.0f) return 1.0f;
	// if (a == 0.0f) return std::numeric_limits<float>::infinity();

	float result = 1.0f / std::sqrt(a);
	std::cout << "[RSQ] Result: " << result << std::endl;
	return result;
}

void x_rsq(float4& dest, const char* mask, const float4& src0) {
	float scalar_src = _scalar(src0);
	float4 result = _ssss(_rsq(scalar_src));

	for (int i = 0; i < strlen(mask); i++) {
		switch (mask[i]) {
		case 'x': dest.x = result.x; break;
		case 'y': dest.y = result.y; break;
		case 'z': dest.z = result.z; break;
		case 'w': dest.w = result.w; break;
		}
	}
}


// Linear interpolation function (matches HLSL lerp)
float4 lerp(const float4& a, const float4& b, float t) {
	return float4(
		a.x * (1 - t) + b.x * t,
		a.y * (1 - t) + b.y * t,
		a.z * (1 - t) + b.z * t,
		a.w * (1 - t) + b.w * t
	);
}

// Debug function to print float4
void print_float4(const char* name, const float4& vec) {
	std::cout << name << ": (" << vec.x << ", " << vec.y << ", " << vec.z << ", " << vec.w << ")" << std::endl;
}

void debug_dp3(const std::string& name, const float4& src0, const float4& src1, const float4& result) {
	std::cout << "[DP3] " << name << ": dot(("
		<< src0.x << "," << src0.y << "," << src0.z << "), ("
		<< src1.x << "," << src1.y << "," << src1.z << ")) = "
		<< result.x << std::endl;
}

extern uint32_t vsh_constants[HOST_D3DVS_CONSTREG_FULL][4];
void RunDebuggingShader(uint8_t* vertexData)
{
	return;

	float4 rawVertexData[16];
	memset(rawVertexData, 0, sizeof(float4) * 16);
	float* pos = (float*)vertexData;
	uint8_t* color = vertexData + 12;
	float* uv = (float*)(vertexData + 24);

	rawVertexData[0] = { pos[0], pos[1], pos[2], 1.0f };
	rawVertexData[1] = { 1.0f, 0.0f, 0.0f, 0.0f }; //{ *reinterpret_cast<const float*>(&color[0]),  *reinterpret_cast<const float*>(&color[1]), *reinterpret_cast<const float*>(&color[2]), 1 };
	rawVertexData[2] = { uv[0], uv[1], 0.0f, 1.0f };

	// Simulate vertex shader execution
	std::vector<float4> vs_consts;
	vs_consts.reserve(192);
	std::vector<float4> vRegisterDefaultValues;
	vRegisterDefaultValues.reserve(16);
	std::vector<float> vRegisterDefaultFlags(16, 0.0f); // Assume all 0 for now

	for (int i = 0; i < 192; i++)
	{
		float4 val = {
	*reinterpret_cast<const float*>(&vsh_constants[i][0]),
	*reinterpret_cast<const float*>(&vsh_constants[i][1]),
	*reinterpret_cast<const float*>(&vsh_constants[i][2]),
	*reinterpret_cast<const float*>(&vsh_constants[i][3])
		};

		vs_consts.push_back(val);
	}

	for (int i = 192; i < 208; i++)
	{
		int index = i;
		float4 val = {
	*reinterpret_cast<const float*>(&vsh_constants[index][0]),
	*reinterpret_cast<const float*>(&vsh_constants[index][1]),
	*reinterpret_cast<const float*>(&vsh_constants[index][2]),
	*reinterpret_cast<const float*>(&vsh_constants[index][3])
		};

		vRegisterDefaultValues.push_back(val);
	}

	for (int i = 208; i < 208 + 4; i++)
	{
		int index = i;
		float4 val = {
	*reinterpret_cast<const float*>(&vsh_constants[index][0]),
	*reinterpret_cast<const float*>(&vsh_constants[index][1]),
	*reinterpret_cast<const float*>(&vsh_constants[index][2]),
	*reinterpret_cast<const float*>(&vsh_constants[index][3])
		};

		vRegisterDefaultFlags.push_back(val[0]);
		vRegisterDefaultFlags.push_back(val[1]);
		vRegisterDefaultFlags.push_back(val[2]);
		vRegisterDefaultFlags.push_back(val[3]);
	}

	// Initialize vertex registers using lerp (conditional based on flags)
	std::vector<float4> v(16);
	std::cout << "\n=== Initializing vertex registers ===" << std::endl;
	for (int i = 0; i < 16; i++)
	{
		v[i] = lerp(rawVertexData[i], vRegisterDefaultValues[i], vRegisterDefaultFlags[i]);
		std::cout << "v" << i << " flag: " << vRegisterDefaultFlags[i]
			<< " = lerp(vertex[" << i << "], default[" << i << "], flag)" << std::endl;
		print_float4("  Raw vertex", rawVertexData[i]);
		print_float4("  Default", vRegisterDefaultValues[i]);
		print_float4("  Result", v[i]);
	}

	// Shader registers
	float4 oPos, oD0, oD1, oB0, oB1, oT0, oT1, oT2, oT3;
	float4 oFog, oPts;
	int1 a0 = 0;
	float4 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11;

	// Initialize all to zero
	oPos = oD0 = oD1 = oB0 = oB1 = oT0 = oT1 = oT2 = oT3 = float4(0, 0, 0, 1);
	r0 = r1 = r2 = r3 = r4 = r5 = r6 = r7 = r8 = r9 = r10 = r11 = float4(0, 0, 0, 0);
	oFog = float4(1, 0, 0, 0);
	oPts = float4(0, 0, 0, 0);

	std::cout << "=== Starting shader execution ===" << std::endl;

	// Execute shader instructions with debugging
	try {
		std::cout << "\n--- Instruction 1: x_arl(a0,x, v2.x) ---" << std::endl;
		print_float4("v2", v[2]);
		x_arl(a0, 'x', v[2].x);
		std::cout << "a0.x = " << a0.x << std::endl;

		std::cout << "\n--- Instruction 2: x_mov(r11, \"xyzw\", c(-91)) ---" << std::endl;
		int const_index = -91 + 96; // Apply X_D3DSCM_CORRECTION
		std::cout << "Constant index: " << const_index << std::endl;
		x_mov(r11, "xyzw", vs_consts[const_index]);
		print_float4("r11", r11);

		std::cout << "\n--- Instruction 3: x_mov(oT0, \"xyzw\", v5) ---" << std::endl;
		x_mov(oT0, "xyzw", v[5]);
		print_float4("oT0", oT0);

		// Continue with the rest of the instructions...
		std::cout << "\n--- Instruction 4: x_mul(r8, \"xyzw\", v1.x, c(a0.x-88)) ---" << std::endl;
		const_index = (a0.x - 88) + 96;
		std::cout << "Constant index: " << const_index << std::endl;
		print_float4("v1", v[1]);
		print_float4("constant", vs_consts[const_index]);
		x_mul(r8, "xyzw", _tof4(v[1].x), vs_consts[const_index]);
		print_float4("r8", r8);

		// Add the remaining instructions following the same pattern...
		// Focus on getting to the RSQ instruction

		std::cout << "\n--- Continuing to RSQ instruction ---" << std::endl;

		// Execute all instructions up to the RSQ
		const_index = (a0.x - 87) + 96;
		x_mul(r9, "xyzw", _tof4(v[1].x), vs_consts[const_index]);

		const_index = (a0.x - 86) + 96;
		x_mul(r10, "xyzw", _tof4(v[1].x), vs_consts[const_index]);

		x_arl(a0, 'x', v[2].y);
		std::cout << "a0.x after second ARL: " << a0.x << std::endl;

		const_index = (a0.x - 88) + 96;
		x_mad(r8, "xyzw", _tof4(v[1].y), vs_consts[const_index], r8);

		const_index = (a0.x - 87) + 96;
		x_mad(r9, "xyzw", _tof4(v[1].y), vs_consts[const_index], r9);

		const_index = (a0.x - 86) + 96;
		x_mad(r10, "xyzw", _tof4(v[1].y), vs_consts[const_index], r10);

		x_dp3(r5, "x", v[3], r8);
		x_dp3(r5, "y", v[3], r9);
		x_dp3(r5, "z", v[3], r10);
		x_dp4(r3, "x", v[0], r8);
		x_dp3(r5, "w", r5, r5);
		x_dp4(r3, "y", v[0], r9);
		x_dp4(r3, "z", v[0], r10);

		std::cout << "\n--- Critical RSQ instruction ---" << std::endl;
		print_float4("r5 before RSQ", r5);
		std::cout << "r5.w value that will be used for RSQ: " << r5.w << std::endl;

		x_rsq(r1, "w", r5.w);
		print_float4("r1 after RSQ", r1);

	}
	catch (const std::exception& e) {
		std::cout << "Exception: " << e.what() << std::endl;
	}

	std::cout << "\n=== Shader execution complete ===" << std::endl;

}