#include "stdafx.h"
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <kernel/function.h>
#include <cpu/guest_thread.h>
#include <file.h>
#include <image.h>


static const std::string GameExecutableFile = "default.xbe";

#include <xbe.h>
#ifdef _WIN32
#include <timeapi.h>
#endif
#include <iostream>

#if defined(_WIN32) && defined(SHADOW_RECOMP_D3D12)
static std::array<std::string_view, 3> g_D3D12RequiredModules =
{
    "D3D12/D3D12Core.dll",
    "dxcompiler.dll",
    "dxil.dll"
};
#endif

Memory g_memory;
Heap g_userHeap;

// return a modifiable pointer inside this structure that corresponds to a virtual address
uint8_t* GetAddr(uint32_t xbeVA, const XbeHeader& header, const XbeSectionHeader* sectionHeaders)
{
    for (uint32_t i = 0; i < header.numSections; i++)
    {
        uint32_t virtStart = sectionHeaders[i].virtualAddress;
        uint32_t virtSize = sectionHeaders[i].virtualSize;

        if (xbeVA >= virtStart && xbeVA < (virtStart + virtSize))
        {
            uint32_t offsetInSection = xbeVA - virtStart;
            uint8_t* sectionBaseHost = reinterpret_cast<uint8_t*>(g_memory.Translate(virtStart));
            return sectionBaseHost + offsetInSection;
        }
    }

    return nullptr; // Not found
}

TLS* m_TLS;

uint32_t LoadXbeModule(const std::filesystem::path& path)
{
    auto module = LoadFile(path);

    if (module.empty())
    {
        assert("Failed to load module" && false);
        return 0;
    }


    const auto* header = reinterpret_cast<const XbeHeader*>(module.data());
    const uint32_t sectionHeadersOffset = header->sectionHeadersAddress - header->baseAddress;
    const auto* sectionHeaders = reinterpret_cast<const XbeSectionHeader*>(module.data() + sectionHeadersOffset);

    uint32_t firstSectionVA = sectionHeaders[0].virtualAddress; 
    uint32_t headersSize = firstSectionVA - header->baseAddress;        

    //Copy headers to their expected VA 
    void* headersDest = g_memory.Translate(header->baseAddress); 
    memcpy(headersDest, module.data(), headersSize);

    for (uint32_t i = 0; i < header->numSections; i++)
    {
        auto srcData = module.data() + sectionHeaders[i].rawAddress;
        auto destData = g_memory.Translate(sectionHeaders[i].virtualAddress);
        memcpy(destData, srcData, sectionHeaders[i].rawSize);
    }

    void* addr = GetAddr(header->tlsAddress, *header, sectionHeaders);
    if (addr)
    {
        m_TLS = new TLS;
        memcpy(m_TLS, addr, sizeof(*m_TLS));
    }

    return decode_entry_point(header->m_entry, header->sizeOfImage);
}

void HostStartup()
{
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    g_userHeap.Init();
}


void FakeCallBack()
{
    X86Context x86Context{0};

    g_memory.FindFunction(0x2AFB53)(x86Context, g_memory.base);

    assert(GetX86Context() == nullptr);
    SetX86Context(x86Context);
}

int main(int argc, char* argv[])
{
    //std::set_terminate(exception_handler);
    HostStartup();
	//InitializeFunctionOffsets();
    std::filesystem::path xbePath;
    xbePath = GameExecutableFile;
    uint32_t entry = LoadXbeModule(xbePath);
	printf("XBE loaded, starting GuestThread::Start with entry function\n");
    //GuestThread::Start({ entry, 0, 0 });
    GuestThreadParams params = { entry, 0, 0, 0, 0 };
    GuestThread::Start(params);
	// Cleanup
    printf("function Main is returning 0\n");
	return 0;
}