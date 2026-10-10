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


namespace fs = std::filesystem;

struct ModArchiveFile
{
	xxHashMap<std::filesystem::path> assets; // key: asset name hash, value: replacement file path
	bool isOneFileLoaded;
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
	xxHashMap<ModArchiveFile> individualFiles; // for individual file replacement in archives 

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

// Lower asset name because the game is not consistent with that for some reason.
static XXH64_hash_t HashLowerName(std::string name)
{
	std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return (char)std::tolower(c); });
	return XXH3_64bits(name.c_str(), name.size());
}

// check for individual file replacement
// this currently work by having a folder hiearchy that match .one file structure, and an index.txt file that list the files to be replaced
void FindIndividualFiles(const fs::path& modGameRoot, Mod* modinf)
{

	if (!fs::exists(modGameRoot) || !fs::is_directory(modGameRoot))
		return;

	for (const auto& entry : fs::recursive_directory_iterator(modGameRoot))
	{
		if (!entry.is_regular_file() || entry.path().filename() != "index.txt")
			continue;

		fs::path fullFolderPath = entry.path().parent_path(); // mods/NameOfTheMod/Game/Character/ArchiveName.one

		// find the "Game" component and build the base path up to and including it
		fs::path gameBase;
		bool found = false;
		for (const auto& part : fullFolderPath)
		{
			gameBase /= part;
			if (part == "game")
			{
				found = true;
				break;
			}
		}

		if (!found)
			continue;

		fs::path relativePart = fs::relative(fullFolderPath, gameBase); // ex "Character/ArchiveName.one"

		std::string oneFileName = relativePart.string();
		std::replace(oneFileName.begin(), oneFileName.end(), '\\', '/');


		auto index = entry.path();
		std::ifstream index_file(index);

		if (!index_file.is_open())
		{
			const std::string mod_nameA = modinf->Name;
			LOGF_IMPL(Error, "{}:", "Failed to open index file : {}", reinterpret_cast<const char*>(mod_nameA.c_str()), index.string());
			return;
		}

		std::string line;
		uint32_t line_number = 0;

		xxHashMap<std::filesystem::path> assets;

		while (!index_file.eof())
		{
			try
			{
				++line_number;
				getline(index_file, line);

				if (line.length() == 0 || line[0] == '#')
				{
					continue;
				}


				fs::path asset_path = entry.path().parent_path() / line;

				std::string assetPathStr = asset_path.string();
				std::replace(assetPathStr.begin(), assetPathStr.end(), '\\', '/');

				//remove "," in case user added that after each file
				if (assetPathStr.back() == ',')
					assetPathStr.pop_back();

				asset_path = assetPathStr;

				if (!std::filesystem::exists(asset_path))
				{
					LOGF_ERROR("{} Asset entry on line {} does not exist: {}", modinf->Name, std::to_string(line_number), assetPathStr.c_str());
					continue;
				}

				auto assetsHash = HashLowerName(asset_path.filename().string());
				assets[assetsHash] = asset_path;

			}
			catch (std::exception& exception)
			{
				LOGF_ERROR("{} An exception occurred while parsing texture index on line {}: {}", modinf->Name, std::to_string(line_number), exception.what());

				assets.clear();
				return;
			}
		}

		if (assets.empty() == false)
		{
			auto archiveHash = HashLowerName(oneFileName);
			modinf->individualFiles[archiveHash].assets = assets;
		}
	}
}


// archives loaded through LoadOneFileAsync (events) never fill their fname slot,
// so we keep their og name hash directly, using the TOneFile address as a key instead.
static std::mutex g_asyncOneFileMutex;
static ankerl::unordered_dense::map<uint32_t, XXH64_hash_t> g_asyncOneFileHashes;

static XXH64_hash_t HashOneName(std::string name)
{
	std::replace(name.begin(), name.end(), '\\', '/'); // events use backslashes
	return HashLowerName(name.substr(0, name.find_last_of('.'))); // remove extension
}

static XXH64_hash_t GetOneFileHash(uint32_t oneFileAddr, TOneFile* oneFile)
{
	if (oneFile->fname[0])
		return HashOneName(oneFile->fname);

	std::lock_guard lock(g_asyncOneFileMutex);
	auto it = g_asyncOneFileHashes.find(oneFileAddr);
	return it != g_asyncOneFileHashes.end() ? it->second : 0;
}

static std::filesystem::path* FindReplacementFile(XXH64_hash_t oneHash, const char* assetName)
{
	for (int i = g_mods.size() - 1; i >= 0; i--) // iterate in reverse to prioritize later mods in the list (which are higher priority in mod loader)
	{
		auto oneIt = g_mods[i].individualFiles.find(oneHash);
		if (oneIt == g_mods[i].individualFiles.end())
			continue;

		auto assetIt = oneIt->second.assets.find(HashLowerName(assetName));
		if (assetIt != oneIt->second.assets.end())
			return &assetIt->second;
	}

	return nullptr;
}

