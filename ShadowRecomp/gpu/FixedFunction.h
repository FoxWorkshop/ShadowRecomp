void UpdateFixedFunctionVertexShaderState(size_t Handle, bool isPSFixed);

extern std::array<bool, 4> bWorldViewDirty;


struct Material
{
	D3DCOLORVALUE Diffuse; // base for Materials[0], these will be array-offsetted
	D3DCOLORVALUE Ambient;
	D3DCOLORVALUE Specular;
	D3DCOLORVALUE Emissive;
	D3DCOLORVALUE Power; // c110.w/y/z free
};
static_assert(sizeof(Material) % 16 == 0, "Material must be multiple of 16 bytes");

void UpdateMaterial(Material* pMaterial);
void UpdateLightData(uint32_t index, X_D3DLIGHT8* pLight);