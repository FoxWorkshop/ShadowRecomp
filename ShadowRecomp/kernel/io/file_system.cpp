#include "file_system.h"
#include <cpu/guest_thread.h>
#include <kernel/kernel.h>
#include <kernel/function.h>
#include <stdafx.h>
#include <iostream>

//Original code from Unleashed Recomp, O.G Xbox uses similar file / write operation functions than Xbox 360.

struct FileHandle : KernelObject
{
	std::fstream stream;
	std::filesystem::path path;
};

struct FindHandle : KernelObject
{
	std::error_code ec;
	ankerl::unordered_dense::map<std::u8string, std::pair<size_t, bool>> searchResult; // Relative path, file size, is directory
	decltype(searchResult)::iterator iterator;

	FindHandle(const std::string_view& path)
	{
		auto addDirectory = [&](const std::filesystem::path& directory)
			{
				for (auto& entry : std::filesystem::directory_iterator(directory, ec))
				{
					std::u8string relativePath = entry.path().lexically_relative(directory).u8string();
					searchResult.emplace(relativePath, std::make_pair(entry.is_directory(ec) ? 0 : entry.file_size(ec), entry.is_directory(ec)));
				}
			};

		std::string_view pathNoPrefix = path;
		size_t index = pathNoPrefix.find(":\\");
		if (index != std::string_view::npos)
			pathNoPrefix.remove_prefix(index + 2);


		addDirectory(FileSystem::ResolvePath(path, false));

		iterator = searchResult.begin();
	}

	void fillFindData(WIN32_FIND_DATAA* lpFindFileData)
	{
		if (iterator->second.second)
			lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
		else
			lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

		strncpy(lpFindFileData->cFileName, (const char*)(iterator->first.c_str()), sizeof(lpFindFileData->cFileName));
		lpFindFileData->nFileSizeLow = (uint32_t(iterator->second.first >> 32U));
		lpFindFileData->nFileSizeHigh = (uint32_t(iterator->second.first));
		lpFindFileData->ftCreationTime = {};
		lpFindFileData->ftLastAccessTime = {};
		lpFindFileData->ftLastWriteTime = {};
	}
};

std::filesystem::path FileSystem::ResolvePath(const std::string_view& path, bool checkForMods)
{

	thread_local std::string builtPath;
	builtPath.clear();
	builtPath = MainGameFolder + "/";

	size_t index = path.find(":\\");
	if (index != std::string::npos)
	{
		std::string_view relativePart = path.substr(index + 2);
		// Remove leading "dvd\" if present (case-insensitive)
		if (relativePart.size() >= 4 &&
			std::tolower(relativePart[0]) == 'd' &&
			std::tolower(relativePart[1]) == 'v' &&
			std::tolower(relativePart[2]) == 'd' &&
			(relativePart[3] == '\\' || relativePart[3] == '/'))
		{
			relativePart.remove_prefix(4); // skip "dvd\"
		}

		builtPath += relativePart;
	}
	else
	{
		builtPath += path;
	}

	std::replace(builtPath.begin(), builtPath.end(), '\\', '/');

	return std::u8string_view((const char8_t*)builtPath.c_str());
}

FileHandle* XCreateFile_r(const char* SourceString, ULONG CreateDisposition, ULONG ShareAccess, int a4, int mode, unsigned int a6, int a7)
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
		logs.LogFile("Loading", filePath.c_str());
		fileOpenMode |= std::ios::in;
	}
	else
	{
		fileOpenMode |= std::ios::out;
	}

	if (filePath.string().find("number.txd") != std::string::npos)
	{
		printf("ok\n");
	}

	std::error_code ec;
	fileStream.open(filePath, fileOpenMode);
	if (!std::filesystem::exists(filePath))
		std::cout << "File does not exist: " << filePath << std::endl;
	if (!fileStream.is_open())
	{
		std::cout << "Failed to open: " << filePath << " | errno: " << errno << " | message: " << strerror(errno) << std::endl;
		return GetInvalidKernelObject<FileHandle>();
	}

	FileHandle* fileHandle = CreateKernelObject<FileHandle>();
	fileHandle->stream = std::move(fileStream);
	fileHandle->path = std::move(filePath);
	return fileHandle;
}

uint32_t XReadFile_r(FileHandle* Handle, void* buffer, int length, uint32_t* bytesRead, XOVERLAPPED* overLapped)
{
	uint32_t result = FALSE;

	if (overLapped != nullptr)
	{
		std::streamoff streamOffset = overLapped->Offset + (std::streamoff(overLapped->OffsetHigh) << 32U);
		Handle->stream.clear();
		Handle->stream.seekg(streamOffset, std::ios::beg);
		if (Handle->stream.bad())
		{
			return FALSE;
		}
	}

	uint32_t numberOfBytesRead;
	Handle->stream.read((char*)(buffer), length);
	if (!Handle->stream.bad())
	{
		numberOfBytesRead = uint32_t(Handle->stream.gcount());
		result = TRUE;
	}

	if (result)
	{
		if (overLapped != nullptr)
		{
			overLapped->Internal = 0;
			overLapped->InternalHigh = numberOfBytesRead;
		}
		else if (bytesRead != nullptr)
		{
			*bytesRead = numberOfBytesRead;
		}
	}

	return result;
}

