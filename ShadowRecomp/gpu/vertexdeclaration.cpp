#include "video.h"
#include "vertexdeclaration.h"
#include "vertexshader.h"
#include <xxHashMap.h>
#include <kernel/heap.h>
#include <algorithm>

//original code (mostly) re used from CXBX Reloaded

static Mutex g_vertexDeclarationMutex;
static xxHashMap<GuestVertexDeclaration*> g_vertexDeclarations;

static const char* ConvertDeclUsage(uint32_t usage)
{
	switch (usage)
	{
	case D3DDECLUSAGE_POSITION:
		return "POSITION";
	case D3DDECLUSAGE_BLENDWEIGHT:
		return "BLENDWEIGHT";
	case D3DDECLUSAGE_BLENDINDICES:
		return "BLENDINDICES";
	case D3DDECLUSAGE_NORMAL:
		return "NORMAL";
	case D3DDECLUSAGE_PSIZE:
		return "PSIZE";
	case D3DDECLUSAGE_TEXCOORD:
		return "TEXCOORD";
	case D3DDECLUSAGE_TANGENT:
		return "TANGENT";
	case D3DDECLUSAGE_BINORMAL:
		return "BINORMAL";
	case D3DDECLUSAGE_TESSFACTOR:
		return "TESSFACTOR";
	case D3DDECLUSAGE_POSITIONT:
		return "POSITIONT";
	case D3DDECLUSAGE_COLOR:
		return "COLOR";
	case D3DDECLUSAGE_FOG:
		return "FOG";
	case D3DDECLUSAGE_DEPTH:
		return "DEPTH";
	case D3DDECLUSAGE_SAMPLE:
		return "SAMPLE";
	default:
		assert(false && "Unknown usage");
		return "UNKNOWN";
	}
}

static const char* ConvertDeclTypeToString(RenderFormat type)
{
	switch (type)
	{
	case RenderFormat::R32_FLOAT:
		return "R32_FLOAT";
	case RenderFormat::R32_UINT:
		return "R32_UINT";
	case RenderFormat::R32G32_FLOAT:
		return "R32G32_FLOAT";
	case RenderFormat::R32G32B32_FLOAT:
		return "R32G32B32_FLOAT";
	case RenderFormat::R32G32B32A32_FLOAT:
		return "R32G32B32A32_FLOAT";
	case RenderFormat::B8G8R8A8_UNORM:
		return "B8G8R8A8_UNORM";
	case RenderFormat::R8G8B8A8_UINT:
		return "R8G8B8A8_UINT";
	case RenderFormat::R16G16_SINT:
		return "R16G16_SINT";
	case RenderFormat::R16G16B16A16_SINT:
		return "R16G16B16A16_SINT";
	case RenderFormat::R8G8B8A8_UNORM:
		return "R8G8B8A8_UNORM";
	case RenderFormat::R16G16_SNORM:
		return "R16G16_SNORM";
	case RenderFormat::R16G16B16A16_SNORM:
		return "R16G16B16A16_SNORM";
	case RenderFormat::R16G16_UNORM:
		return "R16G16_UNORM";
	case RenderFormat::R16G16B16A16_UNORM:
		return "R16G16B16A16_UNORM";
	case RenderFormat::R16G16_FLOAT:
		return "R16G16_FLOAT";
	case RenderFormat::R16G16B16A16_FLOAT:
		return "R16G16B16A16_FLOAT";
	default:
		assert(false && "Unknown type");
		return "UNKNOWN";
	}
}

static RenderFormat ConvertDeclType(uint32_t type)
{
	switch (type)
	{
	case D3DDECLTYPE_FLOAT1:
		return RenderFormat::R32_FLOAT;
	case D3DDECLTYPE_FLOAT2:
		return RenderFormat::R32G32_FLOAT;
	case D3DDECLTYPE_FLOAT3:
		return RenderFormat::R32G32B32_FLOAT;
	case D3DDECLTYPE_FLOAT4:
		return RenderFormat::R32G32B32A32_FLOAT;
	case D3DDECLTYPE_D3DCOLOR:
		return RenderFormat::B8G8R8A8_UNORM;
	case D3DDECLTYPE_UBYTE4:
		return RenderFormat::R8G8B8A8_UINT;
	case D3DDECLTYPE_SHORT2:
		return RenderFormat::R16G16_SINT;
	case D3DDECLTYPE_SHORT4:
		return RenderFormat::R16G16B16A16_SINT;
	case D3DDECLTYPE_UBYTE4N:
		return RenderFormat::R8G8B8A8_UNORM;
	case D3DDECLTYPE_SHORT2N:
		return RenderFormat::R16G16_SNORM;
	case D3DDECLTYPE_SHORT4N:
		return RenderFormat::R16G16B16A16_SNORM;
	case D3DDECLTYPE_USHORT2N:
		return RenderFormat::R16G16_UNORM;
	case D3DDECLTYPE_USHORT4N:
		return RenderFormat::R16G16B16A16_UNORM;
	case D3DDECLTYPE_FLOAT16_2:
		return RenderFormat::R16G16_FLOAT;
	case D3DDECLTYPE_FLOAT16_4:
		return RenderFormat::R16G16B16A16_FLOAT;
	default:
		assert(false && "Unknown type");
		return RenderFormat::UNKNOWN;
	}
}


