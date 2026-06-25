#include "bc_diff.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <filesystem>
#include <vector>
#include <xxhash.h>
#include <unordered_map>

struct KnownTexture
{
    XXH64_hash_t hash;
    uint32_t width;
    uint32_t height;

};
static const std::unordered_map<std::string, KnownTexture> g_knownHashes = {
    
    { "000_squarebuttons.png", { 0x4da2581facf551b7, 64, 64  } },
    { "001_command_icons.png", { 0x3a722a3f52e5ba47, 256, 128 }  },
    { "000_VHArrow_xbox.png", { 0x128153115c84976f, 128, 64 } },
    // English
    { "en_gamemenu.png", { 0x4d3f5ad2ae3aa138, 256, 256 } },
    { "en_pause_main.png", { 0x9cb8f582eb427c23, 256, 256}  },
    // French
    { "fr_gamemenu.png", { 0x40b12bd811215f86, 256, 256} },
    { "fr_pause_main.png", { 0x57a38acc9c9bd901, 256, 256 }},
    // Spain
    { "sp_gamemenu.png", { 0x5d57615fe545a1be, 256, 256 }},
    { "sp_pause_main.png", { 0x83be4c4e30e9a107, 256, 256} },
    // Italian
    { "it_gamemenu.png", { 0x60485abb6d226f93, 256, 256 } },
    { "it_pause_main.png", {0x2ca2b37e3e36a3d7, 256, 256 } },
    // JP
    { "jp_gamemenu.png", { 0x11f9c6870ed50f7f, 256, 256 } },
    { "jp_pause_main.png", { 0xee81741588d9dcf7, 256, 256 } },
    // German
    { "ge_gamemenu.png", { 0x1381d7a2a9bd50c2, 256, 256 } },
    { "ge_pause_main.png", { 0x4e77aa11c67e4b37, 256, 256 } },

};

static std::vector<uint8_t> readAllBytes(const char* filePath)
{
    FILE* file = fopen(filePath, "rb");

    if (!file)
        return {};

    fseek(file, 0, SEEK_END);

    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    std::vector<uint8_t> data(fileSize);
    fread(data.data(), 1, fileSize, file);

    fclose(file);

    return data;
}

/**int OldMain(int argc, char** argv)
{
    if (argc != 4)
    {
        printf("Usage: %s [old directory] [new directory] [destination file]", argv[0]);
        return 0;
    }

    // Debug configuration doesn't compile without this???
    assert(argc == 4);

    std::filesystem::path oldDirectoryPath = argv[1];
    std::filesystem::path newDirectoryPath = argv[2];

    std::vector<BlockCompressionDiffPatchEntry> entries;
    std::vector<BlockCompressionDiffPatch> patches;
    std::vector<uint8_t> patchBytes;

    for (auto& oldFile : std::filesystem::recursive_directory_iterator(oldDirectoryPath))
    {
        auto newFile = newDirectoryPath / std::filesystem::relative(oldFile, oldDirectoryPath);
        if (!std::filesystem::exists(newFile))
        {
            fprintf(stderr, "Cannot locate %s\n", newFile.string().c_str());
            continue;
        }

        auto oldFileData = readAllBytes(oldFile.path().string().c_str());
        auto newFileData = readAllBytes(newFile.string().c_str());

        constexpr size_t BC_STRIDE = 8;

        if (oldFileData.size() != newFileData.size())
        {
            fprintf(stderr, "%s does not match %s in file size\n", oldFile.path().string().c_str(), newFile.string().c_str());
            continue;
        }

        if ((oldFileData.size() % BC_STRIDE) != 0)
        {
            fprintf(stderr, "%s is not aligned to %d bytes\n", oldFile.path().string().c_str(), BC_STRIDE);
            continue;
        }

        if (oldFileData.size() >= BC_STRIDE)
        {
            size_t patchIndex = patches.size();

            for (size_t i = 0; i < oldFileData.size() - BC_STRIDE + 1; i += BC_STRIDE)
            {
                if (memcmp(&oldFileData[i], &newFileData[i], BC_STRIDE) == 0)
                    continue;

                size_t patchBytesOffset = patchBytes.size();
                patchBytes.insert(patchBytes.end(), newFileData.begin() + i, newFileData.begin() + i + BC_STRIDE);

                if (patchIndex >= patches.size() || ((patches.back().destinationOffset + patches.back().patchBytesSize) != i))
                {
                    auto& patch = patches.emplace_back();
                    patch.destinationOffset = i;
                    patch.patchBytesOffset = patchBytesOffset;
                    patch.patchBytesSize = BC_STRIDE;
                }
                else
                {
                    patches.back().patchBytesSize += BC_STRIDE;
                }
            }

            size_t patchCount = patches.size() - patchIndex;
            if (patchCount != 0)
            {
                auto& entry = entries.emplace_back();
                entry.hash = XXH3_64bits(oldFileData.data(), oldFileData.size());
                entry.patchesOffset = patchIndex * sizeof(BlockCompressionDiffPatch);
                entry.patchCount = patchCount;

                printf("Generated BC patch for %s\n", oldFile.path().string().c_str());
            }
            else
            {
                printf("Skipping %s, files are identical\n", oldFile.path().string().c_str()); 
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](auto& lhs, auto& rhs) { return lhs.hash < rhs.hash; });

    BlockCompressionDiffPatchHeader header;
    header.entriesOffset = sizeof(BlockCompressionDiffPatchHeader);
    header.entryCount = entries.size();

    size_t patchesOffset = header.entriesOffset + sizeof(BlockCompressionDiffPatchEntry) * entries.size();
    size_t patchBytesOffset = patchesOffset + sizeof(BlockCompressionDiffPatch) * patches.size();

    for (auto& entry : entries)
        entry.patchesOffset += patchesOffset;

    for (auto& patch : patches)
        patch.patchBytesOffset += patchBytesOffset;

    FILE* file = fopen(argv[3], "wb");
    if (!file)
    {
        fprintf(stderr, "Cannot open %s for writing\n", argv[3]);
        return 1;
    }

    fwrite(&header, sizeof(header), 1, file);
    fwrite(entries.data(), sizeof(BlockCompressionDiffPatchEntry), entries.size(), file);
    fwrite(patches.data(), sizeof(BlockCompressionDiffPatch), patches.size(), file);
    fwrite(patchBytes.data(), 1, patchBytes.size(), file);
    fclose(file);

    return 0;
}*/