static uint32_t XGetFileSize_r(FileHandle* hFile, uint32_t* lpFileSizeHigh)
{
	std::error_code ec;
	auto fileSize = std::filesystem::file_size(hFile->path, ec);
	if (!ec)
	{
		if (lpFileSizeHigh != nullptr)
		{
			*lpFileSizeHigh = uint32_t(fileSize >> 32U);
		}

		return (uint32_t)(fileSize);
	}

	return INVALID_FILE_SIZE;
}

uint32_t XGetFileSizeEx_r(FileHandle* hFile, LARGE_INTEGER* lpFileSize)
{
	std::error_code ec;
	auto fileSize = std::filesystem::file_size(hFile->path, ec);
	if (!ec)
	{
		if (lpFileSize != nullptr)
		{
			lpFileSize->QuadPart = fileSize;
		}

		return TRUE;
	}

	return FALSE;
}

uint32_t XSetFilePointer_r(FileHandle* hFile, int32_t lDistanceToMove, int32_t* lpDistanceToMoveHigh, uint32_t dwMoveMethod)
{
	int32_t distanceToMoveHigh = (lpDistanceToMoveHigh != nullptr) ? *lpDistanceToMoveHigh : 0;
	std::streamoff streamOffset = lDistanceToMove + (std::streamoff(distanceToMoveHigh) << 32U);
	std::fstream::seekdir streamSeekDir = {};
	switch (dwMoveMethod)
	{
	case FILE_BEGIN:
		streamSeekDir = std::ios::beg;
		break;
	case FILE_CURRENT:
		streamSeekDir = std::ios::cur;
		break;
	case FILE_END:
		streamSeekDir = std::ios::end;
		break;
	default:
		assert(false && "Unknown move method.");
		break;
	}

	hFile->stream.clear();
	hFile->stream.seekg(streamOffset, streamSeekDir);
	if (hFile->stream.bad())
	{
		return INVALID_SET_FILE_POINTER;
	}

	std::streampos streamPos = hFile->stream.tellg();
	if (lpDistanceToMoveHigh != nullptr)
		*lpDistanceToMoveHigh = int32_t(streamPos >> 32U);

	return uint32_t(streamPos);
}

uint32_t XSetFilePointerEx_r(FileHandle* hFile, int32_t lDistanceToMove, LARGE_INTEGER* lpNewFilePointer, uint32_t dwMoveMethod)
{
	std::fstream::seekdir streamSeekDir = {};
	switch (dwMoveMethod)
	{
	case FILE_BEGIN:
		streamSeekDir = std::ios::beg;
		break;
	case FILE_CURRENT:
		streamSeekDir = std::ios::cur;
		break;
	case FILE_END:
		streamSeekDir = std::ios::end;
		break;
	default:
		assert(false && "Unknown move method.");
		break;
	}

	hFile->stream.clear();
	hFile->stream.seekg(lDistanceToMove, streamSeekDir);
	if (hFile->stream.bad())
	{
		return FALSE;
	}

	if (lpNewFilePointer != nullptr)
	{
		lpNewFilePointer->QuadPart = hFile->stream.tellg();
	}

	return TRUE;
}

uint32_t XWriteFile_r(FileHandle* hFile, const void* lpBuffer, uint32_t nNumberOfBytesToWrite, uint32_t* lpNumberOfBytesWritten, void* lpOverlapped)
{
	assert(lpOverlapped == nullptr && "Overlapped not implemented.");

	hFile->stream.write((const char*)(lpBuffer), nNumberOfBytesToWrite);
	if (hFile->stream.bad())
		return FALSE;

	if (lpNumberOfBytesWritten != nullptr)
		*lpNumberOfBytesWritten = uint32_t(hFile->stream.gcount());

	return TRUE;
}

FindHandle* XFindFirstFile(const char* lpFileName, WIN32_FIND_DATAA* lpFindFileData)
{
	std::string_view path = lpFileName;
	if (path.find("\\*") == (path.size() - 2) || path.find("/*") == (path.size() - 2))
	{
		path.remove_suffix(1);
	}
	else if (path.find("\\*.*") == (path.size() - 4) || path.find("/*.*") == (path.size() - 4))
	{
		path.remove_suffix(3);
	}
	else
	{
		assert(!std::filesystem::path(path).has_extension() && "Unknown search pattern.");
	}

	FindHandle findHandle(path);

	if (findHandle.searchResult.empty())
		return GetInvalidKernelObject<FindHandle>();

	findHandle.fillFindData(lpFindFileData);

	return CreateKernelObject<FindHandle>(std::move(findHandle));
}