class XboxVertexDeclarationConverter
{
protected:
	// Internal variables
	HostVertexDeclaration* pCurrentVertexDeclaration;
	HostVertexShaderStreamInfo* pCurrentVertexShaderStreamInfo = nullptr;
	bool IsFixedFunction;
	D3DVERTEXELEMENT9* pCurrentHostVertexElement;
	std::array<bool, 16> RegVIsPresentInDeclaration;

private:
#define D3DDECLUSAGE_UNSUPPORTED ((D3DDECLUSAGE)-1)

	D3DDECLUSAGE Xb2PCRegisterType(DWORD VertexRegister, BYTE& UsageIndex)
	{
		UsageIndex = 0;
		switch (VertexRegister) {
		case X_D3DVSDE_POSITION    /*= 0*/:                 
			return D3DDECLUSAGE_POSITION;
		case X_D3DVSDE_BLENDWEIGHT /*= 1*/:                 
			return D3DDECLUSAGE_BLENDWEIGHT;
		case X_D3DVSDE_NORMAL      /*= 2*/:                 
			return D3DDECLUSAGE_NORMAL;
		case X_D3DVSDE_DIFFUSE     /*= 3*/:                 
			return D3DDECLUSAGE_COLOR;
		case X_D3DVSDE_SPECULAR    /*= 4*/: 
			UsageIndex = 1; 
			return D3DDECLUSAGE_COLOR;
		case X_D3DVSDE_FOG         /*= 5*/:                 
			return D3DDECLUSAGE_FOG;
		case X_D3DVSDE_POINTSIZE   /*= 6*/:                 
			return D3DDECLUSAGE_PSIZE;
		case X_D3DVSDE_BACKDIFFUSE /*= 7*/: 
			UsageIndex = 2; 
			return D3DDECLUSAGE_COLOR;
		case X_D3DVSDE_BACKSPECULAR/*= 8*/: 
			UsageIndex = 3; 
			return D3DDECLUSAGE_COLOR;
		case X_D3DVSDE_TEXCOORD0   /*= 9*/:                 
			return D3DDECLUSAGE_TEXCOORD;
		case X_D3DVSDE_TEXCOORD1   /*=10*/:
			UsageIndex = 1; 
			return D3DDECLUSAGE_TEXCOORD;
		case X_D3DVSDE_TEXCOORD2   /*=11*/: 
			UsageIndex = 2; 
			return D3DDECLUSAGE_TEXCOORD;
		case X_D3DVSDE_TEXCOORD3   /*=12*/: 
			UsageIndex = 3; 
			return D3DDECLUSAGE_TEXCOORD;
			// Unused registers. Assign them a semantic anyway so we don't have to worry about
			// binding a vertex declaration with them in it
		case 13: UsageIndex = 4; return D3DDECLUSAGE_TEXCOORD;
		case 14: UsageIndex = 5; return D3DDECLUSAGE_TEXCOORD;
		case 15: UsageIndex = 6; return D3DDECLUSAGE_TEXCOORD;
		default:
			return D3DDECLUSAGE_UNSUPPORTED;
		}
	}

	// VERTEX SHADER

