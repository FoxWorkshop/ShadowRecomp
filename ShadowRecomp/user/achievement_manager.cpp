#include "achievement_manager.h"
#include <os/logger.h>
#include <user/config.h>
#include <ui/achievement_overlay.h>
#include <gpu//video.h>


// achievements icons
#include <res/images/common/pure_hero.png.h>
#include <res/images/common/pure_dark.png.h>
#include <res/images/common/neutral_hero.png.h>
#include <res/images/common/PlaceHolder.png.h>
#include <res/images/common/overlay.png.h>
#include <res/images/common/pure_hero_dark.png.h>
#include <res/images/common/hero_hero.png.h>
#include <res/images/common/hero_dark.png.h>
#include <res/images/common/dark_dark.png.h>
#include <res/images/common/dark_hero.png.h>
#include <decompressor.h>

std::unordered_map<uint16_t, std::unique_ptr<GuestTexture>> g_achievementsTextureCache;

#define NUM_RECORDS sizeof(AchievementManager::Data.Records) / sizeof(AchievementData::SHRRecord)

time_t AchievementManager::GetTimestamp(uint16_t id)
{
    for (int i = 0; i < NUM_RECORDS; i++)
    {
        if (!Data.Records[i].ID)
            break;

        if (Data.Records[i].ID == id)
            return Data.Records[i].Timestamp;
    }

    return 0;
}

size_t AchievementManager::GetTotalRecords()
{
    auto result = 0;

    for (int i = 0; i < NUM_RECORDS; i++)
    {
        if (!Data.Records[i].ID)
            break;

        result++;
    }

    return result;
}

bool AchievementManager::IsUnlocked(uint16_t id)
{
    if (!id)
        return false;

    for (int i = 1; i < NUM_RECORDS; i++)
    {
        if (Data.Records[i].ID == id)
            return true;
    }

    return false;
}

void AchievementManager::Unlock(uint16_t id)
{
    if (IsUnlocked(id))
        return;

    Data.Records[id].ID = id;
    Data.Records[id].Timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    if (Config::AchievementNotifications)
        AchievementOverlay::Open(id);
}

void AchievementManager::UnlockAll()
{
    for (uint16_t i = 1; i <= SHR_RECORDS; i++)
        AchievementManager::Unlock(i);
}

void AchievementManager::Reset()
{
    Data = {};
}

bool AchievementManager::LoadBinary()
{

    AchievementManager::Reset();


    BinStatus = EAchBinStatus::Success;

    auto dataPath = GetDataPath(true);

    if (!std::filesystem::exists(dataPath))
    {
        // Try loading base achievement data as fallback.
        dataPath = GetDataPath(false);

        if (!std::filesystem::exists(dataPath))
            return true;
    }

    std::error_code ec;
    auto fileSize = std::filesystem::file_size(dataPath, ec);
    auto dataSize = sizeof(AchievementData);

    if (fileSize != dataSize)
    {
        BinStatus = EAchBinStatus::BadFileSize;
        return true;
    }

    std::ifstream file(dataPath, std::ios::binary);

    if (!file)
    {
        BinStatus = EAchBinStatus::IOError;
        return true;
    }

    AchievementData data{};

    file.read((char*)&data.Signature, sizeof(data.Signature));

    if (!data.VerifySignature())
    {
        BinStatus = EAchBinStatus::BadSignature;
        file.close();
        return true;
    }

    file.read((char*)&data.Version, sizeof(data.Version));

    if (!data.VerifyVersion())
    {
        BinStatus = EAchBinStatus::BadVersion;
        file.close();
        return true;
    }

    file.seekg(0);
    file.read((char*)&data, sizeof(data));

    if (!data.VerifyChecksum())
    {
        BinStatus = EAchBinStatus::BadChecksum;
        file.close();
        return true;
    }

    file.close();

    memcpy(&Data, &data, dataSize);

    return false;
}

bool AchievementManager::SaveBinary(bool ignoreStatus)
{
    if (!ignoreStatus && BinStatus != EAchBinStatus::Success)
    {
        LOGN_WARNING("Achievement data will not be saved in this session!");
        return false;
    }

    LOGN("Saving achievements...");

    std::ofstream file(GetDataPath(true), std::ios::binary);

    if (!file)
    {
        LOGN_ERROR("Failed to write achievement data.");
        return false;
    }

    Data.Checksum = Data.CalculateChecksum();

    file.write((const char*)&Data, sizeof(AchievementData));
    file.close();

    BinStatus = EAchBinStatus::Success;

    return true;
}

