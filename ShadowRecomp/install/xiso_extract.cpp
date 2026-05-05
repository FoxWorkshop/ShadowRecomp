// Credit to extract-xiso devs (in@fishtank.com and other extract-xiso contributors) for a lot of the implementation here
// https://github.com/XboxDev/extract-xiso/

#include "xiso_extract.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <xxhash.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#else
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace XisoExtract
{
    static constexpr uint32_t SECTOR_SIZE = 2048;
    static constexpr uint32_t HEADER_OFFSET = 0x10000;
    static constexpr uint32_t BUFFER_SIZE = 0x200000;
    static constexpr uint32_t DWORD_SIZE = 4;

    static constexpr uint32_t XGD1_LSEEK_OFFSET = 0x18300000u;
    static constexpr uint32_t GLOBAL_LSEEK_OFFSET = 0x0FD90000u;
    static constexpr uint32_t XGD3_LSEEK_OFFSET = 0x02080000u;

    static constexpr const char* HEADER_MAGIC = "MICROSOFT*XBOX*MEDIA";
    static constexpr uint32_t HEADER_MAGIC_LENGTH = 20;

    static constexpr uint8_t ATTRIBUTE_DIR = 0x10;
    static constexpr uint16_t PAD_SHORT = 0xFFFF;

    static constexpr float HASH_PHASE_END = 0.35f;
    static constexpr float EXTRACT_PHASE_RANGE = 1.0f - HASH_PHASE_END;

    // Supports both the Redump and XISO of NTSC-U Shadow the Hedgehog Xbox
    static constexpr XXH128_hash_t VALID_HASHES[] =
    {
        { 0x9c8f38553b205287ull, 0x3a005d5c8fb7a469ull },
        { 0xcab29d6db63896eaull, 0xe837382596ad70d4ull },
    };

    struct Entry
    {
        uint16_t lOffset;
        uint16_t rOffset;
        uint32_t startSector;
        uint32_t fileSize;
        uint8_t attributes;
        uint8_t filenameLength;
        char filename[256];
    };

    static uint64_t s_discOffset = 0;
    static uint64_t s_extractedBytes = 0;
    static uint64_t s_totalBytes = 0;
    static uint64_t s_lastExtractionReport = 0;
    static Progress s_progress = {};
    static ProgressCallback s_callback = nullptr;

    static int64_t XisoLseek(int fd, int64_t offset, int whence)
    {
#ifdef _WIN32
        return _lseeki64(fd, offset, whence);
#else
        return lseek(fd, offset, whence);
#endif
    }

    static bool XisoRead(int fd, void* buf, size_t count)
    {
        size_t totalRead = 0;
        char* ptr = static_cast<char*>(buf);
        while (totalRead < count)
        {
#ifdef _WIN32
            int bytesRead = _read(fd, ptr + totalRead, static_cast<unsigned int>(count - totalRead));
#else
            ssize_t bytesRead = read(fd, ptr + totalRead, count - totalRead);
#endif
            if (bytesRead <= 0)
                return false;
            totalRead += bytesRead;
        }
        return true;
    }

    static bool XisoWrite(int fd, const void* buf, size_t count)
    {
        size_t totalWritten = 0;
        const char* ptr = static_cast<const char*>(buf);
        while (totalWritten < count)
        {
#ifdef _WIN32
            int bytesWritten = _write(fd, ptr + totalWritten, static_cast<unsigned int>(count - totalWritten));
#else
            ssize_t bytesWritten = write(fd, ptr + totalWritten, count - totalWritten);
#endif
            if (bytesWritten <= 0)
                return false;
            totalWritten += bytesWritten;
        }
        return true;
    }

#ifdef _WIN32
    static bool CreateDir(const std::filesystem::path& path)
    {
        return CreateDirectoryW(path.wstring().c_str(), nullptr) || GetLastError() == ERROR_ALREADY_EXISTS;
    }
#else
    static bool CreateDir(const std::filesystem::path& path)
    {
        return mkdir(path.string().c_str(), 0755) == 0 || errno == EEXIST;
    }
#endif

    static bool ComputeHash(const std::filesystem::path& path, XXH128_hash_t& outHash)
    {
#ifdef _WIN32
        int fd = _wopen(path.wstring().c_str(), _O_RDONLY | _O_BINARY);
#else
        int fd = open(path.string().c_str(), O_RDONLY);
#endif
        if (fd < 0)
            return false;

        auto fileEnd = XisoLseek(fd, 0, SEEK_END);
        uint64_t fileSize = (fileEnd > 0) ? uint64_t(fileEnd) : 0;
        XisoLseek(fd, 0, SEEK_SET);

        XXH3_state_t* state = XXH3_createState();
        if (!state)
        {
#ifdef _WIN32
            _close(fd);
#else
            close(fd);
#endif
            return false;
        }

        XXH3_128bits_reset(state);

        std::vector<uint8_t> readBuf(BUFFER_SIZE);
        uint64_t totalRead = 0;
        uint64_t lastReported = 0;

        for (;;)
        {
#ifdef _WIN32
            int rd = _read(fd, readBuf.data(), static_cast<unsigned int>(readBuf.size()));
#else
            ssize_t rd = read(fd, readBuf.data(), readBuf.size());
#endif
            if (rd < 0)
            {
                XXH3_freeState(state);
#ifdef _WIN32
                _close(fd);
#else
                close(fd);
#endif
                return false;
            }

            if (rd > 0)
                XXH3_128bits_update(state, readBuf.data(), rd);

            totalRead += rd;

            if (s_callback && fileSize > 0 && totalRead - lastReported >= 16 * 1024 * 1024)
            {
                lastReported = totalRead;
                s_progress.phase = Phase::Hashing;
                s_progress.ratio = HASH_PHASE_END * static_cast<float>(static_cast<double>(totalRead) / static_cast<double>(fileSize));
                s_progress.currentFileName = "Verifying disc image...";
                s_callback(s_progress);
            }

            if (rd == 0)
                break;
        }

        outHash = XXH3_128bits_digest(state);
        XXH3_freeState(state);

#ifdef _WIN32
        _close(fd);
#else
        close(fd);
#endif

        return true;
    }

    static bool FindHeader(int fd)
    {
        char buffer[HEADER_MAGIC_LENGTH];

        auto tryOffset = [&](uint64_t offset) -> bool
        {
            if (XisoLseek(fd, HEADER_OFFSET + offset, SEEK_SET) < 0) return false;
            if (!XisoRead(fd, buffer, HEADER_MAGIC_LENGTH)) return false;
            if (memcmp(buffer, HEADER_MAGIC, HEADER_MAGIC_LENGTH) != 0) return false;
            s_discOffset = offset;
            return true;
        };

        if (tryOffset(0) ||
            tryOffset(XGD1_LSEEK_OFFSET) ||
            tryOffset(GLOBAL_LSEEK_OFFSET) ||
            tryOffset(XGD3_LSEEK_OFFSET))
        {
            return true;
        }

        auto fileSize = XisoLseek(fd, 0, SEEK_END);
        if (fileSize < 0)
            return false;

        static constexpr uint64_t SCAN_STEP = 0x10000;
        for (uint64_t offset = SCAN_STEP; offset + HEADER_OFFSET < uint64_t(fileSize); offset += SCAN_STEP)
        {
            if (tryOffset(offset))
                return true;
        }

        return false;
    }

    static bool ExtractFileData(int fd, const Entry& entry, const std::filesystem::path& outputDir, std::vector<uint8_t>& buffer)
    {
        auto outputPath = outputDir / entry.filename;

#ifdef _WIN32
        int outFd = _wopen(outputPath.wstring().c_str(), _O_WRONLY | _O_CREAT | _O_BINARY | _O_TRUNC, _S_IREAD | _S_IWRITE);
#else
        int outFd = open(outputPath.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
#endif
        if (outFd < 0)
            return false;

        if (XisoLseek(fd, static_cast<int64_t>(entry.startSector) * SECTOR_SIZE + s_discOffset, SEEK_SET) < 0)
        {
#ifdef _WIN32
            _close(outFd);
#else
            close(outFd);
#endif
            return false;
        }

        uint32_t remaining = entry.fileSize;
        while (remaining > 0)
        {
            uint32_t chunkSize = (remaining > BUFFER_SIZE) ? BUFFER_SIZE : remaining;
            if (!XisoRead(fd, buffer.data(), chunkSize))
            {
#ifdef _WIN32
                _close(outFd);
#else
                close(outFd);
#endif
                return false;
            }
            if (!XisoWrite(outFd, buffer.data(), chunkSize))
            {
#ifdef _WIN32
                _close(outFd);
#else
                close(outFd);
#endif
                return false;
            }
            remaining -= chunkSize;
            s_extractedBytes += chunkSize;

            if (s_callback && s_totalBytes > 0 && s_extractedBytes - s_lastExtractionReport >= 16 * 1024 * 1024)
            {
                s_lastExtractionReport = s_extractedBytes;
                s_progress.phase = Phase::Extracting;
                s_progress.ratio = HASH_PHASE_END + EXTRACT_PHASE_RANGE * static_cast<float>(static_cast<double>(s_extractedBytes) / static_cast<double>(s_totalBytes));
                s_callback(s_progress);
            }
        }

#ifdef _WIN32
        _close(outFd);
#else
        close(outFd);
#endif
        return true;
    }

    static bool TraverseDirectory(int fd, int64_t dirStart, const std::filesystem::path& outputDir, std::vector<uint8_t>& buffer, bool countOnly = false)
    {
        if (XisoLseek(fd, dirStart, SEEK_SET) < 0)
            return false;

        std::vector<Entry> pending;
        pending.reserve(128);

        uint16_t lOffset = 0;
        Entry current{};

    read_entry:
        {
            uint16_t tmp;
            if (!XisoRead(fd, &tmp, 2)) return false;

            if (tmp == PAD_SHORT)
            {
                if (lOffset == 0)
                    goto left_processed;

                lOffset = lOffset * DWORD_SIZE + (SECTOR_SIZE - (lOffset * DWORD_SIZE) % SECTOR_SIZE);
                if (XisoLseek(fd, dirStart + static_cast<int64_t>(lOffset), SEEK_SET) < 0)
                    return false;
                goto read_entry;
            }

            lOffset = tmp;

            current.lOffset = tmp;
            if (!XisoRead(fd, &current.rOffset, 2)) return false;
            if (!XisoRead(fd, &current.startSector, 4)) return false;
            if (!XisoRead(fd, &current.fileSize, 4)) return false;
            if (!XisoRead(fd, &current.attributes, 1)) return false;
            if (!XisoRead(fd, &current.filenameLength, 1)) return false;
            if (current.filenameLength > 0)
            {
                if (!XisoRead(fd, current.filename, current.filenameLength)) return false;
            }
            current.filename[current.filenameLength] = '\0';
        }

        if (strcmp(current.filename, ".") == 0 || strcmp(current.filename, "..") == 0 ||
            strchr(current.filename, '/') != nullptr || strchr(current.filename, '\\') != nullptr)
        {
            if (lOffset)
            {
                if (XisoLseek(fd, dirStart + static_cast<int64_t>(lOffset) * DWORD_SIZE, SEEK_SET) < 0)
                    return false;
                goto read_entry;
            }
            if (current.rOffset)
            {
                if (XisoLseek(fd, dirStart + static_cast<int64_t>(current.rOffset) * DWORD_SIZE, SEEK_SET) < 0)
                    return false;
                goto read_entry;
            }
            goto left_processed;
        }

        if (lOffset)
        {
            pending.push_back(current);
            if (XisoLseek(fd, dirStart + static_cast<int64_t>(lOffset) * DWORD_SIZE, SEEK_SET) < 0)
                return false;
            goto read_entry;
        }

        goto process_current;

    process_current:
        if (current.attributes & ATTRIBUTE_DIR)
        {
            if (!countOnly)
            {
                auto subDir = outputDir / current.filename;
                CreateDir(subDir);
            }

            if (current.fileSize > 0)
            {
                int64_t subDirStart = static_cast<int64_t>(current.startSector) * SECTOR_SIZE + s_discOffset;
                if (!TraverseDirectory(fd, subDirStart, outputDir / current.filename, buffer, countOnly))
                    return false;

                lOffset = 0;
                if (XisoLseek(fd, dirStart, SEEK_SET) < 0)
                    return false;
            }
        }
        else
        {
            if (countOnly)
            {
                s_totalBytes += current.fileSize;
            }
            else
            {
                s_progress.currentFileName = current.filename;
                if (!ExtractFileData(fd, current, outputDir, buffer))
                    return false;
            }
        }

        if (current.rOffset)
        {
            if (XisoLseek(fd, dirStart + static_cast<int64_t>(current.rOffset) * DWORD_SIZE, SEEK_SET) < 0)
                return false;
            goto read_entry;
        }

        goto left_processed;

    left_processed:
        if (pending.empty())
            return true;

        current = pending.back();
        pending.pop_back();
        goto process_current;
    }

    bool Extract(const std::filesystem::path& isoPath, const std::filesystem::path& outputDir, ProgressCallback callback)
    {
        s_callback = callback;
        s_progress = {};
        s_extractedBytes = 0;
        s_totalBytes = 0;
        s_lastExtractionReport = 0;

        XXH128_hash_t hash{};
        if (!ComputeHash(isoPath, hash))
            return false;

        bool hashValid = false;
        for (const auto& validHash : VALID_HASHES)
        {
            if (hash.low64 == validHash.low64 && hash.high64 == validHash.high64)
            {
                hashValid = true;
                break;
            }
        }

        if (!hashValid)
            return false;

#ifdef _WIN32
        int fd = _wopen(isoPath.wstring().c_str(), _O_RDONLY | _O_BINARY);
#else
        int fd = open(isoPath.string().c_str(), O_RDONLY);
#endif
        if (fd < 0)
            return false;

        if (!FindHeader(fd))
        {
#ifdef _WIN32
            _close(fd);
#else
            close(fd);
#endif
            return false;
        }

        int32_t rootDirSector, rootDirSize;
        if (!XisoRead(fd, &rootDirSector, 4) || !XisoRead(fd, &rootDirSize, 4))
        {
#ifdef _WIN32
            _close(fd);
#else
            close(fd);
#endif
            return false;
        }

        if (!rootDirSector && !rootDirSize)
        {
#ifdef _WIN32
            _close(fd);
#else
            close(fd);
#endif
            return false;
        }

        CreateDir(outputDir);

        s_progress.phase = Phase::Extracting;
        s_progress.ratio = HASH_PHASE_END;
        s_totalBytes = 0;

        std::vector<uint8_t> buffer(BUFFER_SIZE);

        int64_t rootDirStart = static_cast<int64_t>(rootDirSector) * SECTOR_SIZE + s_discOffset;

        if (!TraverseDirectory(fd, rootDirStart, outputDir, buffer, true))
        {
#ifdef _WIN32
            _close(fd);
#else
            close(fd);
#endif
            return false;
        }

        bool result = TraverseDirectory(fd, rootDirStart, outputDir, buffer, false);

#ifdef _WIN32
        _close(fd);
#else
        close(fd);
#endif

        s_progress.ratio = 1.0f;
        s_progress.currentFileName.clear();
        if (s_callback)
            s_callback(s_progress);

        s_callback = nullptr;
        return result;
    }
}
