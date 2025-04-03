#pragma once

#include <cpu/x86_context.h>
#include <array>
#include "memory.h"

template <typename R, typename... T>
constexpr std::tuple<T...> function_args(R(*)(T...)) noexcept
{
    return std::tuple<T...>();
}

template<auto V>
static constexpr decltype(V) constant_v = V;

template<typename T>
static constexpr bool is_precise_v = std::is_same_v<T, float> || std::is_same_v<T, double>;

template<auto Func>
struct arg_count_t
{
    static constexpr size_t value = std::tuple_size_v<decltype(function_args(Func))>;
};

template<typename TCallable, int I = 0, typename ...TArgs>
std::enable_if_t<(I >= sizeof...(TArgs)), void> _tuple_for(std::tuple<TArgs...>&, const TCallable& callable) noexcept
{
}

template<typename TCallable, int I = 0, typename ...TArgs>
std::enable_if_t<(I < sizeof...(TArgs)), void> _tuple_for(std::tuple<TArgs...>& tpl, const TCallable& callable) noexcept
{
    callable(std::get<I>(tpl), I);
    _tuple_for<TCallable, I + 1>(tpl, callable);
}

struct ArgTranslator
{
    constexpr static uint32_t GetIntegerArgumentValue(const X86Context& ctx, uint8_t* base, size_t arg) noexcept
    {
        // Calculate offset - this part is constexpr-valid
        const uint32_t offset = ctx.esp.u32 + 4 + (arg * 4);

        // In constexpr context, return 0 or some safe value
        if (std::is_constant_evaluated()) {
            return 0; // Or other appropriate default
        }

        // Runtime version does the actual memory access
        return X86_LOAD_U32(offset);
    }

    static double GetPrecisionArgumentValue(const X86Context& ctx, uint8_t* base, size_t arg) noexcept
    {
        if (arg < 4)
        {
            //Fall back to stack if not in registers
            return ctx.vec[arg].f32[0];
        }
        // Otherwise get from stack
        uint32_t val = X86_LOAD_U32(ctx.esp.u32 + 4 + (arg * 4));
        return *reinterpret_cast<double*>(&val);
    }

    constexpr static void SetIntegerArgumentValue(X86Context& ctx, uint8_t* base, size_t arg, uint32_t value) noexcept
    {
        // Return values go in eax
        if (arg == 0) {
            ctx.eax.u32 = value;
        }
    }

    static void SetPrecisionArgumentValue(X86Context& ctx, uint8_t* base, size_t arg, double value) noexcept
    {
        // Floating point return values go in xmm0 or st(0)
        if (arg == 0) {
            ctx.vec[0].f32[0] = static_cast<float>(value);
            ctx.fpu.st0() = value; // Also set in x87 for compatibility
        }
    }

    template<typename T>
    constexpr static std::enable_if_t<!std::is_pointer_v<T>, T> GetValue(X86Context& ctx, uint8_t* base, size_t idx) noexcept
    {
        if constexpr (is_precise_v<T>)
        {
            return static_cast<T>(GetPrecisionArgumentValue(ctx, base, idx));
        }
        else
        {
            return static_cast<T>(GetIntegerArgumentValue(ctx, base, idx));
        }
    }

    template<typename T>
    constexpr static std::enable_if_t<std::is_pointer_v<T>, T> GetValue(X86Context& ctx, uint8_t* base, size_t idx) noexcept
    {
        const auto v = GetIntegerArgumentValue(ctx, base, idx);
        if (!v)
        {
            return nullptr;
        }
        return reinterpret_cast<T>(base + static_cast<uint32_t>(v));
    }

    template<typename T>
    constexpr static std::enable_if_t<!std::is_pointer_v<T>, void> SetValue(X86Context& ctx, uint8_t* base, size_t idx, T value) noexcept
    {
        if constexpr (is_precise_v<T>)
        {
            SetPrecisionArgumentValue(ctx, base, idx, value);
        }
        else if constexpr (std::is_null_pointer_v<T>)
        {
            SetIntegerArgumentValue(ctx, base, idx, 0);
        }
        else if constexpr (std::is_pointer_v<T>)
        {
            SetIntegerArgumentValue(ctx, base, idx, g_memory.MapVirtual(value));
        }
        else
        {
            SetIntegerArgumentValue(ctx, base, idx, value);
        }
    }

    template<typename T>
    constexpr static std::enable_if_t<std::is_pointer_v<T>, void> SetValue(X86Context& ctx, uint8_t* base, size_t idx, T value) noexcept
    {
        const auto v = g_memory.MapVirtual((void*)value);
        if (!v)
        {
            return;
        }
        SetValue(ctx, base, idx, v);
    }
};

struct Argument
{
    int type{};  // 0 = integer, 1 = float
    int ordinal{};
};

template<typename T1>
constexpr std::array<Argument, std::tuple_size_v<T1>> GatherFunctionArguments(const T1& tpl)
{
    std::array<Argument, std::tuple_size_v<T1>> args{};
    size_t i{};

    if constexpr (!args.empty())
    {
        std::apply([&](const auto& first, const auto&... rest)
            {
                auto append = [&]<typename T2>(const T2 & v)
                {
                    if constexpr (is_precise_v<T2>)
                    {
                        args[i] = { 1, static_cast<int>(i) };
                    }
                    else
                    {
                        args[i] = { 0, static_cast<int>(i) };
                    }
                    i++;
                };

                append(first);
                (append(rest), ...);
            }, tpl);
    }

    return args;
}

