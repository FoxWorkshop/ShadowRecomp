#include "video.h"
#include "vertexdeclaration.h"
#include "vertexshader.h"
#include <shader/shader_cache.h>
#include <map>
#include "nv2a.h"
#include "renderstate.h"
#include <kernel/function.h>
#include <os/logger.h>

extern std::unique_ptr<uint8_t[]> g_shaderCache;
uint32_t g_Xbox_VertexShader_FunctionSlots_StartAddress = 0;

// Variable set by [D3DDevice_LoadVertexShader() / [D3DDevice_LoadVertexShaderProgram() (both through CopyVertexShaderFunctionSlots):
uint32_t g_Xbox_VertexShader_FunctionSlots[(X_VSH_MAX_INSTRUCTION_COUNT + 1) * X_VSH_INSTRUCTION_SIZE] = { 0 }; // One extra for FLD_FINAL terminator

uint32_t* GetVertexShaderSlotPtr(const uint32_t SlotIndexAddress)
{
	if (SlotIndexAddress < X_VSH_MAX_INSTRUCTION_COUNT) {
		return &g_Xbox_VertexShader_FunctionSlots[SlotIndexAddress * X_VSH_INSTRUCTION_SIZE];
	}
	else {
		LOGN_ERROR("SlotIndexAddress out of range");
		return nullptr;
	}
}


void SetVertexShaderSlots(DWORD* pTokens, DWORD Address, DWORD NrInstructions)
{

	auto VertexShaderSlotPtr = GetVertexShaderSlotPtr(Address);
	if (VertexShaderSlotPtr == nullptr) {
		return;
	}

	memcpy(VertexShaderSlotPtr, pTokens, NrInstructions * X_VSH_INSTRUCTION_SIZE_BYTES);

	// Make sure slot parsing in EmuParseVshFunction (VshConvertToIntermediate) stops after the last slot;
	// Just setting bit 0 in 3rd DWORD suffices (see XboxVertexShaderDecoder.VshGetField.FieldMapping[FLD_FINAL]) :
	g_Xbox_VertexShader_FunctionSlots[(X_VSH_MAX_INSTRUCTION_COUNT * X_VSH_INSTRUCTION_SIZE) + 3] = 1;
}


static X_D3DVertexShader g_Xbox_VertexShader_ForFVF = {};

