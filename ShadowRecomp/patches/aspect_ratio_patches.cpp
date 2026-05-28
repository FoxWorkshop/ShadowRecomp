#include <kernel/function.h>
#include <patches/aspect_ratio_patches.h>
#include <gpu/video.h>
#include <ui/imgui_utils.h>
#include <user/config.h>
#include <api/CSD/csd.h>
#include <os/logger.h>

bool readyToDebugConstant = false;
#define widthMin 640.f
#define heightMin 480.f

static Mutex g_pathMutex;
static std::map<const void*, XXH64_hash_t> g_paths;

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

void PatchResolutionCapMidAsmHook(X86Register& eax)
{

}


static void EmplacePath(const void* key, const std::string_view& value)
{
	std::lock_guard lock(g_pathMutex);

	g_paths.emplace(key, HashStr(value));
}


std::string SanitizeName(const char* str)
{
	if (!str) return "";
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

	LOGFN_UTILITY("CSD hierarchy: {}", fixedpath);
}

void TraverseScene(CSDScene* scene, std::string path)
{
	EmplacePath(scene, path);
	path += "/";

	for (size_t i = 0; i < scene->CastNodeCount; i++)
	{
		auto& castNode = scene->pCastNodes[i];
		TraverseCast(scene, i, &castNode, castNode.RootCastIndex, path);
	}
}

void TraverseSceneNode(CSDSceneNode* node, const std::string& path)
{
	EmplacePath(node, path);
	std::string basePath = path + "/";

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

	auto addrNodePtr = X86_LOAD_U32((uint32_t)thisPtr + 8);

	auto resPtr = X86_LOAD_U32(addrNodePtr);
	CSDSceneNode* nodePtr = (CSDSceneNode*)(base + resPtr);

	if (nodePtr)
	{
		TraverseSceneNode(nodePtr, path);
	}
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


//draw CSD element, this patches HUD to implement 16:9 support
// this handle title screen, menu and basic hud in game, but not all of it
X86_FUNC_IMPL(__imp__sub_38B5E0);
X86_FUNC(sub_38B5E0)
{

	UpdateUserConfigConstant(true);
	__imp__sub_38B5E0(ctx, base);
	UpdateUserConfigConstant(false);
}

//draw rest of the hud
X86_FUNC_IMPL(__imp__sub_281550);
X86_FUNC(sub_281550)
{

	UpdateUserConfigConstant(true);
	__imp__sub_281550(ctx, base);
	UpdateUserConfigConstant(false);
}


// menu / goal ring fadeout fix

X86_FUNC_IMPL(__imp__sub_19730);
X86_FUNC(sub_19730)
{

	UpdateUserConfigConstant(true);
	__imp__sub_19730(ctx, base);
	UpdateUserConfigConstant(false);
}


// patch background size for message
X86_FUNC_IMPL(__imp__sub_2A57E0);
X86_FUNC(sub_2A57E0)
{

	UpdateUserConfigConstant(true);
	__imp__sub_2A57E0(ctx, base);
	UpdateUserConfigConstant(false);
}


// remove the infamous half-pixel offset issue 
// see https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm#3.3.1%20Pixel%20Coordinate%20System
void D3DDevice_SetScreenSpaceOffset(float a1, float a2)
{
	;
}

GUEST_FUNCTION_HOOK_STDCALL(sub_3D1320, D3DDevice_SetScreenSpaceOffset);