template<auto Func>
constexpr std::array<Argument, arg_count_t<Func>::value> GatherFunctionArguments()
{
    return GatherFunctionArguments(function_args(Func));
}

template<auto Func, size_t I>
struct arg_ordinal_t
{
    static constexpr size_t value = GatherFunctionArguments<Func>()[I].ordinal;
};

template<auto Func, int I = 0, typename ...TArgs>
void _translate_args_to_host(X86Context& ctx, uint8_t* base, std::tuple<TArgs...>&) noexcept
    requires (I >= sizeof...(TArgs))
{
}

template <auto Func, int I = 0, typename ...TArgs>
std::enable_if_t<(I < sizeof...(TArgs)), void> _translate_args_to_host(X86Context& ctx, uint8_t* base, std::tuple<TArgs...>& tpl) noexcept
{
    using T = std::tuple_element_t<I, std::remove_reference_t<decltype(tpl)>>;
    std::get<I>(tpl) = ArgTranslator::GetValue<T>(ctx, base, arg_ordinal_t<Func, I>::value);

    _translate_args_to_host<Func, I + 1>(ctx, base, tpl);
}

template<int I = 0, typename ...TArgs>
void _translate_args_to_guest(X86Context& ctx, uint8_t* base, std::tuple<TArgs...>&) noexcept
    requires (I >= sizeof...(TArgs))
{
}

template <int I = 0, typename ...TArgs>
std::enable_if_t<(I < sizeof...(TArgs)), void> _translate_args_to_guest(X86Context& ctx, uint8_t* base, std::tuple<TArgs...>& tpl) noexcept
{
    using T = std::tuple_element_t<I, std::remove_reference_t<decltype(tpl)>>;
    ArgTranslator::SetValue<T>(ctx, base, GatherFunctionArguments(std::tuple<TArgs...>{})[I].ordinal, std::get<I>(tpl));

    _translate_args_to_guest<I + 1>(ctx, base, tpl);
}

template<auto Func>
X86_FUNC(HostToGuestFunction)
{
    /**using ret_t = decltype(std::apply(Func, function_args(Func)));

    auto args = function_args(Func);
    _translate_args_to_host<Func>(ctx, base, args);

    if constexpr (std::is_same_v<ret_t, void>)
    {
        std::apply(Func, args);
    }
    else
    {
        auto v = std::apply(Func, args);

        if constexpr (std::is_pointer<ret_t>())
        {
            if (v != nullptr)
            {
                ctx.eax.u32 = static_cast<uint32_t>(reinterpret_cast<size_t>(v) - reinterpret_cast<size_t>(base));
            }
            else
            {
                ctx.eax.u32 = 0;
            }
        }
        else if constexpr (is_precise_v<ret_t>)
        {
            ctx.vec[0].f32[0] = static_cast<float>(v);
            ctx.fpu.st0() = v; // Set in both x87 and SSE for compatibility
        }
        else
        {
            ctx.eax.u32 = (uint32_t)v;
        }
    }*/
}

template<typename T, typename TFunction, typename... TArgs>
T GuestToHostFunction(const TFunction& func, TArgs&&... argv)
{
    auto args = std::make_tuple(std::forward<TArgs>(argv)...);
    auto& currentCtx = *GetX86Context();

    X86Context newCtx;
    newCtx.esp = currentCtx.esp; // Maintain stack pointer
    newCtx.ebp = currentCtx.ebp; // Maintain base pointer
    newCtx.mxcsr = currentCtx.mxcsr;

    _translate_args_to_guest(newCtx, g_memory.base, args);

    SetX86Context(newCtx);

    if constexpr (std::is_function_v<TFunction>)
        func(newCtx, g_memory.base);
    else
        g_memory.FindFunction(func)(newCtx, g_memory.base);

    currentCtx.mxcsr = newCtx.mxcsr;
    SetX86Context(currentCtx);

    if constexpr (std::is_pointer_v<T>)
    {
        return reinterpret_cast<T>((uint64_t)g_memory.Translate(newCtx.eax.u32));
    }
    else if constexpr (is_precise_v<T>)
    {
        // Prefer SSE value if available, fall back to x87
        if (newCtx.vec[0].f32[0] != 0.0f) {
            return static_cast<T>(newCtx.vec[0].f32[0]);
        }
        return static_cast<T>(newCtx.fpu.st0);
    }
    else if constexpr (std::is_integral_v<T>)
    {
        return static_cast<T>(newCtx.eax.u32);
    }
    else
    {
        static_assert(std::is_void_v<T>, "Unsupported return type.");
    }
}

#define GUEST_FUNCTION_HOOK(subroutine, function) \
    X86_FUNC(subroutine) { HostToGuestFunction<function>(ctx, base); }

#define GUEST_FUNCTION_STUB(subroutine) \
    X86_FUNC(subroutine) { }