std::unique_ptr<GuestTexture> LoadAchivementTexture(uint16_t id)
{
    switch (id)
    {
    case 1:
        return LOAD_ZSTD_TEXTURE(g_puredark);
    case 3:
        return LOAD_ZSTD_TEXTURE(g_darkdark);
    case 4:
        return LOAD_ZSTD_TEXTURE(g_darkhero);
    case 6:
        return LOAD_ZSTD_TEXTURE(g_neutralhero);
    case 7:
        return LOAD_ZSTD_TEXTURE(g_herodark);
    case 8:
        return LOAD_ZSTD_TEXTURE(g_herohero);
    case 9:
        return LOAD_ZSTD_TEXTURE(g_pureherodark);
    case 10:
        return LOAD_ZSTD_TEXTURE(g_purehero);
    }

    return LOAD_ZSTD_TEXTURE(g_placeholder);
}

void InitAchievementTextures()
{
    for (uint16_t i = 1; i <= SHR_RECORDS; i++)
    {
        g_achievementsTextureCache[i] = LoadAchivementTexture(i);
    }
}

std::vector<Achievement> GetAchievements(ELanguage language)
{
	std::vector<Achievement> result;

    for (int i = 0; i < SHR_RECORDS; i++)
    {

        Achievement achievement{};
        achievement.ID = i + 1;
        achievement.Name = g_shrLocale[i + 1][language].Name;
        achievement.UnlockedDesc = g_shrLocale[i + 1][language].UnlockedDesc;
        achievement.LockedDesc = g_shrLocale[i + 1][language].LockedDesc;
        achievement.pImageBuffer = (uint8_t*)g_achievementsTextureCache[i + 1].get();


        result.push_back(achievement);
    }

    return result;
}

Achievement GetAchievement(ELanguage language, const uint8_t id)
{
    Achievement achievement{};
    achievement.ID = id;
    achievement.Name = g_shrLocale[id][language].Name;
    achievement.UnlockedDesc = g_shrLocale[id][language].UnlockedDesc;
    achievement.LockedDesc = g_shrLocale[id][language].LockedDesc;
    achievement.pImageBuffer = (uint8_t*)g_achievementsTextureCache[id].get();
    return achievement;
}

// Library has 326 ending, this is checked with index 0-325
bool IsLibrarySequenceUnlocked(int index)
{
	auto base = g_memory.base;
    auto ptr = X86_LOAD_U32(0x549640);

    if (!ptr)
		return false;

    int bit = 1 << (index & 0x1F);

	uint32_t address = ptr + 4 * ((unsigned int)index >> 5) + 0x16d0;
	printf("address %08X\n", g_memory.Translate(address));
    int word = X86_LOAD_U32(address);

    // ecx = ebx = count (index)
    //	ctx.eflags.update_test_flags_U32(X86_LOAD_U32(ctx.edx.u32 + ctx.ecx.u32 * 0x4 + 0x16d0) & ctx.eax.u32);
    return (bit & word) != 0;
}

//  this match the route map order so far
// 0 = dark ending, this might be 0-9 to check the 10 endings
// 4 neutral dark ending 
// 5 = neutral hero ending
bool IsEndingUnlocked(uint16_t flag)
{
    auto base = g_memory.base;
    auto ptr = X86_LOAD_U32(0x549640);

    if (!ptr)
        return false;

    uint32_t address = ptr + 0xB88;

    int ptrAddr = X86_LOAD_U32(address);

    auto ptrFLag = (uint8_t*)g_memory.Translate(ptrAddr);
    if ((*((WORD*)ptrFLag) & (unsigned __int16)flag) != 0)
    {
        return true;

    }

    return false;
}

void SetEndingAchievementMidAsmHook(X86Register& rEcx)
{
   const uint8_t id = rEcx.u8_lo;
    if (id > 9 || id < 0)
        return;

    AchievementManager::Unlock(id);
    AchievementManager::SaveBinary();
}