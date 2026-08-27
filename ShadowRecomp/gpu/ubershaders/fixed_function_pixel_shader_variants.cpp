#include "fixed_function_pixel_shader_variants.h"
#include "ubershader_catalog.h"
#include "gpu/shared_constants.h"
#include <rapidhash.h>
#include <cassert>
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
        // A source value above 255 would wrap and merge distinct states.
        auto narrow = [](uint32_t value)
        {
            assert(value <= UINT8_MAX);
            return static_cast<uint8_t>(value);
        };

        for (uint32_t i = 0; i < FixedFunctionPixelShaderState::TextureStageCount; i++)
        {
            state.color_op[i] = narrow(constants.color_op[i]);
            state.color_arg0[i] = narrow(constants.color_arg0[i]);
            state.color_arg1[i] = narrow(constants.color_arg1[i]);
            state.color_arg2[i] = narrow(constants.color_arg2[i]);
            state.alpha_op[i] = narrow(constants.alpha_op[i]);
            state.alpha_arg0[i] = narrow(constants.alpha_arg0[i]);
            state.alpha_arg1[i] = narrow(constants.alpha_arg1[i]);
            state.alpha_arg2[i] = narrow(constants.alpha_arg2[i]);
            state.result_arg[i] = narrow(constants.result_arg[i]);
            state.texture_sample_type[i] = narrow(constants.TextureSampleType[i]);
        }
        state.specular_enable = narrow(constants.SpecularEnable);
        state.fog_enable = narrow(constants.FogEnable);
        state.fog_table_mode = narrow(constants.FogTableMode);
        state.alpha_test_enable = narrow(constants.alphaTestEnable);
        state.alpha_func = narrow(constants.alphaFunc);
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
