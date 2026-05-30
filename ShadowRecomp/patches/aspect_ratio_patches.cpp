#include <kernel/function.h>
#include <patches/aspect_ratio_patches.h>
#include <gpu/video.h>
#include <ui/imgui_utils.h>
#include <user/config.h>
#include <ui/game_window.h>
#include <api/CSD/csd.h>
#include <os/logger.h>
#include <app.h>
#include <ui/options_menu.h>
#include "pause_patches.h"

bool readyToDebugConstant = false;
bool s_pauseTextRemoved = false;
#define widthMin 640.f
#define heightMin 480.f

static Mutex g_pathMutex;
static std::map<const void*, XXH64_hash_t> g_paths;
static std::optional<CsdModifier> g_sceneModifier{};
static std::optional<CsdModifier> g_castNodeModifier{};
static std::optional<CsdModifier> g_castModifier{};

static XXH64_hash_t HashStr(const std::string_view& value)
{
	return XXH3_64bits(value.data(), value.size());
}


static float ComputeScale(float aspectRatio)
{
	return ((aspectRatio * heightMin) / widthMin) / sqrt((aspectRatio * heightMin) / widthMin);
}

void AspectRatioPatches::ComputeOffsets()
{
	float width = Video::s_viewportWidth;
	float height = Video::s_viewportHeight;

	g_aspectRatio = width / height;
	g_aspectRatioGameplayScale = 1.0f;

	if (g_aspectRatio >= NARROW_ASPECT_RATIO)
	{
		g_aspectRatioOffsetX = (width - height * WIDE_ASPECT_RATIO) / 2.0f;
		g_aspectRatioOffsetY = 0.0f;
		g_aspectRatioScale = height / heightMin;

		// keep same scale above Steam Deck aspect ratio
		if (g_aspectRatio < WIDE_ASPECT_RATIO)
		{
			// interpolate to original 4:3 scale
			float steamDeckScale = g_aspectRatio / WIDE_ASPECT_RATIO;
			float narrowScale = ComputeScale(NARROW_ASPECT_RATIO);

			float lerpFactor = std::clamp((g_aspectRatio - NARROW_ASPECT_RATIO) / (STEAM_DECK_ASPECT_RATIO - NARROW_ASPECT_RATIO), 0.0f, 1.0f);
			g_aspectRatioGameplayScale = narrowScale + (steamDeckScale - narrowScale) * lerpFactor;
		}
	}
	else
	{
		// 4:3 crop
		g_aspectRatioOffsetX = (width - width * NARROW_ASPECT_RATIO) / 2.0f;
		g_aspectRatioOffsetY = (height - width / NARROW_ASPECT_RATIO) / 2.0f;
		g_aspectRatioScale = width / widthMin;
		g_aspectRatioGameplayScale = ComputeScale(NARROW_ASPECT_RATIO);
	}

	g_aspectRatioNarrowScale = std::clamp((g_aspectRatio - NARROW_ASPECT_RATIO) / (WIDE_ASPECT_RATIO - NARROW_ASPECT_RATIO), 0.0f, 1.0f);
	g_aspectRatioNarrowMargin = std::lerp(50.0f, 0.0f, g_aspectRatioNarrowScale);
	g_horzCentre = g_aspectRatioOffsetX + 640.0f * (1.0f - g_aspectRatioGameplayScale) * g_aspectRatioScale;
	g_vertCentre = g_aspectRatioOffsetY + 360.0f * (1.0f - g_aspectRatioGameplayScale) * g_aspectRatioScale;
	UpdateFontSize();
}


std::string NormalizeCSDPath(std::string& path)
{
	std::replace(path.begin(), path.end(), '\\', '/');

	auto pos = path.find("csdFiles");

	if (pos != std::string::npos)
		return path.substr(pos);

	return path;
}


static void EmplacePath(const void* key, const std::string_view& value)
{
	std::lock_guard lock(g_pathMutex);
	//printf("Emplace key %x, %s\n", key, value.data());
	g_paths.emplace(key, HashStr(value));

}

// Some csd have node name using Shift-JIS encoded Japanese characters being misinterpreted as Latin-1/UTF-8. 
// This can crash Recomp when storing those, so we fix that by replacing them with _
std::string SanitizeName(const char* str)
{
	if (!str) 
		return "";

	std::string result(str);
	for (char& c : result)
	{
		if ((unsigned char)c > 127)
			c = '_';
	}
	return result;
}

