#include "stdafx.h"
#include <kernel/memory.h>
#include <kernel/heap.h>
#include <kernel/function.h>
#include <gpu/video.h>
#include <cpu/guest_thread.h>
#include <input/input.h>
#include <file.h>
#include <image.h>
#include <user/paths.h>
#include <kernel/io/file_system.h>
#include <audio/audio.h>
#include <ui/game_window.h>
#include <os/logger.h>
#include <os/process.h>
#include <os/registry.h>

#include <xbe.h>
#ifdef _WIN32
#include <timeapi.h>
#endif
#include <iostream>
#include <filesystem>
#include <mod/mod_loader.h>
#include <preload_executable.h>
#include <patches/unix_patches.h>

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
        assert("Failed to load XBE file, please make sure game files are present." && false);
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

    input::Init();

    if (g_memory.base == nullptr)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), Localise("System_MemoryAllocationFailed").c_str(), GameWindow::s_pWindow);
        std::_Exit(1);
    }


    g_userHeap.Init();
}

std::atomic<bool> running{ true };
extern bool isHostThread;

void __stdcall system_events_launch()
{
    GuestThreadParams events = { 0, 0, 0, 0, 0 };
    GuestThread::Start_Non_Periodic_Events(events);
}

void __stdcall dpc_launch()
{
    GuestThreadParams dpc = { 0, 0, 0, 0, 0 };
    GuestThread::Start_Dpc_Routine(dpc);
}

std::vector<float> dataFloat;

std::vector<float>* getDataFloat()
{
    return &dataFloat;
}

std::vector<float> ReadFloatPairs(const std::string& path)
{
 
    std::ifstream in(path);
    if (!in.is_open()) 
        return dataFloat;

    std::string line;
    while (std::getline(in, line))
    {
        std::istringstream iss(line);
        float a;
        if (iss >> a)
            dataFloat.push_back(a);
    }
    return dataFloat;
}





int main(int argc, char* argv[])
{
#ifdef _WIN32
    timeBeginPeriod(1);
#endif

    os::process::CheckConsole();

    if (!os::registry::Init())
        LOGN_WARNING("OS does not support registry.");


    os::logger::Init();

    PreloadContext preloadContext;
    preloadContext.PreloadExecutable();

    LOGN("Starting Shadow Recomp...");

    std::filesystem::path xbePath;
    xbePath = GameExecutable;

    bool forceInstaller = false;
    bool forceDLCInstaller = false;
    bool useDefaultWorkingDirectory = false;
    bool forceInstallationCheck = false;
    bool graphicsApiRetry = false;
    const char* sdlVideoDriver = nullptr;

    for (uint32_t i = 1; i < argc; i++)
    {
        forceInstaller = forceInstaller || (strcmp(argv[i], "--install") == 0);
        forceDLCInstaller = forceDLCInstaller || (strcmp(argv[i], "--install-dlc") == 0);
        useDefaultWorkingDirectory = useDefaultWorkingDirectory || (strcmp(argv[i], "--use-cwd") == 0);
        forceInstallationCheck = forceInstallationCheck || (strcmp(argv[i], "--install-check") == 0);
        graphicsApiRetry = graphicsApiRetry || (strcmp(argv[i], "--graphics-api-retry") == 0);

        if (strcmp(argv[i], "--sdl-video-driver") == 0)
        {
            if ((i + 1) < argc)
                sdlVideoDriver = argv[++i];
            else
                LOGN_WARNING("No argument was specified for --sdl-video-driver. Option will be ignored.");
        }
    }

    if (!useDefaultWorkingDirectory)
    {
        // Set the current working directory to the executable's path.
        std::error_code ec;
        std::filesystem::current_path(os::process::GetExecutableRoot(), ec);
    }

    Config::Load();

#if defined(_WIN32) && defined(SHADOW_RECOMP_D3D12)
    for (auto& dll : g_D3D12RequiredModules)
    {
        if (!std::filesystem::exists(g_executableRoot / dll))
        {
            char text[512];
            snprintf(text, sizeof(text), "missing graphics DLL", dll.data());
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), text, GameWindow::s_pWindow);
            std::_Exit(1);
        }
    }
#endif

    if (Config::ShowConsole)
        os::process::ShowConsole();

    HostStartup();

    ModLoader::Init();

    InitDpcData();


    NV2ADevice_Init();

    //Create "interrupt events" thread, a loop will check and fire guest function stored from KeConnectInterrupt
   // std::thread eventsThread(system_events_launch);

    //Create DPC Routine thread, a loop will check and fire guest function stored from KeInsertQueueDpc
    //std::thread dpcThread(dpc_launch);

    //init mutex for Loading thread patch
    InitPatchesLoadingCriticalSection();
    uint32_t entry = LoadXbeModule(xbePath);


    if (!Video::CreateHostDevice(sdlVideoDriver, graphicsApiRetry))
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, GameWindow::GetTitle(), "Backend Graphics error", GameWindow::s_pWindow);
        std::_Exit(1);
    }



    PatchUnixStrings();

    //launch the Start function of the game
    isHostThread = true;
    GuestThreadParams params = { entry, 0, 0, 0, 0 };
    GuestThread::Start(params);

    
    running = false;
    printf("function Main is returning 0\n");
	return 0;
}