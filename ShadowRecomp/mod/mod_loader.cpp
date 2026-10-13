#include "mod_loader.h"

#include <kernel/function.h>
#include <kernel/heap.h>
#include <user/paths.h>
#include <xxHashMap.h>
#include <nlohmann/json.hpp>
#include <mod/IniFile.h>
#include <os/logger.h>
#include <user/config.h>
#include <cctype>
#include <algorithm>
#include <patches/global.h>
#include <kernel/io/file_system.h>


namespace fs = std::filesystem;

// Used to store the mod's file replacement data
struct ModArchiveWorker
{
	std::string archive; // ex: .one file 
	std::string asset;  // og asset name
	std::filesystem::path file;
	bool isWildcard; // used for massive assets replacement in different archives at once, ex: "event/*" 
	bool fromIncludeDir; // for codeless config folders, they get priority over the mod root data
};

struct Mod
{
	std::string Name;
	std::string Author;
	std::string Description;
	std::string Version;
	std::string Folder;
	std::string ID;
	bool MainSaveRedirect;

	std::vector<std::filesystem::path> includeDirs; // for codeless config
	std::filesystem::path commonDir; // for regular game files replacement that run regardless of config option
	std::vector<ModArchiveWorker> archiveWorker; // for individual file replacement in archives
	xxHashMap<std::vector<uint32_t>> replaceRulesByAsset; // key: asset name hash, value: index in replaceRules
};

enum class ModReservedID : uint8_t
{
	Reloaded,
	Multiplayer
};

const char* reservedIDName[] =
{
	"shthr.Reloaded",
	"shthr.Multiplayer"
};

static std::vector<Mod> g_mods;


std::filesystem::path ModLoader::ResolvePath(std::string_view path)
{
	std::string_view root;

	size_t sepIndex = path.find(":\\");
	if (sepIndex != std::string_view::npos)
	{
		root = path.substr(0, sepIndex);
		path.remove_prefix(sepIndex + 2);
	}


	if (g_mods.empty())
		return {};

	thread_local xxHashMap<std::filesystem::path> s_cache;

	XXH64_hash_t hash = XXH3_64bits(path.data(), path.size());
	auto findResult = s_cache.find(hash);
	if (findResult != s_cache.end())
		return findResult->second;

	std::string pathStr(path);
	std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
	std::filesystem::path fsPath(std::move(pathStr));


	for (int i = g_mods.size() - 1; i >= 0; i--) // iterate in reverse to prioritize later mods in the list (which are higher priority in mod loader)
	{
		auto& mod = g_mods[i];

		for (auto& includeDir : mod.includeDirs)
		{
			std::filesystem::path modPath = includeDir / fsPath;
			if (std::filesystem::exists(modPath))
				return s_cache.emplace(hash, modPath).first->second;
		}

		std::filesystem::path commonDir = mod.commonDir;
		std::filesystem::path modPath = commonDir / fsPath;
		if (std::filesystem::exists(modPath))
			return s_cache.emplace(hash, modPath).first->second;
	}

	return s_cache.emplace(hash, std::filesystem::path{}).first->second;
}

std::vector<std::filesystem::path>* ModLoader::GetIncludeDirectories(size_t modIndex)
{
	return modIndex < g_mods.size() ? &g_mods[modIndex].includeDirs : nullptr;
}

static std::string ToLower(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return str;
}

static void Trim(std::string& str)
{
	size_t start = str.find_first_not_of(" \t\r\n");
	size_t end = str.find_last_not_of(" \t\r\n");
	str = start == std::string::npos ? std::string() : str.substr(start, end - start + 1);
}

// Lower asset name because the game is not consistent with that for some reason.
static XXH64_hash_t HashLowerName(std::string name)
{
	name = ToLower(std::move(name));
	return XXH3_64bits(name.c_str(), name.size());
}