static X_D3DVertexShader* XboxVertexShaderFromFVF(size_t xboxFvf)
{
	// Note : FVFs don't tessellate, all slots read from stream zero, therefore
	// the following zero-initialization of StreamIndex (like all other fields)
	// is never updated below.
	g_Xbox_VertexShader_ForFVF = { 0 };

	// Shorthand, glueing earlier implementation below to global g_Xbox_VertexShader_ForFVF variable :
	X_VERTEXATTRIBUTEFORMAT& declaration = g_Xbox_VertexShader_ForFVF.VertexAttribute;

	static DWORD X_D3DVSDT_FLOAT[] = { 0, X_D3DVSDT_FLOAT1, X_D3DVSDT_FLOAT2, X_D3DVSDT_FLOAT3, X_D3DVSDT_FLOAT4 };

	static const DWORD InvalidXboxFVFBits = X_D3DFVF_RESERVED0 | X_D3DFVF_RESERVED1 /* probably D3DFVF_PSIZE if detected */
		| 0x0000F000 // Bits between texture count and the texture formats
		| 0xFF000000; // All bits above the four alllowed texture formats


	// Position & Blendweights
	int32_t nrPositionFloats = 3;
	int32_t nrBlendWeights = 0;
	uint32_t offset = 0;
	DWORD position = (xboxFvf & X_D3DFVF_POSITION_MASK);
	if (xboxFvf == 0)
	{
		LOGFN_WARNING("xboxFvF is null, position is equal {}", position);
	}

	switch (position) {
	case 0: nrPositionFloats = 0; break;
	case X_D3DFVF_XYZ: /*nrPositionFloats is set to 3 by default*/ break;
	case X_D3DFVF_XYZRHW: nrPositionFloats = 4; g_Xbox_VertexShader_ForFVF.Flags |= X_VERTEXSHADER_FLAG_PASSTHROUGH; break;
	case X_D3DFVF_XYZB1:
		nrBlendWeights = 1;
		break;
	case X_D3DFVF_XYZB2:
		nrBlendWeights = 2;
		break;
	case X_D3DFVF_XYZB3:
		nrBlendWeights = 3;
		break;
	case X_D3DFVF_XYZB4:
		nrBlendWeights = 4;
		break;
	case X_D3DFVF_POSITION_MASK: /*Keep nrPositionFloats set to 3*/  break;
		DEFAULT_UNREACHABLE;
	}

	// Assign vertex element (attribute) slots
	GuestVertexElement* pSlot;

	// Write Position
	if (nrPositionFloats > 0)
	{
		pSlot = &declaration.Slots[X_D3DVSDE_POSITION];
		pSlot->Format = X_D3DVSDT_FLOAT[nrPositionFloats];
		pSlot->Offset = offset;
		offset += sizeof(float) * nrPositionFloats;
		// Write Blend Weights
		if (nrBlendWeights > 0)
		{
			pSlot = &declaration.Slots[X_D3DVSDE_BLENDWEIGHT];
			pSlot->Format = X_D3DVSDT_FLOAT[nrBlendWeights];
			pSlot->Offset = offset;
			offset += sizeof(float) * nrBlendWeights;
		}
	}

	// Write Normal, Diffuse, and Specular
	if (xboxFvf & X_D3DFVF_NORMAL)
	{
		pSlot = &declaration.Slots[X_D3DVSDE_NORMAL];
		pSlot->Format = X_D3DVSDT_FLOAT[3];
		pSlot->Offset = offset;
		offset += sizeof(float) * 3;
	}

	if (xboxFvf & X_D3DFVF_DIFFUSE)
	{
		g_Xbox_VertexShader_ForFVF.Flags |= X_VERTEXSHADER_FLAG_HASDIFFUSE;
		pSlot = &declaration.Slots[X_D3DVSDE_DIFFUSE];
		pSlot->Format = X_D3DVSDT_D3DCOLOR;
		pSlot->Offset = offset;
		offset += sizeof(DWORD) * 1;
	}

	if (xboxFvf & X_D3DFVF_SPECULAR)
	{
		g_Xbox_VertexShader_ForFVF.Flags |= X_VERTEXSHADER_FLAG_HASSPECULAR;
		pSlot = &declaration.Slots[X_D3DVSDE_SPECULAR];
		pSlot->Format = X_D3DVSDT_D3DCOLOR;
		pSlot->Offset = offset;
		offset += sizeof(DWORD) * 1;
	}

	// Write Texture Coordinates
	int32_t textureCount = (xboxFvf & X_D3DFVF_TEXCOUNT_MASK) >> X_D3DFVF_TEXCOUNT_SHIFT;

	if (textureCount > 4)
		textureCount = 4; // Safeguard, since the X_D3DFVF_TEXCOUNT bitfield could contain invalid values (5 up to 15)

	for (int i = 0; i < textureCount; i++)
	{
		auto FVFTextureFormat = (xboxFvf >> X_D3DFVF_TEXCOORDSIZE_SHIFT(i)) & 0x003;

		int32_t numberOfCoordinates = ((FVFTextureFormat + 1) & 3) + 1; // ((input   +1 ) &3 ) +1 ) = output

		assert(numberOfCoordinates > 0);

		pSlot = &declaration.Slots[X_D3DVSDE_TEXCOORD0 + i];
		pSlot->Format = X_D3DVSDT_FLOAT[numberOfCoordinates];
		pSlot->Offset = offset;
		offset += sizeof(float) * numberOfCoordinates;
		// Update the VertexShader texture Dimensionality field here as well
		g_Xbox_VertexShader_ForFVF.Dimensionality[i] = numberOfCoordinates;
	}

	// Make sure all unused slots have a X_D3DVSDT_NONE format
	for (uint32_t i = 0; i < X_VSH_MAX_ATTRIBUTES; i++)
	{
		pSlot = &declaration.Slots[i];
		if (pSlot->Format == 0)
			pSlot->Format = X_D3DVSDT_NONE;

	}

	// Return the global g_Xbox_VertexShader_ForFVF variable 
	return &g_Xbox_VertexShader_ForFVF;
}


