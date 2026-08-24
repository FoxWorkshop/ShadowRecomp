#include "fixed_function_pixel_shader_variants.h"
#include "ubershader_catalog.h"
#include "gpu/shared_constants.h"
#include <rapidhash.h>
#include <cstring>
#include "shader/fixed_function_pixel_shader/variants/generated/fixed_function_pixel_shader_variants.generated.h"

namespace
{
    // Generated catalog for the fixed-function pixel shader family.
    struct PixelShaderFamily
    {
        static constexpr auto& States = FixedFunctionPixelShaderVariants::Generated::States;
        static constexpr auto& Programs = FixedFunctionPixelShaderVariants::Generated::Programs;
        static constexpr const char* Name = "FixedFunctionPixelShader";

        static uint64_t Hash(const FixedFunctionPixelShaderState& state)
        {
            const uint64_t hash = rapidhash(&state, sizeof(state));
            return hash != 0 ? hash : 1; // 0 is special cased for the fallback.
        }

        static const FixedFunctionPixelShaderState& Normalize(const FixedFunctionPixelShaderState& state)
        {
            return state;
        }
    };

    static UbershaderCatalog<PixelShaderFamily> g_catalog;
}

namespace FixedFunctionPixelShaderVariants
{
    void Initialize(plume::RenderDevice* device, bool vulkan)
    {
        g_catalog.Initialize(device, vulkan);
    }

    FixedFunctionPixelShaderState Capture(const SharedConstants& constants)
    {
        FixedFunctionPixelShaderState state{};
        memcpy(state.color_op, constants.color_op, sizeof(state.color_op));
        memcpy(state.color_arg0, constants.color_arg0, sizeof(state.color_arg0));
        memcpy(state.color_arg1, constants.color_arg1, sizeof(state.color_arg1));
        memcpy(state.color_arg2, constants.color_arg2, sizeof(state.color_arg2));
        memcpy(state.alpha_op, constants.alpha_op, sizeof(state.alpha_op));
        memcpy(state.alpha_arg0, constants.alpha_arg0, sizeof(state.alpha_arg0));
        memcpy(state.alpha_arg1, constants.alpha_arg1, sizeof(state.alpha_arg1));
        memcpy(state.alpha_arg2, constants.alpha_arg2, sizeof(state.alpha_arg2));
        memcpy(state.result_arg, constants.result_arg, sizeof(state.result_arg));
        memcpy(state.texture_sample_type, constants.TextureSampleType, sizeof(state.texture_sample_type));
        state.specular_enable = constants.SpecularEnable;
        state.fog_enable = constants.FogEnable;
        state.fog_table_mode = constants.FogTableMode;
        state.alpha_test_enable = constants.alphaTestEnable;
        state.alpha_func = constants.alphaFunc;
        return state;
    }

    ShaderVariantKey Select(const FixedFunctionPixelShaderState& state)
    {
        return g_catalog.Select(state);
    }

    bool Contains(ShaderVariantKey key)
    {
        return g_catalog.Contains(key);
    }

    plume::RenderShader* Resolve(ShaderVariantKey key)
    {
        return g_catalog.Resolve(key);
    }
}
