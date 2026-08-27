#include "video.h"
#include "vertexdeclaration.h"
#include <xxHashMap.h>
#include <kernel/heap.h>
#include "renderstate.h"
#include <algorithm>
#include <DirectXMath.h>
#include "FixedFunction.h"
#include "ubershaders/fixed_function_vertex_shader_variants.h"

// original code (mostly) re used from CXBX Reloaded
// Since modern API don't have a fixed function pipeline, we have to emulate it ourselves using shaders.
// This page handle many features of the fixed function pipeline such as lighting, world/view/projection matrices, etc
// Those will be used as constants for our uber vertex shader which is emulating the fixed function pipeline
// for more information, see FixedFunctionVertexShader.hlsl and FixedFunctionPixelShader.hlsl

using namespace DirectX;

// Combines world/view matrices
static std::array<D3DMATRIX, 4> g_WorldView;
// World/view inverse transpose for lighting calculations
static std::array<D3DMATRIX, 4> g_WorldViewInverseTranspose;

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

D3D8LightState::D3D8LightState() {
	// Define the default light
	// When unset lights are enabled, they're set to the default light
	auto defaultLight = X_D3DLIGHT8();
	defaultLight.Type = D3DLIGHT_DIRECTIONAL;
	defaultLight.Diffuse = colorValue(1, 1, 1, 0);
	defaultLight.Specular = colorValue(0, 0, 0, 0);
	defaultLight.Ambient = colorValue(0, 0, 0, 0);
	defaultLight.Position = toVector(0, 0, 0);
	defaultLight.Direction = toVector(0, 0, 1);
	defaultLight.Range = 0;
	defaultLight.Falloff = 0;
	defaultLight.Attenuation0 = 0;
	defaultLight.Attenuation1 = 0;
	defaultLight.Attenuation2 = 0;
	defaultLight.Theta = 0;
	defaultLight.Phi = 0;

	// We'll just preset every light to the default light
	Lights.fill(defaultLight);
	EnabledLights.fill(-1);
}

bool D3D8LightState::EnableLight(uint32_t index, bool enable) {
	// Since Xbox only supports 8 lights, we keep track of the 8 most recently enabled lights
	// Lights are ordered oldest to newest, with disabled lights at the end

	// Check to see if the light is already enabled
	for (size_t i = 0; i < EnabledLightCount; i++) {

		// If the light is already in the enabled lights
		if (EnabledLights[i] == index) {
			// Re-enabling the newest light leaves the set and order unchanged
			if (enable && i == EnabledLightCount - 1)
				return false;

			// Either way we move this light to the end
			std::rotate(std::begin(EnabledLights) + i, std::begin(EnabledLights) + i + 1, std::begin(EnabledLights) + EnabledLightCount);

			if (!enable)
			{
				// Disable the light
				EnabledLights[EnabledLightCount - 1] = -1;
				EnabledLightCount--;
			}
			return true;
		}
	}

	if (enable) {
		// The light was not in the enabled lights. Let's add it
		if (EnabledLightCount < EnabledLights.size()) {
			EnabledLights[EnabledLightCount] = index; // add it to the end
			EnabledLightCount++;
		}
		else {
			// Replace the oldest element and move to end

			EnabledLights[0] = index;
			std::rotate(std::begin(EnabledLights), std::begin(EnabledLights) + 1, std::end(EnabledLights));
		}
		return true;
	}

	return false;
}



D3D8LightState g_LightState = D3D8LightState();
// I (sewer) won't deny, that having these as static is a bit sus.
// But fret not, D3D8 is singlethreaded in operation; all callers which
// access this can only run on one thread at a time- promise!
static bool g_fixedFunctionVertexShaderVariantDirty = true;
static bool g_fixedFunctionVertexShaderStateDirty = true;
static bool g_fixedFunctionVertexShaderMaterialDirty = true;
static bool g_fixedFunctionVertexShaderLightsDirty = true;
static XMFLOAT4 g_fixedFunctionLightAmbient;
static ShaderVariantKey g_fixedFunctionVertexShaderVariant = GenericShaderVariant;

void MarkFixedFunctionVertexShaderStateDirty()
{
	g_fixedFunctionVertexShaderStateDirty = true;
}

void MarkFixedFunctionVertexShaderVariantDirty()
{
	g_fixedFunctionVertexShaderVariantDirty = true;
	MarkFixedFunctionVertexShaderStateDirty();
}