static X_VERTEXATTRIBUTEFORMAT* GetVertexShaderAttributes(X_D3DVertexShader* pXboxVertexShader)
{
	return &(pXboxVertexShader->VertexAttribute);
}

DWORD* GetVertexShaderTokens(X_D3DVertexShader* pXboxVertexShader, DWORD* pNrTokens)
{
	*pNrTokens = pXboxVertexShader->ProgramAndConstantsDwords;
	return &pXboxVertexShader->ProgramAndConstants[0];
}


namespace XboxVertexShaderDecoder
{
	// Xbox Vertex SHader microcode types

	enum VSH_OUTPUT_TYPE {
		OUTPUT_C = 0,
		OUTPUT_O
	};

	enum VSH_OUTPUT_MUX {
		OMUX_MAC = 0,
		OMUX_ILU
	};

	// Host intermediate vertex shader types

	enum VSH_FIELD_NAME {
		FLD_ILU = 0,
		FLD_MAC,
		FLD_CONST,
		FLD_V,
		// Input A
		FLD_A_NEG,
		FLD_A_SWZ_X,
		FLD_A_SWZ_Y,
		FLD_A_SWZ_Z,
		FLD_A_SWZ_W,
		FLD_A_R,
		FLD_A_MUX,
		// Input B
		FLD_B_NEG,
		FLD_B_SWZ_X,
		FLD_B_SWZ_Y,
		FLD_B_SWZ_Z,
		FLD_B_SWZ_W,
		FLD_B_R,
		FLD_B_MUX,
		// Input C
		FLD_C_NEG,
		FLD_C_SWZ_X,
		FLD_C_SWZ_Y,
		FLD_C_SWZ_Z,
		FLD_C_SWZ_W,
		FLD_C_R_HIGH,
		FLD_C_R_LOW,
		FLD_C_MUX,
		// Output
		FLD_OUT_MAC_MASK,
		FLD_OUT_R,
		FLD_OUT_ILU_MASK,
		FLD_OUT_O_MASK,
		FLD_OUT_ORB,
		FLD_OUT_ADDRESS,
		FLD_OUT_MUX,
		// Relative addressing
		FLD_A0X,
		// Final instruction
		FLD_FINAL
	};

	// Retrieves a number of bits in the instruction token
	static inline uint32_t VshGetFromToken(
		uint32_t* pShaderToken,
		uint8_t SubToken,
		uint8_t StartBit,
		uint8_t BitLength)
	{
		return (pShaderToken[SubToken] >> StartBit) & ~(0xFFFFFFFF << BitLength);
	}

