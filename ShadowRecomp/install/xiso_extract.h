// Credit to extract-xiso devs (in@fishtank.com and other extract-xiso contributors) for a lot of the implementation here
// https://github.com/XboxDev/extract-xiso/

#pragma once

#include <string>
#include <filesystem>
#include <functional>
#include <atomic>

namespace XisoExtract
{
    enum class Phase : uint32_t
    {
        Hashing,
        Extracting,
    };

    struct Progress
    {
        Phase phase = Phase::Hashing;
        float ratio = 0.0f;
        std::string currentFileName;
    };

    using ProgressCallback = std::function<void(const Progress&)>;

    bool Extract(const std::filesystem::path& isoPath, const std::filesystem::path& outputDir, ProgressCallback callback = nullptr);
}
