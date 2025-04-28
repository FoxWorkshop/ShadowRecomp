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
//hacky way to repair sprintf which strip the last 4 characters for some reason.
//note that I'd simply hook sprintf if it was possible, but since the numbers of parameters are undertermined
//until runtime it's complicated, so I take a dirty shortcut.

X86_FUNC_IMPL(__imp__sub_382D53);
X86_FUNC(sub_382D53)
{
	uint32_t ebp = X86_LOAD_U32(ctx.ebp.u32 + 12);
	std::string backup = (char*)(base + ebp); //first we backup the string that got passed

	__imp__sub_382D53(ctx, base); //call og function

	ctx.esp.u32 -= 4; //the og function adjusted esp, we need to take that in account 
	ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (7 * 4)); //get the edited buffer from og function
	std::string newBuffer = (char*)(base + ebp);
	if (ctx.eax.u32 > backup.size() + 5 || newBuffer.size() < backup.size()) //if there is a huge size difference, assume it cuts a few characters
	{
		//add missing string part that got cut
		int offset = backup.size() - newBuffer.size();
		newBuffer += backup.substr(backup.length() - offset);
		char* writable = (char*)g_userHeap.Alloc(newBuffer.size() + 1); // +1 for null terminator
		std::copy(newBuffer.begin(), newBuffer.end(), writable);
		writable[newBuffer.size()] = '\0'; // Null-terminate manually

		uint32_t targetAddr = ctx.esp.u32 + 4 + (34 * 4);
		for (size_t i = 0; i < newBuffer.size() + 1; ++i)
		{
			*(volatile uint8_t*)(base + targetAddr + i) = writable[i]; //write new string to esp so function can access it
		}

		auto map = g_memory.MapVirtual(writable);
		ctx.esi.u32 = (uint32_t)(base + map);
		ctx.eax.u32 = newBuffer.size(); //sprintf returns the number of characters, so we fix it here just in case
	}

	ctx.esp.u32 += 4; //bring back esp to what it was when og function was called
}
