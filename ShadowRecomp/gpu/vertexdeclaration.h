


typedef uint64_t VertexDeclarationKey;

typedef struct _HostVertexShaderStreamElement
{
	UINT XboxType; // The stream element data type (xbox)
	UINT XboxByteSize; // The stream element data size (xbox)
	BYTE HostDataType; // The stream element data type (pc)
	UINT HostByteSize; // The stream element data size (pc)
}
HostVertexShaderStreamElement;

typedef struct _HostVertexShaderStreamInfo
{
	BOOL  NeedPatch;       // This is to know whether it's data which must be patched
	WORD HostVertexStride;
	DWORD NumberOfVertexElements;        // Number of the stream data types
	WORD XboxStreamIndex;
	HostVertexShaderStreamElement VertexElements[X_VSH_MAX_ATTRIBUTES];
}
HostVertexShaderStreamInfo;




typedef struct _HostVertexDeclaration
{
	VertexDeclarationKey Key;
	HostVertexShaderStreamInfo VertexStreams[X_VSH_MAX_STREAMS]; // Note : VertexStreams is indexed by a counter, NOT StreamIndex!
	D3DVERTEXELEMENT9* pHostVertexElements;
	UINT NumberOfVertexStreams; // The number of streams the vertex shader uses
	bool vRegisterInDeclaration[X_VSH_MAX_ATTRIBUTES];
}
HostVertexDeclaration;

// ******************************************************************
struct X_VERTEXATTRIBUTEFORMAT
{
	// Note : Alignment looks okay even without #pragma pack(1) / #include "AlignPrefix1.h" (and it's closure)
	GuestVertexElement Slots[X_VSH_MAX_ATTRIBUTES];
};


HostVertexDeclaration* HostGetVertexDeclaration(size_t handle, const bool isFixedFunction);
GuestVertexDeclaration* CreateVertexDeclarationWithoutAddRef(D3DVERTEXELEMENT9* vertexElements, VertexShaderMode shaderMode);
void SetFixedFunctionDefaultVertexAttributes(size_t Handle);
X_VERTEXATTRIBUTEFORMAT* GetXboxVertexAttributeFormat(size_t handle);