void MarkFixedFunctionVertexShaderLightsDirty()
{
	g_fixedFunctionVertexShaderLightsDirty = true;
	MarkFixedFunctionVertexShaderStateDirty();
}

void LightEnable(uint32_t Index, int32_t bEnable)
{
	if (g_LightState.EnableLight(Index, bEnable))
	{
		MarkFixedFunctionVertexShaderLightsDirty();
		MarkFixedFunctionVertexShaderVariantDirty();
	}
}

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


struct Transforms
{
	XMFLOAT4X4 View;
	XMFLOAT4X4 Projection;
	XMFLOAT4X4 Texture[4]; // c8-c23 (4 matrices x 4 regs each)
	XMFLOAT4X4 WorldView[4];
	XMFLOAT4X4 WorldViewInverseTranspose[4];
};

static_assert(sizeof(Transforms) % 16 == 0, "Transforms must be multiple of 16 bytes");

struct Light {

	XMFLOAT4 Diffuse;
	XMFLOAT4 Specular;

	// Viewspace light position Range in posW
	D3DVECTOR4 PositionV;
	// alignas(16) float Range;

	 // Viewspace light direction (normalized) - type in directionW
	D3DVECTOR4 DirectionVN;
	// alignas(16) float Type; // 1=Point, 2=Spot, 3=Directional

	D3DVECTOR4 Attenuation; //fall off in W
	//alignas(16) float Falloff;

	D3DVECTOR4 SpotParams; // cN+5: x=CosHalfPhi, y=SpotIntensityDivisor, zw unused
};
static_assert(sizeof(Light) % 16 == 0, "Light must be multiple of 16 bytes");


// Vertex lighting
// Both frontface and backface lighting can be calculated
struct TwoSidedColor
{
	D3DVECTOR4 Front; // c104.xyz
	D3DVECTOR4 Back; // c105.xyz
};
static_assert(sizeof(TwoSidedColor) % 16 == 0, "TwoSidedColor must be multiple of 16 bytes");

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
	float _pad0; 	// c118.w unused


	// c119
	// float ColorVertex;
	float VertexBlend_NrOfMatrices;
	float VertexBlend_CalcLastWeight;
	float NormalizeNormals;
	float _pad1; 	// c119.w unused

};

static_assert(sizeof(Modes) == 64);

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
	float _pad0;
	float _pad1;
};

static_assert(sizeof(Fog) == 32);


struct TextureState
{
	float TextureTransformFlagsCount;
	float TextureTransformFlagsProjected;
	float TexCoordIndex;
	float TexCoordIndexGen;
};

static_assert(sizeof(TextureState) == 16);

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

static_assert(sizeof(PointSprite) == 32);

struct FixedFunctionVertexShaderState
{
	Transforms Transforms;
	Light Lights[8];
	TwoSidedColor TotalLightsAmbient; // c104 - c105
	Material Materials[2]; // c106 - c115
	Modes Modes; // c116 - c119
	Fog Fog; // c120 - c121
	TextureState TextureStates[4]; // c122 - c125;
	PointSprite PointSprite; // c126 - c127
	float TexCoordComponentCount[4]; // c128
};


static_assert(sizeof(FixedFunctionVertexShaderState) == 2064);

FixedFunctionVertexShaderState ffShaderState = {};
extern int32_t* g_texture_stage_states;
extern int32_t g_RenderUpscaleFactor;
extern std::array<D3DMATRIX, X_D3DTS_MAX> g_Transforms;

void UpdateMaterial(Material* pMaterial)
{
	auto& material = ffShaderState.Materials[0];
	if (memcmp(&material.Diffuse, &pMaterial->Diffuse, sizeof(D3DCOLORVALUE)) == 0 &&
		memcmp(&material.Ambient, &pMaterial->Ambient, sizeof(D3DCOLORVALUE)) == 0 &&
		memcmp(&material.Specular, &pMaterial->Specular, sizeof(D3DCOLORVALUE)) == 0 &&
		memcmp(&material.Emissive, &pMaterial->Emissive, sizeof(D3DCOLORVALUE)) == 0 &&
		memcmp(&material.Power.r, &pMaterial->Power.r, sizeof(material.Power.r)) == 0)
		return;

	g_fixedFunctionVertexShaderMaterialDirty = true;
	ffShaderState.Materials[0].Ambient = pMaterial->Ambient;
	ffShaderState.Materials[0].Diffuse = pMaterial->Diffuse;
	ffShaderState.Materials[0].Specular = pMaterial->Specular;
	ffShaderState.Materials[0].Emissive = pMaterial->Emissive;
	ffShaderState.Materials[0].Power.r = pMaterial->Power.r; //power is originally only a single float, but we have it as a float4 for easier packing into constant buffers.
}

