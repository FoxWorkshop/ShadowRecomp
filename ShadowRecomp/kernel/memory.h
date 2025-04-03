#pragma once

#ifndef _WIN32
#define MEM_COMMIT  0x00001000  
#define MEM_RESERVE 0x00002000  
#endif

struct Memory
{
    uint8_t* base{};

    Memory();

    bool IsInMemoryRange(const void* host) const noexcept
    {
        return host >= base && host < (base + X86_MEMORY_SIZE);
    }

    void* Translate(size_t offset) const noexcept
    {
        if (offset)
            assert(offset < 0x100000000ull);

        return base + offset;
    }

    uint32_t MapVirtual(const void* host) const noexcept
    {
        if (host)
            assert(IsInMemoryRange(host));

        return static_cast<uint32_t>(static_cast<const uint8_t*>(host) - base);
    }

    X86Func* FindFunction(uint32_t guest) const noexcept
    {
        return X86_LOOKUP_FUNC(base, guest);
    }

    void InsertFunction(uint32_t guest, X86Func* host)
    {
        X86_LOOKUP_FUNC(base, guest) = host;
    }
};

extern "C" void* MmGetHostAddress(uint32_t ptr);
extern Memory g_memory;