int main(int argc, char** argv)
{
    if (argc != 3)
    {
        printf("Usage: %s [new texture directory] [destination file]\n", argv[0]);
        return 0;
    }

    std::filesystem::path newDirectoryPath = argv[1];

    std::vector<BlockCompressionDiffEntry> entries;
    std::vector<uint8_t> pixelData;

    for (auto& newFile : std::filesystem::recursive_directory_iterator(newDirectoryPath))
    {
        if (!newFile.is_regular_file())
            continue;

        auto relativePath = std::filesystem::relative(newFile, newDirectoryPath).string();

        auto it = g_knownHashes.find(relativePath);
        if (it == g_knownHashes.end())
        {
            fprintf(stderr, "No known hash for %s, skipping\n", relativePath.c_str());
            continue;
        }

        auto newFileData = readAllBytes(newFile.path().string().c_str());
        if (newFileData.empty())
        {
            fprintf(stderr, "Failed to read %s\n", newFile.path().string().c_str());
            continue;
        }

        const KnownTexture& known = it->second;

        BlockCompressionDiffEntry entry{};
        entry.hash = known.hash;
        entry.dataOffset = static_cast<uint32_t>(pixelData.size());
        entry.dataSize = static_cast<uint32_t>(newFileData.size());
        entry.width = known.width;
        entry.height = known.height;

        pixelData.insert(pixelData.end(), newFileData.begin(), newFileData.end());
        entries.push_back(entry);

        printf("Packed replacement for %s (hash %016llx, %u bytes)\n",
            relativePath.c_str(), known.hash, entry.dataSize);
    }

    std::sort(entries.begin(), entries.end(), [](auto& lhs, auto& rhs) { return lhs.hash < rhs.hash; });

    BlockCompressionDiffPatchHeader header{};
    header.entriesOffset = sizeof(BlockCompressionDiffPatchHeader);
    header.entryCount = static_cast<uint32_t>(entries.size());

    size_t dataBlobOffset = header.entriesOffset + sizeof(BlockCompressionDiffEntry) * entries.size();

    for (auto& entry : entries)
        entry.dataOffset += static_cast<uint32_t>(dataBlobOffset);

    FILE* file = fopen(argv[2], "wb");
    if (!file)
    {
        fprintf(stderr, "Cannot open %s for writing\n", argv[2]);
        return 1;
    }

    fwrite(&header, sizeof(header), 1, file);
    fwrite(entries.data(), sizeof(BlockCompressionDiffEntry), entries.size(), file);
    fwrite(pixelData.data(), 1, pixelData.size(), file);
    fclose(file);

    printf("Wrote %zu entries to %s\n", entries.size(), argv[2]);
    return 0;
}
