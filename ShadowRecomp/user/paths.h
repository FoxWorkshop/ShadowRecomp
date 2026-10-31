#pragma once

#include <mod/mod_loader.h>

#define USER_DIRECTORY "ShadowRecomp"

#ifndef GAME_INSTALL_DIRECTORY
#define GAME_INSTALL_DIRECTORY "."
#endif

extern std::filesystem::path g_executableRoot;

inline std::filesystem::path GetGamePath()
{
    return GAME_INSTALL_DIRECTORY;
}

bool CheckPortable();
std::filesystem::path BuildUserPath();
const std::filesystem::path& GetUserPath();

inline std::filesystem::path GetSavePath(bool checkForMods)
{
    if (checkForMods && !ModLoader::s_saveFilePath.empty())
        return ModLoader::s_saveFilePath.parent_path();
    else
        return GetUserPath() / "save";
}

inline std::filesystem::path GetSaveFilePath(bool checkForMods)
{
    if (checkForMods && !ModLoader::s_saveFilePath.empty())
        return ModLoader::s_saveFilePath;
    else
        return GetSavePath(false) / "Save.dat";
}
