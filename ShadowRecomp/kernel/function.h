#pragma once
#include "cpu/x86_context.h"
#include "memory.h"
#include <tuple>
#include <array>
#include "xbox_types.h"
#include "types.h"
#include "heap.h"
#include <winnt.h>
#include <minwindef.h>
#include <Windows.h>
#include <windef.h>

// Function type definitions
typedef void (*XbeFunction)(X86Context& ctx, uint8_t* base);
typedef void (*XbeOriginalFn)();

// Argument extraction utilities
template <typename R, typename... T>
constexpr std::tuple<T...> function_args(R(*)(T...)) noexcept {
    return std::tuple<T...>();
}

template<auto V>
static constexpr decltype(V) constant_v = V;

template<typename T>
static constexpr bool is_precise_v = std::is_same_v<T, float> || std::is_same_v<T, double>;

template<auto Func>
struct arg_count_t {
    static constexpr size_t value = std::tuple_size_v<decltype(function_args(Func))>;
};

// X86-specific argument translator
struct ArgTranslator {
    // Get arguments from X86 context (CDECL calling convention)
    template<typename T>
    static T GetValue(X86Context& ctx, uint8_t* base, size_t idx) 
    {
        if constexpr (is_precise_v<T>) {
            // Floating point args in XMM0-XMM3 (Windows) or XMM0-XMM7 (System V)
            if (idx < 8) {
                return ctx.vec[idx].f32[0];
            }
            // For stack-passed floats
            return *reinterpret_cast<float*>(base + ctx.esp.u32 + 4 + (idx - 8) * 4);
        }
        else if constexpr (std::is_pointer_v<T>) {
            // Integer/pointer args in ECX, EDX, then stack
            uint32_t val;
            if (idx == 0) val = ctx.ecx.u32;
            else if (idx == 1) val = ctx.edx.u32;
            else val = *reinterpret_cast<uint32_t*>(base + ctx.esp.u32 + 4 + (idx - 2) * 4);

            return val ? reinterpret_cast<T>(base + val) : nullptr;
        }
        else {
            // Integer args
            uint32_t val;
            if (idx == 0) 
                val = ctx.ecx.u32;
            else if (idx == 1) 
                val = ctx.edx.u32;
            else 
                val = *reinterpret_cast<uint32_t*>(base + ctx.esp.u32 + 4 + (idx - 2) * 4);

            return static_cast<T>(val);
        }
    }

    // Set arguments in context
    template<typename T>
    static void SetValue(X86Context& ctx, size_t idx, T value) {
        if constexpr (is_precise_v<T>) {
            if (idx < 8) {
                ctx.vec[idx].f32[0] = value;
            }
            else {
                // Stack-passed floats not implemented
                assert(false && "Stack-passed floats not supported");
            }
        }
        else if constexpr (std::is_pointer_v<T>) {
            uint32_t val = value ? g_memory.MapVirtual(value) : 0;
            if (idx == 0) ctx.ecx.u32 = val;
            else if (idx == 1) ctx.edx.u32 = val;
            else {
                *reinterpret_cast<uint32_t*>(ctx.base + ctx.esp.u32 + 4 + (idx - 2) * 4) = val;
            }
        }
        else {
            if (idx == 0) ctx.ecx.u32 = static_cast<uint32_t>(value);
            else if (idx == 1) ctx.edx.u32 = static_cast<uint32_t>(value);
            else {
                *reinterpret_cast<uint32_t*>(ctx.base + ctx.esp.u32 + 4 + (idx - 2) * 4) =
                    static_cast<uint32_t>(value);
            }
        }
    }

    // Set return values
    template<typename T>
    static void SetReturnValue(X86Context& ctx, T value) {
        if constexpr (is_precise_v<T>) {
            ctx.vec[0].f32[0] = value;
        }
        else if constexpr (std::is_pointer_v<T>) {
            ctx.eax.u32 = value ? g_memory.MapVirtual(value) : 0;
        }
        else {
            ctx.eax.u32 = static_cast<uint32_t>(value);
        }
    }
};

struct Argument {
    int type{};  // 0 = integer, 1 = float
    int ordinal{};
};