// The game is inconsistent with cases and slashes in archive names (eg: "event\event2201_sceneA.one", "EVENT/...SCENEA.ONE").
// so force accurate matching.
static std::string NormalizeArchiveName(std::string name)
{
	name = ToLower(std::move(name));
	std::replace(name.begin(), name.end(), '\\', '/');

	size_t drive = name.find(":/");

	if (drive != std::string::npos)
		name.erase(0, drive + 2);

	while (name.starts_with("./"))
		name.erase(0, 2);

	while (name.starts_with('/'))
		name.erase(0, 1);

	if (name.ends_with(".one"))
		name.resize(name.size() - 4);

	return name;
}


static bool MatchWildcard(std::string_view pattern, std::string_view str)
{
	size_t p = 0;
	size_t s = 0;
	size_t starP = std::string_view::npos;
	size_t starS = 0;

	while (s < str.size())
	{
		if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == str[s]))
		{
			p++;
			s++;
		}
		else if (p < pattern.size() && pattern[p] == '*')
		{
			starP = p++;
			starS = s;
		}
		else if (starP != std::string_view::npos)
		{
			p = starP + 1;
			s = ++starS;
		}
		else
		{
			return false;
		}
	}

	while (p < pattern.size() && pattern[p] == '*')
		p++;

	return p == pattern.size();
}

static bool isMatchingName(const ModArchiveWorker& modWK, const std::string& archive)
{
	return modWK.isWildcard ? MatchWildcard(modWK.archive, archive) : modWK.archive == archive;
}

// Handle priority for file replacement rules
// codeless config folder get priority over mod files in root folder
// And a single archive replacement beats wildcard.
static bool handleRulesPriority(const ModArchiveWorker& a, const ModArchiveWorker& b)
{
	if (a.fromIncludeDir != b.fromIncludeDir)
		return a.fromIncludeDir;

	if (a.isWildcard != b.isWildcard)
		return !a.isWildcard;

	if (a.archive.size() != b.archive.size())
		return a.archive.size() > b.archive.size();

	return a.archive < b.archive;
}

static void AddIndividualFileReplacement(Mod* modinf, ModArchiveWorker rule)
{
	modinf->replaceRulesByAsset[HashLowerName(rule.asset)].push_back(static_cast<uint32_t>(modinf->archiveWorker.size()));
	modinf->archiveWorker.push_back(std::move(rule));
}

// Load master ini file that list all the individual files to replace
// This works with a key/value pair, where the key is the asset name and the value is the path to the replacement file.
static void LoadReplaceIni(const fs::path& iniPath, Mod* modinf, bool fromIncludeDir)
{
	if (!fs::exists(iniPath))
		return;

	IniFile ini(iniPath);
	fs::path iniDir = iniPath.parent_path();

	for (auto& [group, values] : ini)
	{
		std::string archive = group;
		Trim(archive);
		archive = NormalizeArchiveName(archive);

		for (auto& [key, data] : *values)
		{
			std::string asset = key;
			std::string value = data;
			Trim(asset);
			Trim(value);

			if (asset.empty() || asset[0] == '#')
				continue;

			if (archive.empty())
			{
				LOGF_ERROR("{}: {}: {} is not under an [archive] group", modinf->Name, iniPath.string(), asset);
				continue;
			}

			if (asset.ends_with('+'))
			{
				LOGF_WARNING("{}: [{}] {}= is not supported yet, skipped", modinf->Name, group, asset);
				continue;
			}

			fs::path assetPath = iniDir / value;

			if (value.empty() || !fs::is_regular_file(assetPath))
			{
				LOGF_ERROR("{}: [{}] {}: file does not exist: {}", modinf->Name, group, asset, assetPath.string());
				continue;
			}

			bool isWildcard = archive.find_first_of("*?") != std::string::npos;
			AddIndividualFileReplacement(modinf, { archive, ToLower(asset), assetPath, isWildcard, fromIncludeDir });
		}
	}
}


