#pragma once

#include <filesystem>
#include <functional>
#include <string>

namespace Installer
{
    struct Progress
    {
        float ratio = 0.0f;
        const char* currentFileName = "";
    };

    using ProgressCallback = std::function<void(const Progress&)>;

    bool Install(const std::filesystem::path& sourcePath, const std::filesystem::path& outputDir, const ProgressCallback& callback, std::string& errorMessage);
    bool CheckInstall(const std::filesystem::path& gameDir, const ProgressCallback& callback, std::string& errorMessage);
}