void TraverseCast(CSDScene* scene, uint32_t castNodeIndex, CSDCastNode* castNode, uint32_t castIndex, const std::string& parentPath)
{

	if (castIndex == ~0)
		return;

	auto& pCastLink = castNode->pCastLinks[castIndex];

	TraverseCast(scene, castNodeIndex, castNode, pCastLink.SiblingCastIndex, parentPath);

	std::string path = SanitizeName(parentPath.c_str());
	path = NormalizeCSDPath(path);

	for (size_t i = 0; i < scene->CastCount; i++)
	{
		auto& index = scene->pCastIndices[i];
		if (index.CastNodeIndex == castNodeIndex && index.CastIndex == castIndex)
		{
			path += index.pCastName;
			break;
		}
	}

	EmplacePath(castNode->pCasts[castIndex].get(), path);

	if (castNode->RootCastIndex == castIndex)
		EmplacePath(castNode, path);

	path += "/";


	std::string fixedpath = SanitizeName(path.c_str());

	TraverseCast(scene, castNodeIndex, castNode, castNode->pCastLinks[castIndex].ChildCastIndex, fixedpath);

	//LOGFN_UTILITY("CSD hierarchy: {}", fixedpath);
}

void TraverseScene(CSDScene* scene, std::string path)
{
	std::string pathFixed = SanitizeName(path.c_str());
	pathFixed = NormalizeCSDPath(pathFixed);
	EmplacePath(scene, pathFixed);

	pathFixed += "/";

	for (size_t i = 0; i < scene->CastNodeCount; i++)
	{
		auto& castNode = scene->pCastNodes[i];
		TraverseCast(scene, i, &castNode, castNode.RootCastIndex, pathFixed);
	}
}

void TraverseSceneNode(CSDSceneNode* node, const std::string& path)
{
	std::string pathFixed = SanitizeName(path.c_str());
	pathFixed = NormalizeCSDPath(pathFixed);

	EmplacePath(node, pathFixed);
	std::string basePath = pathFixed + "/";

	for (uint32_t i = 0; i < node->SceneCount; ++i)
	{
		auto& idx = node->pSceneIndices[i];
		void* scene = (void*)node->pScenes[idx.SceneIndex];

		std::string scenePath = basePath + idx.pSceneName.get();
		TraverseScene(node->pScenes[idx.SceneIndex], scenePath);
	}


	for (uint32_t j = 0; j < node->SceneNodeCount; ++j)
	{
		auto& idx = node->pSceneNodeIndices[j];
		CSDSceneNode* child = &node->pSceneNodes[idx.SceneNodeIndex];
		std::string childPath = basePath + idx.pSceneNodeName.get();
		TraverseSceneNode(child, childPath);
	}
}

// CSD_LOADER
X86_FUNC_IMPL(__imp__sub_2B47C0);
X86_FUNC(sub_2B47C0)
{
	void* thisPtr = (void*)GetHostParameter(1, CallConvention::Thiscall, ParameterType::Pointer);
	char* path = (char*)GetHostParameter(2, CallConvention::Thiscall, ParameterType::Pointer);

	__imp__sub_2B47C0(ctx, base);

	if (!thisPtr || !path)
		return;


	static const char* s_languages[6] =
	{
		"_JP",
		"_EN",
		"_GE",
		"_FR",
		"_SP",
		"_IT"
	};

	std::string str = path;

	auto pSuffix = s_languages[(int)Config::Language.Value];

	auto addrNodePtr = X86_LOAD_U32(g_memory.MapVirtual((const uint8_t*)thisPtr + 8));

	if (addrNodePtr)
	{
		auto resPtr = X86_LOAD_U32(addrNodePtr);
		CSDSceneNode* nodePtr = (CSDSceneNode*)(base + resPtr);

		if (nodePtr)
		{
			TraverseSceneNode(nodePtr, path);
		}
	}
}

// Chao::CSD::Scene::Render
X86_FUNC_IMPL(__imp__sub_38B5E0);
X86_FUNC(sub_38B5E0)
{
	auto pSceneAddr = X86_LOAD_U32(ctx.esp.u32 + 4);
	auto pScene = (CSDScene*)(base + pSceneAddr);

	g_sceneModifier = FindCsdModifier(pSceneAddr);

	
	__imp__sub_38B5E0(ctx, base);
}