static std::vector<std::string> ReadOneFileAssetNames(const fs::path& path)
{
	std::vector<std::string> names;
	std::ifstream file(path, std::ios::binary);
	char header[0xB0];

	if (!file.read(header, sizeof(header)))
		return names;

	uint32_t count = *reinterpret_cast<uint32_t*>(header + 0x1C);
	if (count > 0x4000)
		return names;

	std::vector<char> table(56 * count);
	if (!file.read(table.data(), table.size()))
		return names;

	for (uint32_t i = 0; i < count; i++)
		names.push_back(ToLower(std::string(table.data() + 56 * i, strnlen(table.data() + 56 * i, 44))));

	return names;
}


static void LogIndividualFilesReplacement()
{
	std::vector<std::pair<std::string, std::vector<std::string>>> archives;
	std::error_code ec;
	fs::path gameRoot = MainGameFolder;

	for (auto it = fs::recursive_directory_iterator(gameRoot, ec); !ec && it != fs::recursive_directory_iterator(); it.increment(ec))
	{
		if (!it->is_regular_file() || ToLower(it->path().extension().string()) != ".one")
			continue;

		std::string archive = NormalizeArchiveName(fs::relative(it->path(), gameRoot).generic_string());
		bool isUsed = std::any_of(g_mods.begin(), g_mods.end(), [&](const Mod& mod)
			{
				return std::any_of(mod.archiveWorker.begin(), mod.archiveWorker.end(), [&](const ModArchiveWorker& modWK) { return isMatchingName(modWK, archive); });
			});

		if (isUsed)
			archives.emplace_back(std::move(archive), ReadOneFileAssetNames(it->path()));
	}

	for (auto& mod : g_mods)
	{
		for (auto& modwk : mod.archiveWorker)
		{
			uint32_t matchesCount = 0;
			for (auto& [archiveName, names] : archives)
			{
				if (isMatchingName(modwk, archiveName) && std::find(names.begin(), names.end(), modwk.asset) != names.end())
					matchesCount++;
			}

			if (matchesCount == 0)
				LOGF_IMPL(Error, "Mod Loader", "{}: [{}] {} matched no archive, check the names", mod.Name, modwk.archive, modwk.asset);
			else
				LOGF_IMPL(Utility, "Mod Loader", "{}: [{}] {} matched {} archive(s)", mod.Name, modwk.archive, modwk.asset, matchesCount);
		}
	}
}

// archives loaded through LoadOneFileAsync (events) never fill their fname slot,
// so we keep their og name directly, using the TOneFile address as a key instead.
static std::mutex g_oneFileMutex;
static ankerl::unordered_dense::map<uint32_t, std::string> g_asyncOneFileNames;

// Files replacement are resolved once when the archive is loaded, so we can store them in a map for quick lookup later.
struct OneFileReplacement
{
	std::filesystem::path filePath;
	uint32_t size;
};
static ankerl::unordered_dense::map<uint32_t, xxHashMap<OneFileReplacement>> g_oneFileReplacements;
static bool flagIndividualFileReplace = false;

static std::string GetOneFileName(uint32_t oneFileAddr, TOneFile* oneFile)
{
	if (oneFile->fname[0])
		return NormalizeArchiveName(std::string(oneFile->fname, strnlen(oneFile->fname, sizeof(oneFile->fname))));

	std::lock_guard lock(g_oneFileMutex);
	auto it = g_asyncOneFileNames.find(oneFileAddr);
	return it != g_asyncOneFileNames.end() ? it->second : std::string();
}

static const ModArchiveWorker* FindReplacedFiles(const std::string& archive, XXH64_hash_t assetHash)
{
	for (int i = g_mods.size() - 1; i >= 0; i--) // iterate in reverse to prioritize later mods in the list (which are higher priority in mod loader)
	{
		auto ruleIt = g_mods[i].replaceRulesByAsset.find(assetHash);

		if (ruleIt == g_mods[i].replaceRulesByAsset.end())
			continue;

		const ModArchiveWorker* currentArchive = nullptr;

		for (uint32_t index : ruleIt->second)
		{
			auto& modWK = g_mods[i].archiveWorker[index];

			// if mod is replacing the same asset in multiple archives, pick the one with the highest priority
			if (isMatchingName(modWK, archive) && (!currentArchive || handleRulesPriority(modWK, *currentArchive)))
				currentArchive = &modWK;
		}

		if (currentArchive)
			return currentArchive;
	}

	return nullptr;
}

