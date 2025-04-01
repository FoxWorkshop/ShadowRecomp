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
    auto srcData = module.data();

    const auto* header = reinterpret_cast<const XbeHeader*>(srcData);


    auto destData = reinterpret_cast<uint8_t*>(g_memory.Translate(header->baseAddress));

    return decode_entry_point(header->m_entry, module.size());

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