//TCsdSystem::FreeMemory
X86_FUNC_IMPL(__imp__sub_184E0);
X86_FUNC(sub_184E0)
{
	auto ptr = X86_LOAD_U32(ctx.esp.u32 + 4);

	if (ptr != 0)
	{
		uint32_t magic1 = X86_LOAD_U32(ptr);
		uint32_t magic2 = X86_LOAD_U32(ptr + 0x20);


		if (magic1 == 0x4649584E && magic2 == 0x4A50436E) // NXIF, nCPJ
		{
			uint32_t fileSize = X86_LOAD_U32(ptr + 0x14);

			const uint8_t* base_key = (const uint8_t*)(base + ptr);

			std::lock_guard lock(g_pathMutex);
			auto lower = g_paths.lower_bound(base_key);
			auto upper = g_paths.lower_bound(base_key + fileSize);
			g_paths.erase(lower, upper);

			LOGFN_UTILITY("CSD freed: 0x{:08X}", (uint64_t)base_key);
		}
	}
	__imp__sub_184E0(ctx, base);
}

void RenderCsdCastNodeMidAsmHook(X86Register& rEax)
{
	g_castNodeModifier = FindCsdModifier(rEax.u32);
}

void RenderCsdCastMidAsmHook(X86Register& rEsi)
{
	g_castModifier = FindCsdModifier(rEsi.u32);
}