// TOneFile::LoadOneFileAsync
X86_FUNC_IMPL(__imp__sub_1F7F0);
X86_FUNC(sub_1F7F0)
{
	if (auto name = (const char*)GetHostParameter(2, CallConvention::Thiscall, ParameterType::Pointer))
	{
		std::lock_guard lock(g_asyncOneFileMutex);
		g_asyncOneFileHashes[ctx.ecx.u32] = HashOneName(name);
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

	TOneFile* oneFile = (TOneFile*)g_memory.Translate(oneFileAddr);
	XXH64_hash_t oneHash = GetOneFileHash(oneFileAddr, oneFile);
	if (!oneHash)
		return;

	uint32_t numAssets = X86_LOAD_U32(oneFile->assetStreamData + 16);

	for (uint32_t index = 2; index < numAssets + 2; index++)
	{
		uint32_t record = oneFile->assetStreamData + 56 * index;
		auto filePath = FindReplacementFile(oneHash, (const char*)g_memory.Translate(record + 52)); // fixed 44-byte name field

		std::error_code ec;
		if (filePath)
		{
			uintmax_t size = std::filesystem::file_size(*filePath, ec);
			if (!ec)
				X86_STORE_U32(record + 96, static_cast<uint32_t>(size));
		}
	}
}

// TOneFile::OpenData
// Now that we patched the asset table size, we can load the replacement file 
// instead of the original one.
X86_FUNC_IMPL(__imp__sub_1F570);
X86_FUNC(sub_1F570)
{
	TOneFile* oneFile = (TOneFile*)GetHostParameter(1, CallConvention::Thiscall, ParameterType::Pointer);
	auto assetName = oneFile ? TOneFileCheckFileName(oneFile, GetHostParameter(2, CallConvention::Thiscall)) : nullptr;
	XXH64_hash_t oneHash = assetName ? GetOneFileHash(ctx.ecx.u32, oneFile) : 0;
	auto filePath = oneHash ? FindReplacementFile(oneHash, assetName) : nullptr;

	std::ifstream file;

	if (filePath)
		file.open(*filePath, std::ios::binary | std::ios::ate);

	if (!file.is_open())
	{
		__imp__sub_1F570(ctx, base);
		return;
	}

	uint32_t size = static_cast<uint32_t>(file.tellg());
	file.seekg(0);
	file.read(reinterpret_cast<char*>(GetHostParameter(3, CallConvention::Thiscall, ParameterType::Pointer)), size);
	ctx.eax.u32 = static_cast<uint32_t>(file.gcount());
	LOGF_IMPL(Utility, "Mod Loader", "Loading file: \"{}\"", reinterpret_cast<const char*>(filePath->u8string().c_str()));

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

				if (std::filesystem::exists(configPath))
				{
					// Process from config.ini
					std::unique_ptr<IniFile> configini = std::make_unique<IniFile>(configPath);

					for (uint16_t d = 0; d < dirCount; d++)
					{
						// Retrieves defaults from mod.ini
						std::string dirStr = "IncludeDir" + std::to_string(d);
						std::string incDirPath = ini_mod->getString("Config", dirStr);

						// Loop through all groups in the config ini to check if the current IncludeDir key is present.
						for (auto group = configini->begin(); group != configini->end(); group++)
						{
							// If the corresponding IncludeDir key is found, update the paths with the config ini info.
							if (group->second->hasKey(dirStr))
							{
								incDirPath = group->second->getString(dirStr);
								LOGF_IMPL(Utility, "Mod Loader", "Mod {} use config folder {}", reinterpret_cast<const char*>(mod_nameA.c_str()), reinterpret_cast<const char*>(incDirPath.c_str()));
								includeDirExist = true;
								break;
							}
						}

						if (includeDirExist)
						{
							std::filesystem::path modConfigGameDir = modDir / incDirPath / "game";
							std::string modDirStr = modConfigGameDir.string();
							std::replace(modDirStr.begin(), modDirStr.end(), '\\', '/');
							modConfigGameDir = modDirStr;
							modinf.includeDirs.push_back(modConfigGameDir);
						}

					}
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

		if (commonDirExist || includeDirExist)
		{
			for (auto& includeDir : modinf.includeDirs)
			{
				if (std::filesystem::exists(includeDir))
				{
					FindIndividualFiles(includeDir, &modinf);
				}
			}
			FindIndividualFiles(modGameDir, &modinf);
			g_mods.push_back(modinf);
		}
	}


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