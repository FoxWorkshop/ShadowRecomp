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
			// Test case:
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