uint32_t XFindNextFile(FindHandle* Handle, WIN32_FIND_DATAA* lpFindFileData)
{
	Handle->iterator++;

	if (Handle->iterator == Handle->searchResult.end())
	{
		return FALSE;
	}
	else
	{
		Handle->fillFindData(lpFindFileData);
		return TRUE;
	}
}

int CacheGameFiles(int a1)
{

	LOGN("CACHE STUB");
	return 0;

}


//GUEST_FUNCTION_HOOK_THISCALL(sub_14D10, IsPtrEqual);

//GUEST_FUNCTION_HOOK(sub_382D53, sub382D53_r);
//GUEST_FUNCTION_HOOK(sub_37E577, sub_37E577_r);

GUEST_FUNCTION_HOOK_STDCALL(sub_2AF653, XCreateFile_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_2ADFF0, XReadFile_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AE0DD, XWriteFile_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AE45E, XGetFileSize_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AE420, XGetFileSizeEx_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AE237, XSetFilePointer_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AE331, XSetFilePointerEx_r);
GUEST_FUNCTION_HOOK_STDCALL(sub_2AD93C, XFindFirstFile);
GUEST_FUNCTION_HOOK_STDCALL(sub_2ADA49, XFindNextFile);

GUEST_FUNCTION_HOOK(sub_332B60, CacheGameFiles);


//hacky way to repair sprintf which strip the last 4 characters for some reason.
//note that I'd simply hook sprintf if it was possible, but since the numbers of parameters are undertermined
//until runtime it's complicated, so I take a dirty shortcut.

//sprintf
bool trackingThisCrap = false;
X86_FUNC_IMPL(__imp__sub_37E577);
X86_FUNC(sub_37E577)
{

	if (trackingThisCrap)
	{
		uint32_t ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (0 * 4));
		const char* arg0 = (char*)(base + ebp);
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (1 * 4));
		auto arg1 = (char*)(base + ebp);
		arg1 = arg1;
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (2 * 4));
		auto arg2 = (char*)(base + ebp);
		arg2 = arg2;
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (3 * 4));
		auto arg3 = (char*)(base + ebp);
		arg3 = arg3;

	}

	__imp__sub_37E577(ctx, base); //call og function

	if (trackingThisCrap)
	{
		ctx.esp.u32 -= 4;
		uint32_t ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (0 * 4));
		const char* arg0 = (char*)(base + ebp);
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (1 * 4));
		auto arg1 = (char*)(base + ebp);
		arg1 = arg1;
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (2 * 4));
		auto arg2 = (char*)(base + ebp);
		arg2 = arg2;
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (3 * 4));
		auto arg3 = (char*)(base + ebp);
		arg3 = arg3;
		ctx.esp.u32 += 4;

	}


}

//sscanF
X86_FUNC_IMPL(__imp__sub_37F0C6);
X86_FUNC(sub_37F0C6)
{

	if (trackingThisCrap)
	{

		uint32_t ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (0 * 4));
		const char* arg0 = (char*)(base + ebp);
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (1 * 4));
		auto arg1 = (char*)(base + ebp);
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (2 * 4));
		auto arg2 = (char*)(base + ebp);
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (3 * 4));
		auto arg3 = (char*)(base + ebp);
		arg3 = arg3;
		//auto test = sscanf(arg0, arg1, arg2, arg3);
		//test = test;
	}
	__imp__sub_37F0C6(ctx, base);

	if (trackingThisCrap)
	{
		ctx.esp.u32 -= 4;
		uint32_t ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (0 * 4));
		const char* arg0 = (char*)(base + ebp);
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (1 * 4));
		auto arg1 = (char*)(base + ebp);
		arg1 = arg1;
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (2 * 4));
		auto arg2 = (char*)(base + ebp);
		ebp = X86_LOAD_U32(ctx.esp.u32 + 4 + (3 * 4));
		auto arg3 = (char*)(base + ebp);
		arg3 = arg3;
		ctx.esp.u32 += 4;
	}
}



int __cdecl sub_382D53_r(char* buffer, char* format, uint32_t va)
{
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	if (!trackingThisCrap)
	{
		//__imp__sub_382D53(*ctx, base);
		ctx->esp.u32 -= 4; //esp got adjusted already but the hook will redo it, we need to fix it 
		return ctx->eax.u32;
	}
	else
	{
		auto res = vsprintf(buffer, format, (va_list)va);
		return res;
	}

}