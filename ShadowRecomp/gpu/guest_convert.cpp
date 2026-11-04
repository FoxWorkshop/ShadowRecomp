#include "video.h"
#include "guest_convert.h"

RenderFormat ConvertFormat(X_D3DFORMAT format)
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

		// Those are not used by the game afaik, but this will help modding
	case X_D3DFMT_DXT1:
		return RenderFormat::BC1_UNORM;
	case X_D3DFMT_DXT3:
		return RenderFormat::BC2_UNORM;
	case X_D3DFMT_DXT5:
		return RenderFormat::BC3_UNORM;

	case X_D3DFMT_LIN_D24S8: //Linear 24-bit depth + 8-bit stencil

		return RenderFormat::D32_FLOAT;
	default:

		assert(false && "Unknown format");
		return RenderFormat::R8_UNORM;
	}
}


RenderTextureAddressMode ConvertTextureAddressMode(size_t value)
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

RenderFilter ConvertTextureFilter(uint32_t value)
{
	switch (value)
	{
	case X_D3DTEXF_POINT:
	case X_D3DTEXF_NONE:
		return RenderFilter::NEAREST;
	case X_D3DTEXF_LINEAR:
	case X_D3DTEXF_ANISOTROPIC:
		return RenderFilter::LINEAR;
	default:
		assert(false && "Unknown texture filter");
		return RenderFilter::UNKNOWN;
	}
}

RenderBorderColor ConvertBorderColor(uint32_t value)
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

bool isCapabilityTriangleFan();
RenderPrimitiveTopology ConvertPrimitiveType(uint32_t primitiveType)
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
		return isCapabilityTriangleFan() ? RenderPrimitiveTopology::TRIANGLE_FAN : RenderPrimitiveTopology::TRIANGLE_LIST;
		//todo check if this is correct:
	case X_D3DPT_QUADSTRIP:
		return RenderPrimitiveTopology::TRIANGLE_STRIP;
	case X_D3DPT_POLYGON:
		return isCapabilityTriangleFan() ?
			RenderPrimitiveTopology::TRIANGLE_FAN :
			RenderPrimitiveTopology::TRIANGLE_LIST;
	default:
		assert(false && "Unknown primitive type");
		return RenderPrimitiveTopology::UNKNOWN;
	}
}

RenderBlend ConvertBlendMode(uint32_t blendMode)
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

RenderBlend ConvertAlphaBlendMode(uint32_t blendMode)
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

RenderBlendOperation ConvertBlendOp(uint32_t blendOp)
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

RenderComparisonFunction ConvertD3DCMP(uint32_t value)
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


RenderStencilOp ConvertStencilOp(uint32_t stencilOp)
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