	bool VshConvertToken_STREAMDATA_REG(DWORD VertexRegister, GuestVertexElement & slot)
	{
		DWORD XboxVertexElementDataType = slot.Format;

		// Does this attribute use no storage present the vertex (check this as early as possible to avoid needless processing) ?
		if (XboxVertexElementDataType == X_D3DVSDT_NONE) 
		{

			// Handle tessellating attributes
			switch (slot.TessellationType) {
			case 0: return false; // AUTONONE
			case 1: // AUTONORMAL
				// Note : .Stream, .Offset and .Type are copied from pAttributeSlot->TessellationSource in a post-processing step below,
				// because these could all go through an Xbox to host conversion step, so must be copied over afterwards.
				pCurrentHostVertexElement->Method = D3DDECLMETHOD_CROSSUV; // for D3DVSD_TESSNORMAL
				pCurrentHostVertexElement->Usage = D3DDECLUSAGE_NORMAL; // TODO : Is this correct?
				pCurrentHostVertexElement->UsageIndex = 0; // Note : 1 would be wrong
				return true;
			case 2: // AUTOTEXCOORD
				// pCurrentHostVertexElement->Stream = 0; // The input stream is unused (but must be set to 0), which is the current default value
				// pCurrentHostVertexElement->Offset = 0; // The input offset is unused (but must be set to 0), which is the current default value
				pCurrentHostVertexElement->Type = D3DDECLTYPE_UNUSED; // The input type for D3DDECLMETHOD_UV must be D3DDECLTYPE_UNUSED (the output type implied by D3DDECLMETHOD_UV is D3DDECLTYPE_FLOAT2)
				pCurrentHostVertexElement->Method = D3DDECLMETHOD_UV; // For X_D3DVSD_MASK_TESSUV
				pCurrentHostVertexElement->Usage = D3DDECLUSAGE_NORMAL; // Note : In Fixed Function Vertex Pipeline, D3DDECLMETHOD_UV must specify usage D3DDECLUSAGE_TEXCOORD or D3DDECLUSAGE_BLENDWEIGHT. TODO : So, what to do?
				pCurrentHostVertexElement->UsageIndex = 1; // TODO ; Is this correct?
				return true;
			default:
				printf("invalid TessellationType\n");
				return false;
			}
		}

		WORD XboxVertexElementByteSize = 0; // When set above 0, implies NeedPatching
		BYTE HostVertexElementDataType = 0;
		WORD HostVertexElementByteSize = 0;

		switch (XboxVertexElementDataType) 
		{
		case X_D3DVSDT_FLOAT1: // 0x12:
			HostVertexElementDataType = D3DDECLTYPE_FLOAT1;
			HostVertexElementByteSize = 1 * sizeof(FLOAT);
			break;
		case X_D3DVSDT_FLOAT2: // 0x22:
			HostVertexElementDataType = D3DDECLTYPE_FLOAT2;
			HostVertexElementByteSize = 2 * sizeof(FLOAT);
			break;
		case X_D3DVSDT_FLOAT3: // 0x32:
			HostVertexElementDataType = D3DDECLTYPE_FLOAT3;
			HostVertexElementByteSize = 3 * sizeof(FLOAT);
			break;
		case X_D3DVSDT_FLOAT4: // 0x42:
			HostVertexElementDataType = D3DDECLTYPE_FLOAT4;
			HostVertexElementByteSize = 4 * sizeof(FLOAT);
			break;
		case X_D3DVSDT_D3DCOLOR: // 0x40:
			HostVertexElementDataType = D3DDECLTYPE_D3DCOLOR;
			HostVertexElementByteSize = 1 * sizeof(DWORD);
			break;
		case X_D3DVSDT_SHORT2: // 0x25:
			HostVertexElementDataType = D3DDECLTYPE_SHORT2;
			HostVertexElementByteSize = 2 * sizeof(SHORT);
			break;
		case X_D3DVSDT_SHORT4: // 0x45:
			HostVertexElementDataType = D3DDECLTYPE_SHORT4;
			HostVertexElementByteSize = 4 * sizeof(SHORT);
			break;
		case X_D3DVSDT_NORMSHORT1: // 0x11:
			//if (g_D3DCaps.DeclTypes & D3DDTCAPS_SHORT2N) {
				HostVertexElementDataType = D3DDECLTYPE_SHORT2N;
				HostVertexElementByteSize = 2 * sizeof(SHORT);
			/** }
			else {
				HostVertexElementDataType = D3DDECLTYPE_FLOAT1;
				HostVertexElementByteSize = 1 * sizeof(FLOAT);
			}*/
			XboxVertexElementByteSize = 1 * sizeof(int16_t);
			break;
		case X_D3DVSDT_NORMSHORT2: // 0x21:
			//if (g_D3DCaps.DeclTypes & D3DDTCAPS_SHORT2N) {
				HostVertexElementDataType = D3DDECLTYPE_SHORT2N;
				HostVertexElementByteSize = 2 * sizeof(SHORT);
				// No need for patching in D3D9
			/** }
			else {
				HostVertexElementDataType = D3DDECLTYPE_FLOAT2;
				HostVertexElementByteSize = 2 * sizeof(FLOAT);
				XboxVertexElementByteSize = 2 * sizeof(int16_t);
			}*/
			break;
		case X_D3DVSDT_NORMSHORT3: // 0x31:
			//if (g_D3DCaps.DeclTypes & D3DDTCAPS_SHORT4N) {
				HostVertexElementDataType = D3DDECLTYPE_SHORT4N;
				HostVertexElementByteSize = 4 * sizeof(SHORT);
			//}
			/*/else {
				HostVertexElementDataType = D3DDECLTYPE_FLOAT3;
				HostVertexElementByteSize = 3 * sizeof(FLOAT);
			}*/
			XboxVertexElementByteSize = 3 * sizeof(int16_t);
			break;
		case X_D3DVSDT_NORMSHORT4: // 0x41:
			//if (g_D3DCaps.DeclTypes & D3DDTCAPS_SHORT4N) {
			{
				HostVertexElementDataType = D3DDECLTYPE_SHORT4N;
				HostVertexElementByteSize = 4 * sizeof(SHORT);
				// No need for patching in D3D9
			}
			/**else {
				HostVertexElementDataType = D3DDECLTYPE_FLOAT4;
				HostVertexElementByteSize = 4 * sizeof(FLOAT);
				XboxVertexElementByteSize = 4 * sizeof(int16_t);
			}*/
			break;
		case X_D3DVSDT_NORMPACKED3: // 0x16:
			HostVertexElementDataType = D3DDECLTYPE_FLOAT3;
			HostVertexElementByteSize = 3 * sizeof(FLOAT);
			XboxVertexElementByteSize = 1 * sizeof(DWORD);
			break;
		case X_D3DVSDT_SHORT1: // 0x15:
			HostVertexElementDataType = D3DDECLTYPE_SHORT2;
			HostVertexElementByteSize = 2 * sizeof(SHORT);
			XboxVertexElementByteSize = 1 * sizeof(SHORT);
			break;
		case X_D3DVSDT_SHORT3: // 0x35:
			HostVertexElementDataType = D3DDECLTYPE_SHORT4;
			HostVertexElementByteSize = 4 * sizeof(SHORT);
			XboxVertexElementByteSize = 3 * sizeof(SHORT);
			break;
		case X_D3DVSDT_PBYTE1: // 0x14:
			//if (g_D3DCaps.DeclTypes & D3DDTCAPS_UBYTE4N) {
			
				HostVertexElementDataType = D3DDECLTYPE_UBYTE4N;
				HostVertexElementByteSize = 4 * sizeof(BYTE);
			
			/**else {
				HostVertexElementDataType = D3DDECLTYPE_FLOAT1;
				HostVertexElementByteSize = 1 * sizeof(FLOAT);
			}*/
			XboxVertexElementByteSize = 1 * sizeof(BYTE);
			break;
		case X_D3DVSDT_PBYTE2: // 0x24:
			//if (g_D3DCaps.DeclTypes & D3DDTCAPS_UBYTE4N) {
			{
				HostVertexElementDataType = D3DDECLTYPE_UBYTE4N;
				HostVertexElementByteSize = 4 * sizeof(BYTE);
			}
			/**else {
				HostVertexElementDataType = D3DDECLTYPE_FLOAT2;
				HostVertexElementByteSize = 2 * sizeof(FLOAT);
			}*/
			XboxVertexElementByteSize = 2 * sizeof(BYTE);
			break;
		case X_D3DVSDT_PBYTE3: // 0x34:
			//if (g_D3DCaps.DeclTypes & D3DDTCAPS_UBYTE4N) {
				HostVertexElementDataType = D3DDECLTYPE_UBYTE4N;
				HostVertexElementByteSize = 4 * sizeof(BYTE);
			/**}
			else {
				HostVertexElementDataType = D3DDECLTYPE_FLOAT3;
				HostVertexElementByteSize = 3 * sizeof(FLOAT);
			}*/
			XboxVertexElementByteSize = 3 * sizeof(BYTE);
			break;
		case X_D3DVSDT_PBYTE4: // 0x44:
			// Test-case : Panzer
			//if (g_D3DCaps.DeclTypes & D3DDTCAPS_UBYTE4N) {
		{
				HostVertexElementDataType = D3DDECLTYPE_UBYTE4N;
				HostVertexElementByteSize = 4 * sizeof(BYTE);
				// No need for patching when PC supports D3DDECLTYPE_UBYTE4N
			}
			/**else {
				HostVertexElementDataType = D3DDECLTYPE_FLOAT4;
				HostVertexElementByteSize = 4 * sizeof(FLOAT);
				XboxVertexElementByteSize = 4 * sizeof(BYTE);
			}*/
			break;
		case X_D3DVSDT_FLOAT2H: // 0x72:
			HostVertexElementDataType = D3DDECLTYPE_FLOAT4;
			HostVertexElementByteSize = 4 * sizeof(FLOAT);
			XboxVertexElementByteSize = 3 * sizeof(FLOAT);
			break;
		case X_D3DVSDT_NONE: // 0x02:
			assert(false); // Should already be handled above
			break;
		default:
			printf("Unknown data type for D3DVSD_REG\n"); 
			return false;
		}

		assert(HostVertexElementDataType < D3DDECLTYPE_UNUSED);
		assert(HostVertexElementByteSize > 0);

		// Select new stream, if needed
		if ((pCurrentVertexShaderStreamInfo == nullptr)
			|| (pCurrentVertexShaderStreamInfo->XboxStreamIndex != slot.StreamIndex)) {
			assert(slot.StreamIndex < X_VSH_MAX_STREAMS);
			assert(pCurrentVertexDeclaration->NumberOfVertexStreams < X_VSH_MAX_STREAMS);

			pCurrentVertexShaderStreamInfo =
				&(pCurrentVertexDeclaration->VertexStreams[
					pCurrentVertexDeclaration->NumberOfVertexStreams++]);
			pCurrentVertexShaderStreamInfo->NeedPatch = FALSE;
			pCurrentVertexShaderStreamInfo->XboxStreamIndex = (WORD)slot.StreamIndex;
			pCurrentVertexShaderStreamInfo->HostVertexStride = (WORD)slot.Offset;
			pCurrentVertexShaderStreamInfo->NumberOfVertexElements = 0;
			// Dxbx note : Use Dophin(s), FieldRender, MatrixPaletteSkinning and PersistDisplay as a testcase
		}

		// save patching information
		assert(pCurrentVertexShaderStreamInfo->NumberOfVertexElements < X_VSH_MAX_ATTRIBUTES);

		HostVertexShaderStreamElement* pCurrentVertexShaderStreamElementInfo =
			&(pCurrentVertexShaderStreamInfo->VertexElements[
				pCurrentVertexShaderStreamInfo->NumberOfVertexElements++]);

		if (XboxVertexElementByteSize == 0) {
			XboxVertexElementByteSize = HostVertexElementByteSize;
		}
		else {
			pCurrentVertexShaderStreamInfo->NeedPatch |= TRUE;
		}

		pCurrentVertexShaderStreamElementInfo->XboxType = XboxVertexElementDataType;
		pCurrentVertexShaderStreamElementInfo->XboxByteSize = XboxVertexElementByteSize;
		pCurrentVertexShaderStreamElementInfo->HostDataType = HostVertexElementDataType;
		pCurrentVertexShaderStreamElementInfo->HostByteSize = HostVertexElementByteSize;

		// Convert to host vertex element
		pCurrentHostVertexElement->Stream = pCurrentVertexShaderStreamInfo->XboxStreamIndex; // Use Xbox stream index on host
		// FIXME Don't assume vertex elements are contiguous!
		pCurrentHostVertexElement->Offset = pCurrentVertexShaderStreamInfo->HostVertexStride;
		pCurrentHostVertexElement->Type = pCurrentVertexShaderStreamElementInfo->HostDataType;
		pCurrentHostVertexElement->Method = D3DDECLMETHOD_DEFAULT;
		if (IsFixedFunction) 
		{
			pCurrentHostVertexElement->Usage = Xb2PCRegisterType(VertexRegister, /*&*/pCurrentHostVertexElement->UsageIndex);
		}
		else {
			// D3DDECLUSAGE_TEXCOORD can be useds for any user-defined data
			// We need this because there is no reliable way to detect the real usage
			// Xbox has no concept of 'usage types', it only requires a list of attribute register numbers.
			// So we treat them all as 'user-defined' with an Index of the Vertex Register Index
			// this prevents information loss in shaders due to non-matching dcl types!
			pCurrentHostVertexElement->Usage = D3DDECLUSAGE_TEXCOORD;
			pCurrentHostVertexElement->UsageIndex = (BYTE)VertexRegister;
		}

		pCurrentVertexShaderStreamInfo->HostVertexStride += HostVertexElementByteSize;

		return true;
	}

public:
	D3DVERTEXELEMENT9* Convert(X_VERTEXATTRIBUTEFORMAT* pXboxDeclaration, bool bIsFixedFunction, HostVertexDeclaration* pVertexDeclaration)
	{
		// Get a preprocessed copy of the original Xbox Vertex Declaration
		pCurrentVertexDeclaration = pVertexDeclaration;

		IsFixedFunction = bIsFixedFunction;

		RegVIsPresentInDeclaration.fill(false);

		// Mapping between Xbox register and the resulting host vertex element
		D3DVERTEXELEMENT9* HostVertexElementPerRegister[X_VSH_MAX_ATTRIBUTES] = { 0 };

		// For Direct3D9, we need to reserve the maximum number of elements, plus one token for D3DDECL_END :
		unsigned HostDeclarationSize = (X_VSH_MAX_ATTRIBUTES + 1) * sizeof(D3DVERTEXELEMENT9);

		D3DVERTEXELEMENT9* HostVertexElements = (D3DVERTEXELEMENT9*)g_userHeap.AllocPhysical(HostDeclarationSize, 2);
		pCurrentHostVertexElement = HostVertexElements;

		std::array<byte, X_VSH_MAX_ATTRIBUTES> orderedRegisterIndices;
		for (byte i = 0; i < orderedRegisterIndices.size(); i++)
			orderedRegisterIndices[i] = i;

		// Make sure we convert registers in order of offset, per stream
		// TODO fix elements with identical positions?
		std::sort(orderedRegisterIndices.begin(), orderedRegisterIndices.end(),
			[pXboxDeclaration](const auto& x, const auto& y)
			{
				auto regX = pXboxDeclaration->Slots[x];
				auto regY = pXboxDeclaration->Slots[y];
				return std::tie(regX.StreamIndex, regX.Offset)
					< std::tie(regY.StreamIndex, regY.Offset);
			});

		printf("Parsing vertex declaration\n");
		for (size_t i = 0; i < orderedRegisterIndices.size(); i++) {
			auto regIndex = orderedRegisterIndices[i];
			auto& slot = pXboxDeclaration->Slots[regIndex];
			if (slot.Format > X_D3DVSDT_NONE) {
				// Set Direct3D9 vertex element (declaration) members :
				if (VshConvertToken_STREAMDATA_REG(regIndex, slot)) 
				{
					// Add this register to the list of declared registers
					RegVIsPresentInDeclaration[regIndex] = true;
					// Remember a pointer to this register
					HostVertexElementPerRegister[regIndex] = pCurrentHostVertexElement;
					pCurrentHostVertexElement++;
				}
			}
		}

		*pCurrentHostVertexElement = D3DDECL_END();

		// Post-process host vertex elements that have a D3DDECLMETHOD_CROSSUV method :
		for (int AttributeIndex = 0; AttributeIndex < X_VSH_MAX_ATTRIBUTES; AttributeIndex++) 
		{
			auto pHostElement = HostVertexElementPerRegister[AttributeIndex];
			if (pHostElement == nullptr) 
				continue;
			if (pHostElement->Method == D3DDECLMETHOD_CROSSUV) 
			{
				int TessellationSource = pXboxDeclaration->Slots[AttributeIndex].TessellationSource;
				auto pSourceElement = HostVertexElementPerRegister[TessellationSource];
				// Copy over the Stream, Offset and Type of the host vertex element that serves as 'TessellationSource' :
				pHostElement->Stream = pSourceElement->Stream;
				pHostElement->Offset = pSourceElement->Offset;
				pHostElement->Type = pSourceElement->Type;
				// Note, the input type for D3DDECLMETHOD_CROSSUV can be D3DDECLTYPE_FLOAT[43], D3DDECLTYPE_D3DCOLOR, D3DDECLTYPE_UBYTE4, or D3DDECLTYPE_SHORT4
				// (the output type implied by D3DDECLMETHOD_CROSSUV is D3DDECLTYPE_FLOAT3).
				// TODO : Should we assert this?
			}
		}

		// Ensure valid ordering of the vertex declaration (http://doc.51windows.net/Directx9_SDK/graphics/programmingguide/gettingstarted/vertexdeclaration/vertexdeclaration.htm)
		// In particular "All vertex elements for a stream must be consecutive and sorted by offset"
		// Note : Xbox slots might use non-ordered stream indices, so we can't rely on the output ordering of our converted elements!
		std::sort(/*First=*/HostVertexElements, /*Last=*/pCurrentHostVertexElement, /*Pred=*/[](const auto& x, const auto& y)
			{ return std::tie(x.Stream, x.Method, x.Offset) < std::tie(y.Stream, y.Method, y.Offset); });

		// Record which registers are in the vertex declaration
		for (size_t i = 0; i < RegVIsPresentInDeclaration.size(); i++) {
			pVertexDeclaration->vRegisterInDeclaration[i] = RegVIsPresentInDeclaration[i];
		}

		D3DVERTEXELEMENT9* copyVertex = HostVertexElements;

		return HostVertexElements;
	}
};