void UpdateLightData(uint32_t index, X_D3DLIGHT8* pLight)
{
	auto& light = g_LightState.Lights[index];
	if (memcmp(&light, pLight, sizeof(light)) == 0)
		return;

	const bool typeChanged = light.Type != pLight->Type;
	g_LightState.Lights[index] = *pLight;
	if (std::find(g_LightState.EnabledLights.begin(),
		g_LightState.EnabledLights.begin() + g_LightState.EnabledLightCount, index) ==
		g_LightState.EnabledLights.begin() + g_LightState.EnabledLightCount)
		return;

	MarkFixedFunctionVertexShaderLightsDirty();
	if (typeChanged)
		MarkFixedFunctionVertexShaderVariantDirty();
}

void UpdateFixedFunctionShaderLight(int d3dLightIndex, Light* pShaderLight,
	DirectX::XMFLOAT4* pLightAmbient, DirectX::FXMMATRIX view, bool specularEnable) {
	if (d3dLightIndex == -1)
	{
		pShaderLight->DirectionVN.w = 0; // Disable the light
		return;
	}

	auto d3dLight = &g_LightState.Lights[d3dLightIndex];

	// TODO remove D3DX usage
	// Pre-transform light position to viewspace
	// Transform light position into view space
	if (d3dLight->Type != D3DLIGHT_DIRECTIONAL)
	{
		XMVECTOR pos = XMLoadFloat3((XMFLOAT3*)&d3dLight->Position);
		XMVECTOR positionV = XMVector3Transform(pos, view);
		XMStoreFloat3((XMFLOAT3*)&pShaderLight->PositionV, positionV);
	}

	// Transform light direction into view space (ignore translation)
	XMVECTOR dir = XMLoadFloat3((XMFLOAT3*)&d3dLight->Direction);
	XMVECTOR directionV = XMVector3TransformNormal(dir, view);

	// Normalize direction
	directionV = XMVector3Normalize(directionV);

	XMStoreFloat3((XMFLOAT3*)&pShaderLight->DirectionVN, directionV);

	// Map D3D light to state struct
	pShaderLight->DirectionVN.w = (float)((int)d3dLight->Type); //DirectionVN.w = type
	pShaderLight->Diffuse = toVector(d3dLight->Diffuse);
	pShaderLight->Specular = specularEnable ? toVector(d3dLight->Specular) : toVector(0);
	if (d3dLight->Type != D3DLIGHT_DIRECTIONAL)
	{
		pShaderLight->PositionV.w = d3dLight->Range; //PositionV = range
		pShaderLight->Attenuation.x = d3dLight->Attenuation0;
		pShaderLight->Attenuation.y = d3dLight->Attenuation1;
		pShaderLight->Attenuation.z = d3dLight->Attenuation2;
	}

	pLightAmbient->x += d3dLight->Ambient.r;
	pLightAmbient->y += d3dLight->Ambient.g;
	pLightAmbient->z += d3dLight->Ambient.b;

	if (d3dLight->Type == D3DLIGHT_SPOT)
	{
		pShaderLight->Attenuation.w = d3dLight->Falloff; //Attenuation.w = falloff
		auto cosHalfPhi = cos(d3dLight->Phi / 2);
		pShaderLight->SpotParams.x = cosHalfPhi; //SpotParams.x = CosHalfPhi
		pShaderLight->SpotParams.y = cos(d3dLight->Theta / 2) - cosHalfPhi; //SpotParams.y = SpotIntensityDivisor
	}
}


