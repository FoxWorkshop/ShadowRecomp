#include "file_system.h"
#include <cpu/guest_thread.h>
#include <kernel/kernel.h>
#include <kernel/function.h>
#include <stdafx.h>
#include <iostream>

struct FileHandle : KernelObject
{
    std::fstream stream;
    std::filesystem::path path;
};

std::filesystem::path FileSystem::ResolvePath(const std::string_view& path, bool checkForMods)
{

    thread_local std::string builtPath;
    builtPath.clear();
    builtPath = MainGameFolder + "/";

    size_t index = path.find(":\\");
    if (index != std::string::npos)
    {
        std::string_view root = path.substr(0, index);
        builtPath += path.substr(index + 2);
    }
    else
    {
        builtPath += path;
    }

    std::replace(builtPath.begin(), builtPath.end(), '\\', '/');

    return std::u8string_view((const char8_t*)builtPath.c_str());
}

FileHandle* CreateFile_r(const char* SourceString, ULONG CreateDisposition, ULONG ShareAccess, int a4, int mode, unsigned int a6, int a7)
{
    ULONG CreateDisposition_ = FILE_OPEN;
    switch (mode)
    {
    case 1: 
        CreateDisposition_ = FILE_CREATE;
        break;
    case 2:
        CreateDisposition_ = FILE_OVERWRITE_IF;
        break;
    case 3:
        CreateDisposition_ = FILE_OPEN;
        break;
    case 4:
        CreateDisposition_ = FILE_OPEN_IF;
        break;
    default:
        if (mode != 5 || (CreateDisposition_ = 4, (CreateDisposition & 0x40000000) == 0))
        {
            return GetInvalidKernelObject<FileHandle>();
        }
        break;
    }

    std::filesystem::path filePath = FileSystem::ResolvePath(SourceString, true);
    std::fstream fileStream;
    std::ios::openmode fileOpenMode = std::ios::binary;
    if (CreateDisposition_ == FILE_OPEN || CreateDisposition_ == FILE_OPEN_IF)
    {
        std::cout << "Reading: " << filePath << std::endl;
        fileOpenMode |= std::ios::in;
    }
    else
    {
        fileOpenMode |= std::ios::out;
    }

    fileStream.open(filePath, fileOpenMode);
    if (!fileStream.is_open())
    {
        return GetInvalidKernelObject<FileHandle>();
    }

    FileHandle* fileHandle = CreateKernelObject<FileHandle>();
    fileHandle->stream = std::move(fileStream);
    fileHandle->path = std::move(filePath);
    return fileHandle;
}

uint32_t ReadFile_r(FileHandle* Handle, void* buffer, int length, uint32_t bytesRead, int overLapped)
{

    uint32_t result = FALSE;
    uint32_t numberOfBytesRead;
    Handle->stream.read((char*)(buffer), bytesRead);
    if (!Handle->stream.bad())
    {
        numberOfBytesRead = uint32_t(Handle->stream.gcount());
        result = TRUE;
    }

    return result;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_2AF653, CreateFile_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_2ADFF0, ReadFile_r);