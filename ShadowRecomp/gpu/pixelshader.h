

void UpdatePixelShaderConstant(uint32_t(*psh_constants)[4]);
void UpdatePixelShaderCacheEntryNew(GuestShader* shader, const uint64_t key);
uint64_t GetPixelShaderKey(const uint32_t pXboxPixelShaderDef);
GuestShader* FindPixelShaderCacheEntryNew(const uint32_t pXboxPixelShaderDef);
const uint8_t* CreateRecompiledPixelShader(X_D3DPIXELSHADERDEF* PSDef, void* psGuestInf, size_t* outSize);