void RecalculateDependentMatrices(uint32_t i)
{
	auto worldState = X_D3DTS_WORLD + i;

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
ShaderVariantKey UpdateFixedFunctionVertexShaderState(size_t Handle, bool isPSFixed, uint32_t vertexRegisterDefaultMask)
{
	if (!g_fixedFunctionVertexShaderStateDirty)
	{
		// Upload only the material registers when the rest of the state is clean.
		if (g_fixedFunctionVertexShaderMaterialDirty)
		{
			constexpr int materialRegister = offsetof(FixedFunctionVertexShaderState, Materials) / 16;
			CopyVertexShaderConstant(materialRegister, (DWORD*)&ffShaderState.Materials[0], sizeof(Material) / 16);
			g_fixedFunctionVertexShaderMaterialDirty = false;
		}
		return g_fixedFunctionVertexShaderVariant;
	}

	// Vertex blending
	// Prepare vertex blending mode variables used in transforms, below
	auto VertexBlend = GetGuestRenderState(X_D3DRS_VERTEXBLEND);
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

	// Note DX12 expects matrix to be column major for HLSL so we convert them with Transpose
	// however this breaks Vulkan when multiplication with matrix and vector are involved,
	// so make sure to not transpose if vulkan is used.
	const bool vulkan = is_vulkan();

	// Calculate the number of matrices, by adding the LSB to turn (0,1,3,5) and (0,2,4,6) into (0,2,4,6); Then divide by 2 to get (0,1,2,3), and add 1 to get 1, 2, 3 or 4 matrices :
	auto NrBlendMatrices = ((VertexBlend + (VertexBlend & 1)) / 2) + 1;
	// Looking at the above values, 0 or the LSB of VertexBlend signals that the final weight needs to be calculated from all previous weigths (deducting them all from an initial 1) :
	auto CalcLastBlendWeight = (VertexBlend == X_D3DVBF_DISABLE) || (VertexBlend & 1);
	// Copy the resulting values over to shader state :
	ffShaderState.Modes.VertexBlend_NrOfMatrices = (float)NrBlendMatrices;
	ffShaderState.Modes.VertexBlend_CalcLastWeight = (float)CalcLastBlendWeight;

	// Transforms
	// Transpose row major to column major for HLSL (unless Vulkan is used as explained above).
	XMMATRIX projection = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[X_D3DTS_PROJECTION]); //first we read the projection matrix
	XMMATRIX projectionT = vulkan ? projection : XMMatrixTranspose(projection); //then we transpose it
	XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.Projection, projectionT); //finally we store it (overwrite previous data if any)

	XMMATRIX view = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[X_D3DTS_VIEW]);
	XMMATRIX viewT = vulkan ? view : XMMatrixTranspose(view);
	XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.View, viewT);

	for (unsigned i = 0; i < 4; i++)
	{ // TODO : Would it help to limit this to just the active texture channels?

		XMMATRIX transfoTexture = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[X_D3DTS_TEXTURE0 + i]);
		XMMATRIX transfoTextureT = vulkan ? transfoTexture : XMMatrixTranspose(transfoTexture);
		XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.Texture[i], transfoTextureT);
	}

	for (unsigned i = 0; i < ffShaderState.Modes.VertexBlend_NrOfMatrices; i++)
	{

		XMMATRIX transfoWorldView = XMLoadFloat4x4((XMFLOAT4X4*)GetWorldView(i));
		XMMATRIX transfoWorldViewT = vulkan ? transfoWorldView : XMMatrixTranspose(transfoWorldView);
		XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.WorldView[i], transfoWorldViewT);

		XMMATRIX WorldViewInverseTranspose = XMLoadFloat4x4((XMFLOAT4X4*)GetWorldViewInverseTranspose(i));
		XMMATRIX WorldViewInverseTransposeT = vulkan ? WorldViewInverseTranspose : XMMatrixTranspose(WorldViewInverseTranspose);
		XMStoreFloat4x4((XMFLOAT4X4*)&ffShaderState.Transforms.WorldViewInverseTranspose[i], WorldViewInverseTransposeT);
	}

	// Lighting
	// Point sprites aren't lit - 'each point is always rendered with constant colors.'
	// https://docs.microsoft.com/en-us/windows/win32/direct3d9/point-sprites
	bool PointSpriteEnable = GetGuestRenderState(X_D3DRS_POINTSPRITEENABLE);
	bool LightingEnable = GetGuestRenderState(X_D3DRS_LIGHTING);
	ffShaderState.Modes.Lighting = LightingEnable && !PointSpriteEnable;

	ffShaderState.Modes.TwoSidedLighting = (float)GetGuestRenderState(X_D3DRS_TWOSIDEDLIGHTING);
	ffShaderState.Modes.LocalViewer = (float)GetGuestRenderState(X_D3DRS_LOCALVIEWER);

	// Material sources
	bool ColorVertex = GetGuestRenderState(X_D3DRS_COLORVERTEX) != FALSE;
	ffShaderState.Modes.AmbientMaterialSource = (float)(ColorVertex ? GetGuestRenderState(X_D3DRS_AMBIENTMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.DiffuseMaterialSource = (float)(ColorVertex ? GetGuestRenderState(X_D3DRS_DIFFUSEMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.SpecularMaterialSource = (float)(ColorVertex ? GetGuestRenderState(X_D3DRS_SPECULARMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.EmissiveMaterialSource = (float)(ColorVertex ? GetGuestRenderState(X_D3DRS_EMISSIVEMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.BackAmbientMaterialSource = (float)(ColorVertex ? GetGuestRenderState(X_D3DRS_BACKAMBIENTMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.BackDiffuseMaterialSource = (float)(ColorVertex ? GetGuestRenderState(X_D3DRS_BACKDIFFUSEMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.BackSpecularMaterialSource = (float)(ColorVertex ? GetGuestRenderState(X_D3DRS_BACKSPECULARMATERIALSOURCE) : D3DMCS_MATERIAL);
	ffShaderState.Modes.BackEmissiveMaterialSource = (float)(ColorVertex ? GetGuestRenderState(X_D3DRS_BACKEMISSIVEMATERIALSOURCE) : D3DMCS_MATERIAL);

	// Point sprites; Fetch required variables
	float pointSize = GetGuestRenderStateAsFloat(X_D3DRS_POINTSIZE);
	float pointSize_Min = GetGuestRenderStateAsFloat(X_D3DRS_POINTSIZE_MIN);
	float pointSize_Max = GetGuestRenderStateAsFloat(X_D3DRS_POINTSIZE_MAX);
	bool PointScaleEnable = GetGuestRenderState(X_D3DRS_POINTSCALEENABLE);
	float pointScale_A = GetGuestRenderStateAsFloat(X_D3DRS_POINTSCALE_A);
	float pointScale_B = GetGuestRenderStateAsFloat(X_D3DRS_POINTSCALE_B);
	float pointScale_C = GetGuestRenderStateAsFloat(X_D3DRS_POINTSCALE_C);
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
	auto fogEnable = GetGuestRenderState(X_D3DRS_FOGENABLE);
	auto fogTableMode = GetGuestRenderState(X_D3DRS_FOGTABLEMODE);
	ffShaderState.Fog.Enable = fogEnable;
	// FIXME remove when fixed function PS is implemented
	// Note if we are using the fixed function pixel shader
	// We only want to produce the fog depth value in the VS, not the fog factor
	ffShaderState.Fog.TableMode = !isPSFixed ? D3DFOG_NONE : fogTableMode;

	// Determine how fog depth is calculated
	if (fogEnable && fogTableMode != D3DFOG_NONE)
	{
		auto proj = &ffShaderState.Transforms.Projection;

		if (GetGuestRenderState(X_D3DRS_RANGEFOGENABLE))
		{
		
			ffShaderState.Fog.DepthMode = FixedFunctionVertexShader::FOG_DEPTH_RANGE;
		}
		else if (proj->_14 == 0 &&
			proj->_24 == 0 &&
			proj->_34 == 0 &&
			proj->_44 == 1) {
	
			ffShaderState.Fog.DepthMode = FixedFunctionVertexShader::FOG_DEPTH_Z;
		}
		else {
			// Fog sample

			ffShaderState.Fog.DepthMode = FixedFunctionVertexShader::FOG_DEPTH_W;
		}

		auto density = GetGuestRenderState(X_D3DRS_FOGDENSITY);
		auto fogStart = GetGuestRenderState(X_D3DRS_FOGSTART);
		auto fogEnd = GetGuestRenderState(X_D3DRS_FOGEND);
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

	// Update lights
	// The shader reads the light block and ambient totals only when lighting
	// is enabled, so skip both when it is disabled.
	if (ffShaderState.Modes.Lighting)
	{
		if (g_fixedFunctionVertexShaderLightsDirty)
		{
			g_fixedFunctionLightAmbient = XMFLOAT4(0.f, 0.f, 0.f, 0.f);
			const XMMATRIX lightView = XMLoadFloat4x4((XMFLOAT4X4*)&g_Transforms[X_D3DTS_VIEW]);
			const bool specularEnable = GetGuestRenderState(X_D3DRS_SPECULARENABLE) != FALSE;
			for (size_t i = 0; i < 8; i++)
				UpdateFixedFunctionShaderLight(g_LightState.EnabledLights[i], &ffShaderState.Lights[i],
					&g_fixedFunctionLightAmbient, lightView, specularEnable);
			g_fixedFunctionVertexShaderLightsDirty = false;
		}

		XMFLOAT4 Ambient = toVector(GetGuestRenderState(X_D3DRS_AMBIENT));
		XMFLOAT4 BackAmbient = toVector(GetGuestRenderState(X_D3DRS_BACKAMBIENT));
		XMVECTOR vLightAmbient = XMLoadFloat4(&g_fixedFunctionLightAmbient);

		XMStoreFloat4((XMFLOAT4*)&ffShaderState.TotalLightsAmbient.Front,
			XMVectorAdd(vLightAmbient, XMLoadFloat4(&Ambient)));
		XMStoreFloat4((XMFLOAT4*)&ffShaderState.TotalLightsAmbient.Back,
			XMVectorAdd(vLightAmbient, XMLoadFloat4(&BackAmbient)));
	}

	// Misc flags
	ffShaderState.Modes.NormalizeNormals = (float)GetGuestRenderState(X_D3DRS_NORMALIZENORMALS);

	if (g_fixedFunctionVertexShaderVariantDirty)
	{
		// Move current fixed-function controls shader selector state.
		// We can then pick the optimised shader.
		FixedFunctionVertexShaderControlState controlState{};
		// A source value above 255 would wrap and merge distinct states.
		auto narrow = [](float value)
		{
			assert(value <= 255.f && value >= 0.f);
			return (uint8_t)value;
		};

		controlState.vertex_blend_matrix_count = narrow(ffShaderState.Modes.VertexBlend_NrOfMatrices);
		controlState.vertex_blend_calc_last_weight = narrow(ffShaderState.Modes.VertexBlend_CalcLastWeight);
		controlState.lighting_enable = narrow(ffShaderState.Modes.Lighting);
		controlState.two_sided_lighting_enable = narrow(ffShaderState.Modes.TwoSidedLighting);
		controlState.local_viewer_enable = narrow(ffShaderState.Modes.LocalViewer);
		controlState.normalize_normals_enable = narrow(ffShaderState.Modes.NormalizeNormals);
		controlState.ambient_material_source = narrow(ffShaderState.Modes.AmbientMaterialSource);
		controlState.diffuse_material_source = narrow(ffShaderState.Modes.DiffuseMaterialSource);
		controlState.specular_material_source = narrow(ffShaderState.Modes.SpecularMaterialSource);
		controlState.emissive_material_source = narrow(ffShaderState.Modes.EmissiveMaterialSource);
		controlState.fog_enable = narrow(ffShaderState.Fog.Enable);
		controlState.fog_depth_mode = narrow(ffShaderState.Fog.DepthMode);
		for (uint32_t i = 0; i < FixedFunctionVertexShaderControlState::LightCount; i++)
			controlState.light_type[i] = narrow(ffShaderState.Lights[i].DirectionVN.w);
		for (uint32_t i = 0; i < FixedFunctionVertexShaderControlState::TextureStageCount; i++)
		{
			controlState.texture_transform_count[i] = narrow(ffShaderState.TextureStates[i].TextureTransformFlagsCount);
			controlState.texcoord_index[i] = narrow(ffShaderState.TextureStates[i].TexCoordIndex);
			controlState.texcoord_generation[i] = narrow(ffShaderState.TextureStates[i].TexCoordIndexGen);
			controlState.texcoord_component_count[i] = narrow(ffShaderState.TexCoordComponentCount[i]);
		}
		controlState.vertex_register_default_mask = vertexRegisterDefaultMask;
		controlState.point_sprite_enable = PointSpriteEnable;
		controlState.point_scale_enable = PointScaleEnable;
		controlState.specular_enable = GetGuestRenderState(X_D3DRS_SPECULARENABLE) != FALSE;
		g_fixedFunctionVertexShaderVariant = FixedFunctionVertexShaderVariants::Select(controlState);
		g_fixedFunctionVertexShaderVariantDirty = false;
	}

	// Write fixed function state to shader constants
	const int slotSize = 16;
	const int fixedFunctionStateSize = (sizeof(FixedFunctionVertexShaderState) + slotSize - 1) / slotSize;
	CopyVertexShaderConstant(0, (DWORD*)&ffShaderState, fixedFunctionStateSize);
	g_fixedFunctionVertexShaderStateDirty = false;
	g_fixedFunctionVertexShaderMaterialDirty = false;

	return g_fixedFunctionVertexShaderVariant;
}
