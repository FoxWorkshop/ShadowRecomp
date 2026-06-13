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

enum EAchievementID
{
    Ach_LastStory = 11,
    Ach_AllARanks = 12,
    Ach_AllKeys = 13,
    Ach_ExpertMode = 14,
};
std::unordered_map<uint16_t, std::unique_ptr<GuestTexture>> g_achievementsTextureCache;

#define NUM_RECORDS sizeof(AchievementManager::Data.Records) / sizeof(AchievementData::SHRRecord)

time_t AchievementManager::GetTimestamp(uint16_t id)
{
    for (int i = 0; i < NUM_RECORDS; i++)
    {
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

    for (int i = 0; i < NUM_RECORDS; i++)
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

    // check if all achievements have been unlocked
    // if so unlock the final one
    const size_t unlockedCount = AchievementManager::GetTotalRecords();

    const uint8_t lastSlotID = SHR_RECORDS - 1;
    if (unlockedCount == lastSlotID)
    {
        Data.Records[lastSlotID].ID = SHR_RECORDS;
        Data.Records[lastSlotID].Timestamp = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        if (Config::AchievementNotifications)
            AchievementOverlay::Open(lastSlotID);
    }
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

    AchievementManager::Unlock(id + 1);
    AchievementManager::SaveBinary();
}

void UpdateARankCountMidAsmHook(X86Register& rEdx)
{
    if (rEdx.u32 >= 71)
    {
        AchievementManager::Unlock(Ach_AllARanks);
        AchievementManager::SaveBinary();
    }
}

// This function update key count in the save file for each stage
// The game doesn't seem to have a full key count data so we have to calculate manually
// dword_54DDDC is the index used per stage as an offset to browse the save data
// it starts at 5 in westopolis and increase following level internal order skipping bosses.
// thus, digital circuit which is the second internal stage will be 6, glyphic canyon will be 7 etc.
X86_FUNC_IMPL(__imp__sub_1B2C30);
X86_FUNC(sub_1B2C30)
{
    __imp__sub_1B2C30(ctx, base);

    auto pointerAddr = X86_LOAD_U32(0x549640);
    if (!pointerAddr)
        return;
   
    uint16_t keyCount = 0;
    // the game has 23 stages (22 on stage select + last way) 
    for (int i = 0; i < 23; i++)
    {
        int index = (i + 5); // index start at 5 because they count debug level (0-4)
   
        // check if each key has been found (5 keys per level)
        for (int j = 0; j < 5; j++)
        {
            auto indexSaveStruct = index + index * 0x2;
            indexSaveStruct <<= 3;
            auto offset = 0xf4 + (j * 4);
            uint32_t res = X86_LOAD_U32(pointerAddr + indexSaveStruct * 0x4 + offset);
            keyCount += res != 0xFFFFFFFF;
        }

    }

    if (keyCount >= 115)
    {
        AchievementManager::Unlock(Ach_AllKeys);
        AchievementManager::SaveBinary();
    }
}

const uint32_t g_menuModeAddr = 0x5022A4;
const uint32_t g_stageActionModeAddr = 0x5496C0;
const uint32_t g_currentStageIDAddr = 0x54DDD8;


//StageAction Goal action destructor
void StageActionGoalActionMidAsmHook()
{
    auto base = g_memory.base;
    auto menuModeAddr = X86_LOAD_U32(g_menuModeAddr);
    auto currentLevel = X86_LOAD_U32(g_currentStageIDAddr);
  
    // if we are in expert mode
    if (menuModeAddr == 6)
    {
        if (currentLevel == 700) // if level is last way
        {
            AchievementManager::Unlock(Ach_ExpertMode);
            AchievementManager::SaveBinary();
        }
    }
}