	static uint8_t VshGetField(
		uint32_t* pShaderToken,
		VSH_FIELD_NAME FieldName)
	{
		// Used for xvu spec definition
		static const struct {
			uint8_t          SubToken;
			uint8_t          StartBit;
			uint8_t          BitLength;
		} FieldMapping[/*VSH_FIELD_NAME*/] = {
			// SubToken BitPos  BitSize
			{  1,   25,     3 }, // FLD_ILU,              
			{  1,   21,     4 }, // FLD_MAC,              
			{  1,   13,     8 }, // FLD_CONST,            
			{  1,    9,     4 }, // FLD_V,                
			// Input A
			{  1,    8,     1 }, // FLD_A_NEG,            
			{  1,    6,     2 }, // FLD_A_SWZ_X,          
			{  1,    4,     2 }, // FLD_A_SWZ_Y,          
			{  1,    2,     2 }, // FLD_A_SWZ_Z,          
			{  1,    0,     2 }, // FLD_A_SWZ_W,          
			{  2,   28,     4 }, // FLD_A_R,              
			{  2,   26,     2 }, // FLD_A_MUX,            
			// Input B
			{  2,   25,     1 }, // FLD_B_NEG,            
			{  2,   23,     2 }, // FLD_B_SWZ_X,          
			{  2,   21,     2 }, // FLD_B_SWZ_Y,          
			{  2,   19,     2 }, // FLD_B_SWZ_Z,          
			{  2,   17,     2 }, // FLD_B_SWZ_W,          
			{  2,   13,     4 }, // FLD_B_R,              
			{  2,   11,     2 }, // FLD_B_MUX,            
			// Input C
			{  2,   10,     1 }, // FLD_C_NEG,            
			{  2,    8,     2 }, // FLD_C_SWZ_X,          
			{  2,    6,     2 }, // FLD_C_SWZ_Y,          
			{  2,    4,     2 }, // FLD_C_SWZ_Z,          
			{  2,    2,     2 }, // FLD_C_SWZ_W,          
			{  2,    0,     2 }, // FLD_C_R_HIGH,         
			{  3,   30,     2 }, // FLD_C_R_LOW,          
			{  3,   28,     2 }, // FLD_C_MUX,            
			// Output
			{  3,   24,     4 }, // FLD_OUT_MAC_MASK,   
			{  3,   20,     4 }, // FLD_OUT_R,            
			{  3,   16,     4 }, // FLD_OUT_ILU_MASK,
			{  3,   12,     4 }, // FLD_OUT_O_MASK,
			{  3,   11,     1 }, // FLD_OUT_ORB,          
			{  3,    3,     8 }, // FLD_OUT_ADDRESS,      
			{  3,    2,     1 }, // FLD_OUT_MUX,          
			// Relative addressing
			{  3,    1,     1 }, // FLD_A0X,              
			// Final instruction
			{  3,    0,     1 }  // FLD_FINAL,            
		};

		return (uint8_t)(VshGetFromToken(pShaderToken,
			FieldMapping[FieldName].SubToken,
			FieldMapping[FieldName].StartBit,
			FieldMapping[FieldName].BitLength));
	}

	// Converts the C register address to disassembly format
	static inline int16_t ConvertCRegister(const int16_t CReg)
	{
		return ((((CReg >> 5) & 7) - 3) * 32) + (CReg & 31);
	}

	static VSH_IMD_PARAMETER VshGetIntermediateParam(
		uint32_t* pShaderToken,
		VSH_FIELD_NAME FLD_MUX,
		VSH_FIELD_NAME FLD_NEG,
		uint16_t R,
		uint16_t V,
		uint16_t C)
	{
		VSH_IMD_PARAMETER param{};
		param.Type = (VSH_IMD_PARAMETER_TYPE)VshGetField(pShaderToken, FLD_MUX);
		switch (param.Type) {
		case PARAM_R:
			param.Address = R;
			break;
		case PARAM_V:
			param.Address = V;
			break;
		case PARAM_C:
			param.Address = C;
			break;
		default:
			LOGN_WARNING("parameter type unknown");
		}

		int d = FLD_NEG - FLD_A_NEG;
		param.Neg = VshGetField(pShaderToken, (VSH_FIELD_NAME)(d + FLD_A_NEG)) > 0;
		param.Swizzle[0] = (VSH_SWIZZLE)VshGetField(pShaderToken, (VSH_FIELD_NAME)(d + FLD_A_SWZ_X));
		param.Swizzle[1] = (VSH_SWIZZLE)VshGetField(pShaderToken, (VSH_FIELD_NAME)(d + FLD_A_SWZ_Y));
		param.Swizzle[2] = (VSH_SWIZZLE)VshGetField(pShaderToken, (VSH_FIELD_NAME)(d + FLD_A_SWZ_Z));
		param.Swizzle[3] = (VSH_SWIZZLE)VshGetField(pShaderToken, (VSH_FIELD_NAME)(d + FLD_A_SWZ_W));

		return param;
	}

};