VertexDeclarationKey GetXboxVertexAttributesKey(X_VERTEXATTRIBUTEFORMAT* pXboxVertexAttributeFormat, bool isFixedFunction)
{

	auto attributeHash = XXH3_64bits((void*)pXboxVertexAttributeFormat, sizeof(X_VERTEXATTRIBUTEFORMAT));
	// For now, we use different declarations depending on if the fixed function pipeline
	// is in use, even if the attributes are the same
	return isFixedFunction ? attributeHash : attributeHash ^ 1;
}

//std::unordered_map<VertexDeclarationKey, HostVertexDeclaration*> g_HostVertexDeclarations;

static xxHashMap<HostVertexDeclaration*> g_HostVertexDeclarations;

static bool FreeHostVertexDeclaration(HostVertexDeclaration* pHostVertexDeclaration)
{
	if (pHostVertexDeclaration) 
	{
		g_userHeap.Free(pHostVertexDeclaration);
		return true;
	}

	return false;
}

void RegisterHostVertexDeclaration(VertexDeclarationKey CacheKey, HostVertexDeclaration* pHostVertexDeclaration)
{
	auto it = g_HostVertexDeclarations.find(CacheKey);
	if (it != g_HostVertexDeclarations.end() && it->second != nullptr) 
	{
		FreeHostVertexDeclaration(it->second); // Avoid memory leak
	}

	g_HostVertexDeclarations[CacheKey] = pHostVertexDeclaration;
}