bool Draw(X86Context& ctx, uint8_t* base, X86Func* original, void* vertexStride, uint32_t stride)
{
	CsdModifier modifier{};

	auto vpWidth = float(Video::s_viewportWidth);
	auto vpHeight = float(Video::s_viewportHeight);

	if (g_castModifier.has_value())
	{
		modifier = g_castModifier.value();
	}
	else if (g_castNodeModifier.has_value())
	{
		modifier = g_castNodeModifier.value();
	}
	else if (g_sceneModifier.has_value())
	{
		modifier = g_sceneModifier.value();
	}


	// Remove all flags if the aspect ratio is above 16:9.
	if ((modifier.Flags & CSD_MODIFIER_ULTRAWIDE_ONLY) != 0 && g_aspectRatio <= WIDE_ASPECT_RATIO)
		modifier.Flags &= (~modifier.Flags) | CSD_MODIFIER_ULTRAWIDE_ONLY;

	// Remove all flags if the aspect ratio is below 16:9.
	if ((modifier.Flags & CSD_MODIFIER_NARROW_ONLY) != 0 && g_aspectRatio >= WIDE_ASPECT_RATIO)
		modifier.Flags &= (~modifier.Flags) | CSD_MODIFIER_NARROW_ONLY;

	if ((modifier.Flags & CSD_SKIP) != 0 || ((modifier.Flags & CSD_HIDE_PAUSE) != 0 && OptionsMenu::s_isVisible))
	{
		TogglePauseCSD((modifier.Flags & CSD_HIDE_PAUSE) != 0);
		TogglePauseBossCSD((modifier.Flags & CSD_PAUSE_BOSS) != 0);
		// Skip drawing this cast.
		return false;
	}

	auto offsetX = 0.0f;
	auto offsetY = 0.0f;
	auto pivotX = 0.0f;
	auto pivotY = 0.0f;
	auto scaleX = 1.0f;
	auto scaleY = 1.0f;

	auto needsStretch = g_aspectRatio >= WIDE_ASPECT_RATIO;

	if (needsStretch && (modifier.Flags & CSD_STRETCH_HORIZONTAL) != 0)
	{
		scaleX = vpWidth / 640.f;
	}
	else
	{
		scaleX = g_aspectRatioScale;

	}

	if ((modifier.Flags & CSD_STRETCH_VERTICAL) != 0)
	{
		scaleY = vpHeight / 480.f;
	}
	else
	{
		scaleY = g_aspectRatioScale;

		if ((modifier.Flags & CSD_ALIGN_BOTTOM) != 0)
			offsetY = g_aspectRatioOffsetY * 2.0f;
		else if ((modifier.Flags & CSD_ALIGN_TOP) == 0)
			offsetY = g_aspectRatioOffsetY;

		if ((modifier.Flags & CSD_SCALE) != 0)
		{
			scaleY *= g_aspectRatioGameplayScale;

			if ((modifier.Flags & CSD_ALIGN_BOTTOM) != 0)
				offsetY += 480.f * (1.0f - g_aspectRatioGameplayScale) * g_aspectRatioScale;
			else if ((modifier.Flags & CSD_ALIGN_TOP) == 0)
				offsetY += g_vertCentre - g_aspectRatioOffsetY;

			offsetY += pivotY * g_aspectRatioScale;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		RwD3D8Vertex* vertexData = (RwD3D8Vertex*)vertexStride;

		auto x = offsetX + (vertexData[i].x - pivotX);
		auto y = offsetY + (vertexData[i].y - pivotY);

		if ((modifier.Flags & CSD_EXTEND_BOTTOM) != 0 && (i == 1 || i == 3))
		{
			float extraItems = 2.0f;
			float virtualItemSpacing = 40.f;
			y += virtualItemSpacing * extraItems;
			vertexData[i].y = round(y);
		}
	}

	return true;
}

bool CPlatformRWDrawPrimitive2DMidAsmHook(X86Register& rEcx)
{
	// ecx = vertex stride
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	return Draw(*ctx, base, nullptr, g_memory.Translate(rEcx.u32), 28) == false;
}

const uint32_t GuestDevicePtrAddr = 0x3E0A18;
//GetViewportOffsetAndScale
X86_FUNC_IMPL(__imp__sub_3D24B0);
X86_FUNC(sub_3D24B0)
{
	float* offsetOut = (float*)GetHostParameter(1, CallConvention::Stdcall, ParameterType::Pointer);
	float* scaleOut = (float*)GetHostParameter(2, CallConvention::Stdcall, ParameterType::Pointer);

	__imp__sub_3D24B0(ctx, base);
	auto device = X86_LOAD_U32(GuestDevicePtrAddr);
	device = device;

	if (readyToDebugConstant)
	{

		auto padding610 = X86_LOAD_F32(device + 0x94C + 0x10);
		auto padding614 = X86_LOAD_F32(device + 0x94C + 0x14);

		auto padding636 = X86_LOAD_F32(device + 0x970);
		auto padding68 = X86_LOAD_U32(device + 0x954);

		auto padding616 = X86_LOAD_F32(device + 0x95C);
		auto padding7 = X86_LOAD_F32(device + 0xEF8);
		auto padding74 = X86_LOAD_F32(device + 0xEFC);
		//printf("&D3D_g_pDevice->padding6[16] %f D3D_g_pDevice->padding7 %f D3D_g_pDevice->padding7[4] %f\n", padding616, padding7, padding74);



		//printf("\n");

		readyToDebugConstant = false;

	}
}

// -------------- CSD MODIFIERS --------------- //

const xxHashMap<CsdModifier> g_csdModifiers =
{
	// pause
	{ HashStr("csdFiles/pause/pause.xncp/Pause/PauseText"), { CSD_SKIP | CSD_HIDE_PAUSE } },
	{ HashStr("csdFiles/pause/pauseBoss.xncp/Pause/PauseText"), { CSD_SKIP | CSD_HIDE_PAUSE | CSD_PAUSE_BOSS } },
	//{ HashStr("csdFiles/pause/pause.xncp/Pause/Pause/Null_pause/bottom"), { CSD_EXTEND_BOTTOM } },
	//{ HashStr("csdFiles/pause/pause.xncp/Pause/Pause/Null_pause/bg"), { CSD_EXTEND_BOTTOM } },


};

std::optional<CsdModifier> FindCsdModifier(uint32_t data)
{
	XXH64_hash_t path;
	{
		std::lock_guard lock(g_pathMutex);

		auto findResult = g_paths.find(g_memory.Translate(data));

		if (findResult == g_paths.end())
			return {};

		path = findResult->second;
	}

	auto findResult = g_csdModifiers.find(path);

	if (findResult != g_csdModifiers.end())
		return findResult->second;

	return {};
}

// remove the infamous half-pixel offset issue 
// see https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#3.3.1%20Pixel%20Coordinate%20System
void D3DDevice_SetScreenSpaceOffset(float a1, float a2)
{
	;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3D1320, D3DDevice_SetScreenSpaceOffset);
