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
    return decode_entry_point(header->m_entry, header->sizeOfImage);
}

void HostStartup()
{
#ifdef _WIN32
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
#endif

    g_userHeap.Init();
}

int main(int argc, char* argv[])
{
    HostStartup();

    std::filesystem::path xbePath;
    xbePath = GameExecutableFile;
    uint32_t entry = LoadXbeModule(xbePath);
    GuestThread::Start({ entry, 0, 0 });
	return 0;
}