// TOneFile::LoadOneFileAsync
X86_FUNC_IMPL(__imp__sub_1F7F0);
X86_FUNC(sub_1F7F0)
{
	if (auto name = (const char*)GetHostParameter(2, CallConvention::Thiscall, ParameterType::Pointer))
	{
		std::lock_guard lock(g_oneFileMutex);
		g_asyncOneFileNames[ctx.ecx.u32] = NormalizeArchiveName(name);
	}

	__imp__sub_1F7F0(ctx, base);
}

// In order to implement individual file replacement in .one files, we need to patch the size of the asset table.
// since the original code will allocate a buffer based on the original content
// we have to patch each asset size that mods are replacing to have an accurate size and avoid memory overflow.
// the game will then later use this size for allocating memory properly. (This table fix up runs for both normal and
// async loads.)
// later on a separate function we patch the file loading function to load the replacement file instead of the original one.
X86_FUNC_IMPL(__imp__sub_1F360);
X86_FUNC(sub_1F360)
{
	uint32_t oneFileAddr = X86_LOAD_U32(ctx.esp.u32 + 4);
	__imp__sub_1F360(ctx, base);

	if (!flagIndividualFileReplace) // give up if no mods are replacing individual files
		return;

	TOneFile* oneFile = (TOneFile*)g_memory.Translate(oneFileAddr);
	std::string archive = GetOneFileName(oneFileAddr, oneFile);
	xxHashMap<OneFileReplacement> replacements;

	if (!archive.empty() && oneFile->assetStreamData)
	{
		uint32_t numAssets = X86_LOAD_U32(oneFile->assetStreamData + 16);

		for (uint32_t index = 2; index < numAssets + 2; index++)
		{
			uint32_t record = oneFile->assetStreamData + 56 * index;
			auto assetName = (const char*)g_memory.Translate(record + 52);
			XXH64_hash_t assetHash = HashLowerName(std::string(assetName, strnlen(assetName, 44))); // fixed 44-byte name field

			auto archiveWK = FindReplacedFiles(archive, assetHash);

			if (!archiveWK)
				continue;

			std::error_code ec;
			uintmax_t size = std::filesystem::file_size(archiveWK->file, ec);
			if (ec)
				continue;

			X86_STORE_U32(record + 96, static_cast<uint32_t>(size));
			replacements[assetHash] = { archiveWK->file, static_cast<uint32_t>(size) };
		}
	}

	// the TOneFile address gets reused by the next archive, so always overwrite what we had
	std::lock_guard lock(g_oneFileMutex);
	if (replacements.empty())
		g_oneFileReplacements.erase(oneFileAddr);
	else
		g_oneFileReplacements[oneFileAddr] = std::move(replacements);
}

// TOneFile::OpenData
// Now that we patched the asset table size, we can load the replacement file
// instead of the original one.
X86_FUNC_IMPL(__imp__sub_1F570);
X86_FUNC(sub_1F570)
{
	TOneFile* oneFile = (TOneFile*)GetHostParameter(1, CallConvention::Thiscall, ParameterType::Pointer);
	auto assetName = oneFile ? TOneFileCheckFileName(oneFile, GetHostParameter(2, CallConvention::Thiscall)) : nullptr;
	std::optional<OneFileReplacement> replacement;

	if (assetName)
	{
		XXH64_hash_t assetHash = HashLowerName(std::string(assetName, strnlen(assetName, 44)));

		std::lock_guard lock(g_oneFileMutex);
		auto oneIt = g_oneFileReplacements.find(ctx.ecx.u32);
		if (oneIt != g_oneFileReplacements.end())
		{
			auto assetIt = oneIt->second.find(assetHash);
			if (assetIt != oneIt->second.end())
				replacement = assetIt->second;
		}
	}

	std::ifstream file;

	if (replacement)
		file.open(replacement->filePath, std::ios::binary);

	if (!file.is_open())
	{
		__imp__sub_1F570(ctx, base);
		return;
	}


	file.read(reinterpret_cast<char*>(GetHostParameter(3, CallConvention::Thiscall, ParameterType::Pointer)), replacement->size);
	ctx.eax.u32 = static_cast<uint32_t>(file.gcount());
	LOGF_IMPL(Utility, "Mod Loader", "Loading file: \"{}\"", reinterpret_cast<const char*>(replacement->filePath.u8string().c_str()));

	// async readers (events) pass a thread func and wait for a TOneFileNotice saying the data is ready
	if (uint32_t threadFunc = GetHostParameter(4, CallConvention::Thiscall))
	{
		// TOneFileNotice::Create
		GuestToHostFunction<void, CallConvention::Cdecl>(sub_1F470, threadFunc, ctx.eax.u32, GetHostParameter(5, CallConvention::Thiscall));
	}

	ctx.AddEsp(5); // we loaded the file ourselves, clean up the stack for the skipped function (4 args + ret addr)
}


