void UpdateFixedFunctionVertexShaderState(size_t Handle, bool isPSFixed);

extern std::array<bool, 4> bWorldViewDirty;


struct Material
{
	alignas(16) D3DCOLORVALUE Diffuse; // base for Materials[0], these will be array-offsetted
	alignas(16) D3DCOLORVALUE Ambient;
	alignas(16) D3DCOLORVALUE Specular;
	alignas(16) D3DCOLORVALUE Emissive;
	alignas(16) D3DCOLORVALUE Power; // c110.w/y/z free
};


void UpdateMaterial(Material* pMaterial);
void UpdateLightData(uint32_t index, X_D3DLIGHT8* pLight);