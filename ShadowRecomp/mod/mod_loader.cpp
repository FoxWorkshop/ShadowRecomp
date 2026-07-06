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


// TOneFile::CheckFileID
X86_FUNC_IMPL(__imp__sub_1ED50);
X86_FUNC(sub_1ED50)
{
	char* fileName = (char*)GetHostParameter(2, CallConvention::Thiscall, ParameterType::Pointer);
	//printf("TOneFile::CheckFileID: %s\n", fileName);
	__imp__sub_1ED50(ctx, base);
}

uint32_t g_currentIndexFileFromOneFile = 0;
std::string g_currentNameFileFromOneFile = "";

std::string GetCurrentNameFileFromOneFile()
{
	return g_currentNameFileFromOneFile;
}


// TOneFile::CheckFileName
X86_FUNC_IMPL(__imp__sub_1ED10);
X86_FUNC(sub_1ED10)
{
	//printf("TOneFile::CheckFileID: %s\n", fileName);
	__imp__sub_1ED10(ctx, base);
	if (ctx.eax.u32)
	{
		g_currentNameFileFromOneFile = reinterpret_cast<const char*>(g_memory.Translate((ctx.eax.u32)));
		printf("%s\n", g_currentNameFileFromOneFile.c_str());
	}
}

// RpClumpForAllAtomics
X86_FUNC_IMPL(__imp__sub_2CA9F0);
X86_FUNC(sub_2CA9F0)
{
	__imp__sub_2CA9F0(ctx, base);
}



void GetInfoFromOneFileMidAsmHook(X86Register& rEax, X86Register& rEbx)
{
	g_currentIndexFileFromOneFile = rEbx.u32;


}

void GetNameFileFromOneFileMidAsmHook()
{

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


	// because enabled code don't store varialbe name, we have to compare with the full code list in a separate json file
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