void ModLoader::Init()
{
	using json = nlohmann::json;
	const std::filesystem::path& userPath = GetUserPath();

	const std::filesystem::path& gPath = GetGamePath();

	std::filesystem::path profileListPath = gPath / "mods/.modloader/profiles/profiles.json";
	std::filesystem::path defaultProfile = gPath / "mods/.modloader/profiles/Default.json";

	std::filesystem::path currentProfile = defaultProfile;

	int ind_profile = -1;
	if (std::filesystem::exists(profileListPath))
	{
		std::ifstream ifs(profileListPath);
		json json_profiles = json::parse(ifs);
		ifs.close();

		ind_profile = json_profiles.value("ProfileIndex", 0);
		json proflist = json_profiles["ProfilesList"];
		std::string profname = proflist.at(ind_profile)["Filename"].get<std::string>();
		currentProfile = gPath / "mods/.modloader/profiles" / profname;
	}


	if (ind_profile == -1 && std::filesystem::exists(defaultProfile) == false)
	{
		return; // give up if no profile is found
	}

	std::ifstream ifs(currentProfile);

	json json_profile;
	json_profile = json::parse(ifs);
	ifs.close();

	json json_mods = json_profile["EnabledMods"];
	for (unsigned int i = 1; i <= json_mods.size(); i++)
	{
		std::string mod_fname = json_mods.at(i - 1);


		std::filesystem::path modDir = gPath / "mods" / mod_fname;

		if (!std::filesystem::exists(modDir))
			continue;

		std::filesystem::path modIni = modDir / "mod.ini";

		if (!std::filesystem::exists(modIni))
			continue;

		std::unique_ptr<IniFile> ini_mod(new IniFile(modIni));
		const IniGroup* const modinfo = ini_mod->getGroup("");

		const std::string mod_nameA = modinfo->getString("Name");
		LOGF_IMPL(Utility, "Mod Loader", "Loading: \"{}\"", reinterpret_cast<const char*>(mod_nameA.c_str()));

		Mod modinf = {
		mod_nameA,
		modinfo->getString("Author"),
		modinfo->getString("Description"),
		modinfo->getString("Version"),
		"",
		modinfo->getString("ModID"),
		modinfo->getBool("RedirectMainSave"),
		};

		std::filesystem::path modGameDir = modDir / "game";
		std::string modDirStr = modGameDir.string();
		std::replace(modDirStr.begin(), modDirStr.end(), '\\', '/');
		modGameDir = modDirStr;

		bool commonDirExist = false;
		bool includeDirExist = false;

		if (std::filesystem::exists(modGameDir))
		{
			modinf.commonDir = modGameDir;
			commonDirExist = true;
		}

		if (ini_mod->hasGroup("Config"))
		{
			int dirCount = ini_mod->getInt("Config", "IncludeDirCount", 0);
			if (dirCount > 0)
			{
				std::filesystem::path configPath = modDir / "config.ini";
				std::unique_ptr<IniFile> configini = std::filesystem::exists(configPath) ? std::make_unique<IniFile>(configPath) : nullptr;

				for (uint16_t d = 0; d < dirCount; d++)
				{
					// Retrieves defaults from mod.ini
					std::string dirStr = "IncludeDir" + std::to_string(d);
					std::string incDirPath = ini_mod->getString("Config", dirStr);

					// Loop through all groups in the config ini to check if the current IncludeDir key is present.
					if (configini)
					{

						for (auto group = configini->begin(); group != configini->end(); group++)
						{
							// If the corresponding IncludeDir key is found, update the paths with the config ini info.
							if (group->second->hasKey(dirStr))
							{
								incDirPath = group->second->getString(dirStr);
								break;
							}
						}
					}


					if (incDirPath.empty())
						continue;

					LOGF_IMPL(Utility, "Mod Loader", "Mod {} use config folder {}", reinterpret_cast<const char*>(mod_nameA.c_str()), reinterpret_cast<const char*>(incDirPath.c_str()));
					includeDirExist = true;

					std::filesystem::path modConfigGameDir = modDir / incDirPath / "game";
					std::string modDirStr = modConfigGameDir.string();
					std::replace(modDirStr.begin(), modDirStr.end(), '\\', '/');
					modConfigGameDir = modDirStr;
					modinf.includeDirs.push_back(modConfigGameDir);
				}
			}

		}

		if (strcmp(reservedIDName[(uint8_t)ModReservedID::Reloaded], modinf.ID.c_str()) == 0)
		{
			// add toggle for reloaded
		}

		if (strcmp(reservedIDName[(uint8_t)ModReservedID::Multiplayer], modinf.ID.c_str()) == 0)
		{
			// add toggle for mp
		}

		LoadReplaceIni(modDir / "replace.ini", &modinf, false);

		// for each codeless config folder, load its replace.ini if it exists
		for (auto& includeDir : modinf.includeDirs)
		{
			LoadReplaceIni(includeDir.parent_path() / "replace.ini", &modinf, true); // next to the folder's "game" dir
		}

		if (commonDirExist || includeDirExist || !modinf.archiveWorker.empty())
		{
			flagIndividualFileReplace |= !modinf.archiveWorker.empty();
			g_mods.push_back(std::move(modinf));
		}
	}

	if (flagIndividualFileReplace)
		LogIndividualFilesReplacement();


	// --- load codes if any ---

	std::filesystem::path codePath = gPath / "mods/.modloader/Codes.json";
	if (std::filesystem::exists(codePath) == false)
	{
		return;
	}

	json Enabledjson_codes = json_profile["EnabledCodes"];

	std::vector<std::string> enabledCodes;

	// first we store what codes are enabled
	for (unsigned int i = 0; i < Enabledjson_codes.size(); i++)
	{
		std::string code_fname = Enabledjson_codes.at(i);
		enabledCodes.push_back(code_fname);
	}


	// because enabled code don't store variable name, we have to compare with the full code list in a separate json file
	// this should be changed as it's way too much steps for checking a code

	std::ifstream ifCodes(codePath);
	json json_codes;
	json_codes = json::parse(ifCodes);
	ifCodes.close();


	bool valueFound = false;
	for (auto& def : g_configDefinitions)
	{
		if (!def->IsHidden() || def->GetSection() != "Codes")
			continue;

		/* NOTE: this is inefficient, but it happens
		   once on boot for a handful of codes at release
		   and is temporary until we support real code mods. */

		json json_codesList = json_codes["Codes"];


		for (std::string curCode : enabledCodes)
		{
			for (unsigned int i = 0; i < json_codesList.size(); i++)
			{
				std::string codeInternalName = json_codesList.at(i)["InternalName"].get<std::string>();
				std::string codeName = json_codesList.at(i)["Name"].get<std::string>();
				if (codeInternalName == curCode && codeName == def->GetName())
				{
					LOGF_IMPL(Utility, "Mod Loader", "Loading code: \"{}\"", curCode);
					*(bool*)def->GetValue() = true;
					break;
				}
			}
		}
	}

	enabledCodes.clear();
}