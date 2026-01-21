#include "mod_loader.h"

#include <kernel/function.h>
#include <kernel/heap.h>
#include <user/paths.h>
#include <xxHashMap.h>

struct Mod
{
	std::vector<std::filesystem::path> includeDirs;
	bool merge = false;
	ankerl::unordered_dense::set<std::filesystem::path> readOnly;
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


	for (auto& mod : g_mods)
	{

		for (auto& includeDir : mod.includeDirs)
		{
			std::filesystem::path modPath = includeDir / fsPath;
			if (std::filesystem::exists(modPath))
				return s_cache.emplace(hash, modPath).first->second;
		}
	}

	return s_cache.emplace(hash, std::filesystem::path{}).first->second;
}

std::vector<std::filesystem::path>* ModLoader::GetIncludeDirectories(size_t modIndex)
{
	return modIndex < g_mods.size() ? &g_mods[modIndex].includeDirs : nullptr;
}

void ModLoader::Init()
{

}