template<typename T1>
constexpr std::array<Argument, std::tuple_size_v<T1>> GatherFunctionArguments(const T1& tpl) {
    std::array<Argument, std::tuple_size_v<T1>> args{};
    int floatOrdinal = 0;
    size_t i = 0;

    if constexpr (!args.empty()) {
        std::apply([&](const auto&... elements) {
            ([&](const auto& element) {
                using ElementType = std::decay_t<decltype(element)>;
                if constexpr (is_precise_v<ElementType>) {
                    args[i] = { 1, floatOrdinal++ };
                }
                else {
                    args[i] = { 0, static_cast<int>(i) };
                }
                i++;
                }(elements), ...);
            }, tpl);
    }

    return args;
}

template<auto Func>
constexpr std::array<Argument, arg_count_t<Func>::value> GatherFunctionArguments() {
    return GatherFunctionArguments(function_args(Func));
}

template<auto Func, size_t I>
struct arg_ordinal_t {
    static constexpr size_t value = GatherFunctionArguments<Func>()[I].ordinal;
};

template<auto Func, int I = 0, typename... TArgs>
void _translate_args_to_host(X86Context& ctx, uint8_t* base, std::tuple<TArgs...>&) noexcept
    requires (I >= sizeof...(TArgs)) {}

template<auto Func, int I = 0, typename... TArgs>
std::enable_if_t<(I < sizeof...(TArgs)), void>
    _translate_args_to_host(X86Context& ctx, uint8_t* base, std::tuple<TArgs...>& tpl) noexcept {
    using T = std::tuple_element_t<I, std::remove_reference_t<decltype(tpl)>>;
    std::get<I>(tpl) = ArgTranslator::GetValue<T>(ctx, base, arg_ordinal_t<Func, I>::value);
    _translate_args_to_host<Func, I + 1>(ctx, base, tpl);
}

template<int I = 0, typename... TArgs>
void _translate_args_to_guest(X86Context& ctx, uint8_t* base, std::tuple<TArgs...>&) noexcept
    requires (I >= sizeof...(TArgs)) {}

template<int I = 0, typename... TArgs>
std::enable_if_t<(I < sizeof...(TArgs)), void>
    _translate_args_to_guest(X86Context& ctx, uint8_t* base, std::tuple<TArgs...>& tpl) noexcept {
    using T = std::tuple_element_t<I, std::remove_reference_t<decltype(tpl)>>;
    ArgTranslator::SetValue(ctx, I, std::get<I>(tpl));
    _translate_args_to_guest<I + 1>(ctx, base, tpl);
}

// Hook implementation templates
template<auto Func>
void HostToGuestFunction(X86Context& ctx, uint8_t* base) 
{
    using ret_t = decltype(std::apply(Func, function_args(Func)));
    auto args = function_args(Func);
    _translate_args_to_host<Func>(ctx, base, args);

    if constexpr (std::is_same_v<ret_t, void>) {
        std::apply(Func, args);
    }
    else {
        auto v = std::apply(Func, args);

        if constexpr (std::is_pointer_v<ret_t>) {
            ArgTranslator::SetReturnValue(ctx, v);
        }
        else if constexpr (is_precise_v<ret_t>) {
            ArgTranslator::SetReturnValue(ctx, v);
        }
        else {
            ArgTranslator::SetReturnValue(ctx, v);
        }
    }
}

template<typename T, typename TFunction, typename... TArgs>
T GuestToHostFunction(const TFunction& func, TArgs&&... argv) {
    auto args = std::make_tuple(std::forward<TArgs>(argv)...);
    auto& currentCtx = *GetX86Context();

    X86Context newCtx;
    newCtx = currentCtx; // Copy all registers
    newCtx.base = g_memory.base;

    _translate_args_to_guest(newCtx, g_memory.base, args);
    SetX86Context(newCtx);

    if constexpr (std::is_function_v<std::remove_pointer_t<TFunction>>) {
        func(newCtx, g_memory.base);
    }
    else {
        g_memory.FindFunction(func)(newCtx, g_memory.base);
    }

    currentCtx = newCtx; // Restore context
    SetX86Context(currentCtx);

    if constexpr (!std::is_void_v<T>) {
        return ArgTranslator::GetValue<T>(newCtx, g_memory.base, 0);
    }
}

#define GUEST_FUNCTION_HOOK(subroutine, function) \
    X86_FUNC(subroutine) { HostToGuestFunction<function>(ctx, base); }

#define GUEST_FUNCTION_STUB(subroutine) \
    X86_FUNC(subroutine) { }