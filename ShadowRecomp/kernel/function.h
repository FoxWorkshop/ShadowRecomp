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

template<typename T>
struct function_traits;

template<typename R, typename... Args>
struct function_traits<R(*)(Args...)> {
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arg_count = sizeof...(Args);
};

// Helper to get function arguments as tuple
template<auto Func>
constexpr auto get_function_args() {
    return typename function_traits<decltype(Func)>::args_tuple();
}

// Add an enum for calling conventions
enum class CallingConvention {
    Cdecl,
    Stdcall,
    Fastcall,
    Thiscall
};

// Templated ArgTranslator with specializations
template <CallingConvention Conv>
struct ArgTranslator;

// Cdecl specialization: all arguments on the stack
template <>
struct ArgTranslator<CallingConvention::Cdecl> {
    template<typename T>
    static T GetValue(X86Context& ctx, uint8_t* base, size_t idx) {
        if constexpr (std::is_floating_point_v<T>) {
            // Floats passed via XMM registers in this ABI
            if (idx < 8) return ctx.vec[idx].f32[0];
            uint32_t stack_offset = 4 + (idx - 8) * 4;
            return *reinterpret_cast<T*>(base + ctx.esp.u32 + stack_offset);
        }
        else {
            uint32_t stack_offset = 4 + idx * 4;
            uint32_t val = *reinterpret_cast<uint32_t*>(base + ctx.esp.u32 + stack_offset);
            if constexpr (std::is_pointer_v<T>) return val ? reinterpret_cast<T>(base + val) : nullptr;
            else return static_cast<T>(val);
        }
    }

    template<typename T>
    static void SetReturnValue(X86Context& ctx, T value) 
    {
        if constexpr (std::is_floating_point_v<T>) 
            ctx.vec[0].f32[0] = value;
        else if constexpr (std::is_pointer_v<T>) 
            ctx.eax.u32 = value ? g_memory.MapVirtual(value) : 0;
        else 
            ctx.eax.u32 = static_cast<uint32_t>(value);
    }
};

// Stdcall specialization: args on stack, callee cleanup
template <>
struct ArgTranslator<CallingConvention::Stdcall> {
    template<typename T>
    static T GetValue(X86Context& ctx, uint8_t* base, size_t idx) {
        uint32_t stack_offset = 4 + idx * 4;
        if constexpr (std::is_floating_point_v<T>) {
            return *reinterpret_cast<T*>(base + ctx.esp.u32 + stack_offset);
        }
        else {
            uint32_t val = *reinterpret_cast<uint32_t*>(base + ctx.esp.u32 + stack_offset);
            if constexpr (std::is_pointer_v<T>) return val ? reinterpret_cast<T>(base + val) : nullptr;
            else return static_cast<T>(val);
        }
    }

    template<typename T>
    static void SetReturnValue(X86Context& ctx, T value)
    {
        if constexpr (std::is_floating_point_v<T>)
            ctx.vec[0].f32[0] = value;
        else if constexpr (std::is_pointer_v<T>)
            ctx.eax.u32 = value ? g_memory.MapVirtual(value) : 0;
        else
            ctx.eax.u32 = static_cast<uint32_t>(value);
    }
};

// Fastcall specialization: first two in ECX/EDX
template <>
struct ArgTranslator<CallingConvention::Fastcall> {
    template<typename T>
    static T GetValue(X86Context& ctx, uint8_t* base, size_t idx) {
        if constexpr (std::is_floating_point_v<T>) {
            if (idx < 8) return ctx.vec[idx].f32[0];
            uint32_t stack_offset = 4 + (idx - 8) * 4;
            return *reinterpret_cast<T*>(base + ctx.esp.u32 + stack_offset);
        }
        else if (idx < 2) {
            uint32_t val = (idx == 0) ? ctx.ecx.u32 : ctx.edx.u32;
            if constexpr (std::is_pointer_v<T>) return val ? reinterpret_cast<T>(base + val) : nullptr;
            else return static_cast<T>(val);
        }
        else {
            uint32_t stack_offset = 4 + (idx - 2) * 4;
            uint32_t val = *reinterpret_cast<uint32_t*>(base + ctx.esp.u32 + stack_offset);
            if constexpr (std::is_pointer_v<T>) return val ? reinterpret_cast<T>(base + val) : nullptr;
            else return static_cast<T>(val);
        }
    }
};

// Thiscall specialization: 'this' in ECX, others on stack
template <>
struct ArgTranslator<CallingConvention::Thiscall> {
    template<typename T>
    static T GetValue(X86Context& ctx, uint8_t* base, size_t idx) {
        if constexpr (std::is_floating_point_v<T>) {
            if (idx < 8) return ctx.vec[idx].f32[0];
            uint32_t stack_offset = 4 + (idx - 8) * 4;
            return *reinterpret_cast<T*>(base + ctx.esp.u32 + stack_offset);
        }
        else if (idx == 0) {
            uint32_t val = ctx.ecx.u32;
            if constexpr (std::is_pointer_v<T>) return val ? reinterpret_cast<T>(base + val) : nullptr;
            else return static_cast<T>(val);
        }
        else {
            uint32_t stack_offset = 4 + (idx - 1) * 4;
            uint32_t val = *reinterpret_cast<uint32_t*>(base + ctx.esp.u32 + stack_offset);
            if constexpr (std::is_pointer_v<T>) return val ? reinterpret_cast<T>(base + val) : nullptr;
            else return static_cast<T>(val);
        }
    }
};

template <CallingConvention Conv, auto Func>
void HostToGuestFunction(X86Context& ctx, uint8_t* base) {
    using Traits = function_traits<decltype(Func)>;
    using ret_t = decltype(std::apply(Func, get_function_args<Func>()));

    // Create and populate arguments tuple
    typename Traits::args_tuple args;

    // Translate each argument
    [&] <size_t... I>(std::index_sequence<I...>) {
        ([&] {
            using ArgType = std::tuple_element_t<I, typename Traits::args_tuple>;
            std::get<I>(args) = ArgTranslator<Conv>::GetValue<ArgType>(ctx, base, I);
            }(), ...);
    }(std::make_index_sequence<Traits::arg_count>{});

    // Call the function and handle return
    if constexpr (std::is_void_v<ret_t>) {
        std::apply(Func, args);
    }
    else {
        ArgTranslator<Conv>::SetReturnValue(ctx, std::apply(Func, args));
    }
}

// New hook macros for each convention
#define GUEST_FUNCTION_HOOK(subroutine, function) \
    X86_FUNC(subroutine) { HostToGuestFunction<CallingConvention::Cdecl, function>(ctx, base); }

#define GUEST_FUNCTION_HOOK_STDCALL(subroutine, function) \
    X86_FUNC(subroutine) { HostToGuestFunction<CallingConvention::Stdcall, function>(ctx, base); }

#define GUEST_FUNCTION_HOOK_FASTCALL(subroutine, function) \
    X86_FUNC(subroutine) { HostToGuestFunction<CallingConvention::Fastcall, function>(ctx, base); }

#define GUEST_FUNCTION_HOOK_THISCALL(subroutine, function) \
    X86_FUNC(subroutine) { HostToGuestFunction<CallingConvention::Thiscall, function>(ctx, base); }

#define GUEST_FUNCTION_STUB(subroutine) \
    X86_FUNC(subroutine) { }