HostVertexDeclaration* FetchCachedHostVertexDeclaration(VertexDeclarationKey CacheKey)
{
	auto it = g_HostVertexDeclarations.find(CacheKey);
	if (it != g_HostVertexDeclarations.end()) 
	{
		return it->second;
	}

	return nullptr;
}

D3DVERTEXELEMENT9* EmuRecompileVshDeclaration
(
	X_VERTEXATTRIBUTEFORMAT* pXboxDeclaration,
	bool                  bIsFixedFunction,
	HostVertexDeclaration* pVertexDeclaration
)
{
	XboxVertexDeclarationConverter Converter;

	D3DVERTEXELEMENT9* pHostVertexElements = Converter.Convert(pXboxDeclaration, bIsFixedFunction, pVertexDeclaration);

	return pHostVertexElements;
}

HostVertexDeclaration* HostGetVertexDeclaration(size_t handle, const bool isFixedFunction)
{
	X_VERTEXATTRIBUTEFORMAT* pXboxVertexAttributeFormat = GetXboxVertexAttributeFormat(handle);

	auto XboxVertexAttributesKey = GetXboxVertexAttributesKey(pXboxVertexAttributeFormat, isFixedFunction);
	HostVertexDeclaration* pHostVertexDeclaration = FetchCachedHostVertexDeclaration(XboxVertexAttributesKey);
	if (pHostVertexDeclaration == nullptr) 
	{	
		pHostVertexDeclaration = (HostVertexDeclaration*)g_userHeap.AllocPhysical<HostVertexDeclaration>();

		// Convert Xbox vertex attributes towards host Direct3D 9 vertex element
		D3DVERTEXELEMENT9* pRecompiledVertexElements = EmuRecompileVshDeclaration(
			pXboxVertexAttributeFormat,
			isFixedFunction,
			pHostVertexDeclaration);


		// store the vertex elements
		pHostVertexDeclaration->pHostVertexElements = pRecompiledVertexElements;

		//g_userHeap.Free(pRecompiledVertexElements);

	/*	auto vertexElement_ = pRecompiledVertexElements;
		printf("Guest Vertex elements logs\n");
		int index = 0;
		while (vertexElement_->Stream != 0xFF && vertexElement_->Type != D3DDECLTYPE_UNUSED)
		{
			printf("Stream[%d], %d\n", index, vertexElement_->Stream);
			printf("type[%d], %d\n", index, vertexElement_->Type);
			printf("Offset[%d], %d\n", index, vertexElement_->Offset);
			printf("Method[%d], %d\n", index, vertexElement_->Method);
			printf("Usage[%d], %d\n", index, vertexElement_->Usage);
			printf("Index[%d], %d\n", index, vertexElement_->UsageIndex);
			++vertexElement_;
			++index;
		}*/

		// Cache resulting declarations from given inputs
		pHostVertexDeclaration->Key = XboxVertexAttributesKey;
		RegisterHostVertexDeclaration(XboxVertexAttributesKey, pHostVertexDeclaration);
	}

	return pHostVertexDeclaration;
}
extern bool logIndexedPrimitive;
void SetVertexAttribute(int32_t Register, FLOAT a, FLOAT b, FLOAT c, FLOAT d);
void SetFixedFunctionDefaultVertexAttributes(size_t Handle) 
{

	DWORD vshFlags = GetXboxVertexShaderForHandle(Handle)->Flags;

	auto decl = HostGetVertexDeclaration(Handle, true);
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

GuestVertexDeclaration* CreateVertexDeclarationWithoutAddRef(D3DVERTEXELEMENT9* vertexElements, const bool isFixed)
{
	size_t vertexElementCount = 0;
	auto vertexElement = vertexElements;


	while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
	{

		++vertexElement;
		++vertexElementCount;
	}

	std::lock_guard lock(g_vertexDeclarationMutex);

	XXH64_hash_t hash = XXH3_64bits(vertexElements, vertexElementCount * sizeof(D3DVERTEXELEMENT9));
	auto& vertexDeclaration = g_vertexDeclarations[hash];


	if (vertexDeclaration == nullptr)
	{
		vertexDeclaration = g_userHeap.AllocPhysical<GuestVertexDeclaration>(ResourceType::VertexDeclaration);
		vertexDeclaration->hash = hash;

		static std::vector<RenderInputElement> inputElements;
		inputElements.clear();

		struct Location
		{
			uint32_t usage; //type pos, normal, blendweight etc.
			uint32_t usageIndex; //pos0, pos1, texcor0, texcord1 etc.
			uint32_t location; //actual location in the shader
		};

		constexpr Location locations[] =
		{
			{ D3DDECLUSAGE_TEXCOORD,     0, 0  },
			{ D3DDECLUSAGE_TEXCOORD,     1, 1  },
			{ D3DDECLUSAGE_TEXCOORD,     2, 2  },
			{ D3DDECLUSAGE_TEXCOORD,     3, 3  },
			{ D3DDECLUSAGE_TEXCOORD,     4, 4  },
			{ D3DDECLUSAGE_TEXCOORD,     5, 5  },
			{ D3DDECLUSAGE_TEXCOORD,     6, 6 },
			{ D3DDECLUSAGE_TEXCOORD,     7, 7 },
			{ D3DDECLUSAGE_TEXCOORD,     8, 8 },
			{ D3DDECLUSAGE_TEXCOORD,     9, 9 },
			{ D3DDECLUSAGE_TEXCOORD,    10, 10 },
			{ D3DDECLUSAGE_TEXCOORD,    11, 11 },
			{ D3DDECLUSAGE_TEXCOORD,    12, 12 },
			{ D3DDECLUSAGE_TEXCOORD,    13, 13 },
			{ D3DDECLUSAGE_TEXCOORD,    14, 14 },
			{ D3DDECLUSAGE_TEXCOORD,    15, 15 },
		};

		constexpr Location locationsFixedFunc[] =
		{
			{ D3DDECLUSAGE_POSITION,     0, 0  },
			{ D3DDECLUSAGE_BLENDWEIGHT,  0, 1  },
			{ D3DDECLUSAGE_NORMAL,		 0, 2  },
			{ D3DDECLUSAGE_COLOR,		 0, 3  },
			{ D3DDECLUSAGE_COLOR,		 1, 4  },
			{ D3DDECLUSAGE_FOG,			 0, 5  },
			{ D3DDECLUSAGE_PSIZE,		 0, 6  },

			//backColor
			{ D3DDECLUSAGE_TEXCOORD,     4, 7  },
			{ D3DDECLUSAGE_TEXCOORD,     5, 8  },

			//actual texcoord
			{ D3DDECLUSAGE_TEXCOORD,     0, 9  },
			{ D3DDECLUSAGE_TEXCOORD,     1, 10  },
			{ D3DDECLUSAGE_TEXCOORD,     2, 11 },
			{ D3DDECLUSAGE_TEXCOORD,     3, 12 },

			//reserved
			{ D3DDECLUSAGE_TEXCOORD,     6, 13 },
			{ D3DDECLUSAGE_TEXCOORD,     7, 14 },
			{ D3DDECLUSAGE_TEXCOORD,     8, 15 },

		};


		vertexElement = vertexElements;
		while (vertexElement->Stream != 0xFF && vertexElement->Type != D3DDECLTYPE_UNUSED)
		{
			if (vertexElement->Usage == D3DDECLUSAGE_POSITION && vertexElement->UsageIndex == 2)
			{
				++vertexElement;
				continue;
			}

			auto& inputElement = inputElements.emplace_back();

			inputElement.semanticName = ConvertDeclUsage(vertexElement->Usage);
			inputElement.semanticIndex = vertexElement->UsageIndex;
			inputElement.location = ~0;

			if (isFixed)
			{
				for (auto& location : locationsFixedFunc)
				{
					if (location.usage == vertexElement->Usage && location.usageIndex == vertexElement->UsageIndex)
					{
						inputElement.location = location.location;
						break;
					}
				}
			}
			else
			{
				for (auto& location : locations)
				{
					if (location.usage == vertexElement->Usage && location.usageIndex == vertexElement->UsageIndex)
					{
						inputElement.location = location.location;
						break;
					}
				}
			}


			assert(inputElement.location != ~0);

			inputElement.format = ConvertDeclType(vertexElement->Type);
			inputElement.slotIndex = vertexElement->Stream;
			inputElement.alignedByteOffset = vertexElement->Offset;

			switch (vertexElement->Usage)
			{
			case D3DDECLUSAGE_POSITION:
				if (vertexElement->UsageIndex == 1)
					vertexDeclaration->indexVertexStream = vertexElement->Stream;
				break;

			case D3DDECLUSAGE_NORMAL:
			case D3DDECLUSAGE_TANGENT:
			case D3DDECLUSAGE_BINORMAL:
				if (vertexElement->Type == D3DDECLTYPE_FLOAT3)
					inputElement.format = RenderFormat::R32G32B32_UINT;
				else
					vertexDeclaration->hasR11G11B10Normal = true;
				break;

			case D3DDECLUSAGE_TEXCOORD:
				switch (vertexElement->Type)
				{
				case D3DDECLTYPE_SHORT2:
				case D3DDECLTYPE_SHORT4:
				case D3DDECLTYPE_SHORT2N:
				case D3DDECLTYPE_SHORT4N:
				case D3DDECLTYPE_USHORT2N:
				case D3DDECLTYPE_USHORT4N:
				case D3DDECLTYPE_FLOAT16_2:
				case D3DDECLTYPE_FLOAT16_4:
					vertexDeclaration->swappedTexcoords |= 1 << vertexElement->UsageIndex;
					break;
				}

				break;
			}

			vertexDeclaration->vertexStreams[vertexElement->Stream] = true;

			++vertexElement;
		}

		auto addInputElement = [&](uint32_t usage, uint32_t usageIndex)
			{
				uint32_t location = ~0;

				if (isFixed)
				{
					for (auto& alsoLocation : locationsFixedFunc)
					{
						if (alsoLocation.usage == usage && alsoLocation.usageIndex == usageIndex)
						{
							location = alsoLocation.location;
							break;
						}
					}
				}
				else
				{
					for (auto& alsoLocation : locations)
					{
						if (alsoLocation.usage == usage && alsoLocation.usageIndex == usageIndex)
						{
							location = alsoLocation.location;
							break;
						}
					}
				}

				assert(location != ~0);

				for (auto& inputElement : inputElements)
				{
					if (inputElement.location == location)
						return;
				}

				auto format = RenderFormat::R32_FLOAT;
				switch (usage)
				{
				case D3DDECLUSAGE_NORMAL:
				case D3DDECLUSAGE_TANGENT:
				case D3DDECLUSAGE_BINORMAL:
				case D3DDECLUSAGE_BLENDINDICES:
					format = RenderFormat::R32_UINT;
					break;
				}

				inputElements.emplace_back(ConvertDeclUsage(usage), usageIndex, location, format, 15, 0);
			};

		// Assign any unbound usages to null buffer slot.
		if (isFixed)
		{
			for (auto& location : locationsFixedFunc)
			{
				addInputElement(location.usage, location.usageIndex);
			}
		}
		else
		{
			for (auto& location : locations)
			{
				addInputElement(location.usage, location.usageIndex);
			}
		}


		vertexDeclaration->inputElements = std::make_unique<RenderInputElement[]>(inputElements.size());
		std::copy(inputElements.begin(), inputElements.end(), vertexDeclaration->inputElements.get());

		vertexDeclaration->vertexElements = std::make_unique<D3DVERTEXELEMENT9[]>(vertexElementCount + 1);
		std::copy(vertexElements, vertexElements + vertexElementCount + 1, vertexDeclaration->vertexElements.get());

		vertexDeclaration->inputElementCount = uint32_t(inputElements.size());
		vertexDeclaration->vertexElementCount = vertexElementCount + 1;
	}

	vertexDeclaration->AddRef();
	//vertexDeclaration->uniqueID = ++uniqueIDG;
	return vertexDeclaration;
}