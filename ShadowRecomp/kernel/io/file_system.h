#pragma once


static const std::string GameExecutableFile = "default.xbe";
const std::string MainGameFolder = "game";
static const std::string GameExecutable = MainGameFolder + "/" + GameExecutableFile;

struct FileSystem
{
    static std::filesystem::path ResolvePath(const std::string_view& path, bool checkForMods);
};