void SetVertexShaderPassthroughProgram()
{
	static DWORD XboxShaderBinaryPassthrough[] = {
		0, 0x0020001B, 0x0836106C, 0x2F100FF8,
		0, 0x0420061B, 0x083613FC, 0x5011F818,
		0, 0x002008FF, 0x0836106C, 0x2070F828,
		0, 0x0240081B, 0x1436186C, 0x2F20F824,
		0, 0x0060201B, 0x2436106C, 0x3070F800,
		0, 0x00200200, 0x0836106C, 0x2070F830,
		0, 0x00200E1B, 0x0836106C, 0x2070F838,
		0, 0x0020101B, 0x0836106C, 0x2070F840,
		0, 0x0020121B, 0x0836106C, 0x2070F848,
		0, 0x0020141B, 0x0836106C, 0x2070F850,
		0, 0x0020161B, 0x0836106C, 0x2070F858,
		0, 0x0020181B, 0x0836106C, 0x2070F861 // FLD_FINAL is set here!
	};


	SetVertexShaderSlots(&XboxShaderBinaryPassthrough[0], 0, sizeof(XboxShaderBinaryPassthrough) / X_VSH_INSTRUCTION_SIZE_BYTES);

	// Passthrough programs require scale and offset to be set in constants zero and one (both minus 96)
	// (Note, these are different from GetMultiSampleOffsetAndScale / GetViewPortOffsetAndScale)
	float scale[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	float offset[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	CopyVertexShaderConstant(0, (DWORD*)scale, 1);
	CopyVertexShaderConstant(1, (DWORD*)offset, 1);
}

size_t GetVshFunctionSize(const uint32_t* pXboxFunction) 
{
	auto curToken = (uint32_t*)pXboxFunction;

	while (!XboxVertexShaderDecoder::VshGetField(curToken, XboxVertexShaderDecoder::FLD_FINAL)) {
		curToken += X_VSH_INSTRUCTION_SIZE; 
	}

	curToken += X_VSH_INSTRUCTION_SIZE; // For the final instruction

	return (curToken - pXboxFunction) * sizeof(uint32_t);
}

static std::map<uint64_t, GuestShader*> cache;

uint64_t GetVertexShaderKey()
{
	auto pTokens = GetVertexShaderSlotPtr(g_Xbox_VertexShader_FunctionSlots_StartAddress);
	DWORD shaderSize = GetVshFunctionSize(pTokens);
	auto VertexShaderKey = XXH3_64bits((void*)pTokens, shaderSize);
	return VertexShaderKey;
}

void UpdateShaderCacheEntryNew(GuestShader* shader, const uint64_t key)
{
	cache[key] = shader;
}

GuestShader* FindShaderCacheEntryNew()
{

	auto VertexShaderKey = GetVertexShaderKey();
	auto it = cache.find(VertexShaderKey);
	if (it != cache.end()) 
	{
		return it->second;
	}

	return nullptr; //findResult != end && findResult->hash == hash ? findResult : nullptr;
}

const uint8_t* CreateRecompiledShader(size_t handle, size_t* outSize)
{
	auto pTokens = GetVertexShaderSlotPtr(g_Xbox_VertexShader_FunctionSlots_StartAddress);
	assert(pTokens);
	// Create a vertex shader from the tokens
	DWORD shaderSize;
	const bool isvulkan = Video::is_vulkan();
	bool exportHLSLFile = false;

#ifdef _DEBUG
	exportHLSLFile = true;
#endif

	auto pShaderOut = XSR_CreateRecompiledVertexShader(handle, pTokens, &shaderSize, outSize, isvulkan, exportHLSLFile);
	return pShaderOut;

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
			LOGN_WARNING("Zero-length NV2A method detected!");
			break;
		}

		switch (dwMethod) {
		case NV2A_VP_UPLOAD_INST(0): { // = 0x00000B00
			if ((nrDWORDS & 3) != 0)
				LOGN_WARNING("NV2A_VP_UPLOAD_INST arguments should be a multiple of 4!");

			unsigned nrSlots = nrDWORDS / X_VSH_INSTRUCTION_SIZE;
			SetVertexShaderSlots(pNV2ATokens, Address, nrSlots);
			Address += nrSlots;
			break;
		}
		case NV2A_VP_UPLOAD_CONST_ID: // = 0x00001EA4
			if (nrDWORDS != 1)
				LOGN_WARNING("NV2A_VP_UPLOAD_CONST_ID should have one argument!");

			ConstantAddress = *pNV2ATokens;
			break;
		case NV2A_VP_UPLOAD_CONST(0): { // = 0x00000B80
			if ((nrDWORDS & 3) != 0)
				LOGN_WARNING("NV2A_VP_UPLOAD_CONST arguments should be a multiple of 4!");

			unsigned nrConstants = nrDWORDS / X_VSH_INSTRUCTION_SIZE;
			// TODO : FIXME : Implement and call SetVertexShaderConstants(pNV2ATokens, ConstantAddress, nrConstants);
			ConstantAddress += nrConstants;
			break;
		}
		default:
			// TODO : Remove this break-out hack once NrTokens is reliable and instead have: DEFAULT_UNREACHABLE;
			LOGN_WARNING("Stopping at unexpected NV2A method");
			pEnd = pNV2ATokens;
			break;
		}

		pNV2ATokens += nrDWORDS;
	}
}

