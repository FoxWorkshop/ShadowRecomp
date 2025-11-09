#include "video.h"

typedef void(*FormatToARGBRow)(const uint8_t* src, uint8_t* dst_argb, int width);

BOOL FormatIsSwizzled(X_D3DFORMAT Format);

void UnswizzleBox
(
	CONST PVOID pSrcBuff,
	CONST DWORD dwWidth,
	CONST DWORD dwHeight,
	CONST DWORD dwDepth,
	CONST DWORD dwBytesPerPixel,
	CONST PVOID pDstBuff,
	CONST DWORD dwDstRowPitch,
	CONST DWORD dwDstSlicePitch
);

DWORD XBFormatBitsPerPixel(X_D3DFORMAT Format);
BOOL XBFormatIsCompressed(X_D3DFORMAT Format);

bool XBFormatRequiresConversionToARGB(X_D3DFORMAT Format);

bool ConvertD3DTextureToARGBBuffer(
	X_D3DFORMAT X_Format,
	uint8_t* pSrc,
	int32_t SrcWidth, int32_t SrcHeight, int32_t SrcRowPitch, int32_t SrcSlicePitch,
	uint8_t* pDst, int32_t DstRowPitch, int32_t DstSlicePitch,
	uint32_t uiDepth,
	void* pPalleteData
);

BOOL FormatIsLinear(X_D3DFORMAT Format);
BOOL FormatIsDepthBuffer(X_D3DFORMAT Format);
BOOL FormatIsRenderTarget(X_D3DFORMAT Format);