RenderShader* GetOrLinkShader(GuestShader* guestShader, uint32_t specConstants)
{
	const char* shaderType = guestShader->type == ResourceType::PixelShader ? "PS" : "VS";

	if (Video::is_vulkan() ||
		guestShader->shaderCacheEntry == nullptr ||
		guestShader->shaderCacheEntry->specConstantsMask == 0)
	{
		std::lock_guard lock(guestShader->mutex);

		if (guestShader->shader == nullptr)
		{
			assert(guestShader->shaderCacheEntry != nullptr);

			auto lazyStart = std::chrono::steady_clock::now();

			if (Video::is_vulkan())
			{
				auto compressedSpirvData = g_shaderCache.get() + guestShader->shaderCacheEntry->spirvOffset;

				std::vector<uint8_t> decoded(smolv::GetDecodedBufferSize(compressedSpirvData, guestShader->shaderCacheEntry->spirvSize));
				bool result = smolv::Decode(compressedSpirvData, guestShader->shaderCacheEntry->spirvSize, decoded.data(), decoded.size());
				assert(result);

				guestShader->shader = Video::GetRenderDevice()->createShader(decoded.data(), decoded.size(), "main", RenderShaderFormat::SPIRV);
			}
			else
			{
				guestShader->shader = Video::GetRenderDevice()->createShader(g_shaderCache.get() + guestShader->shaderCacheEntry->dxilOffset,
					guestShader->shaderCacheEntry->dxilSize, "main", RenderShaderFormat::DXIL);
			}

			auto lazyElapsed = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - lazyStart).count();
			LOGFN("GetOrLinkShader: {} LAZY creation hash={:016X} spirvSize={} dxilSize={} -> {:.2f} ms (shader was NOT pre-compiled!)",
				shaderType, guestShader->shaderCacheEntry->hash, guestShader->shaderCacheEntry->spirvSize, guestShader->shaderCacheEntry->dxilSize, lazyElapsed);
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
				linkedShader = Video::GetRenderDevice()->createShader(blob->GetBufferPointer(), blob->GetBufferSize(), "main", RenderShaderFormat::DXIL);
				guestShader->shaderBlobs.push_back(std::move(blob));
			}

			shader = linkedShader.get();
		}
	}
#endif

	return shader;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3D1F70, SelectVertexShader);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1F00, LoadVertexShaderProgram);
GUEST_FUNCTION_HOOK_STDCALL(sub_3D1EA0, LoadVertexShader);