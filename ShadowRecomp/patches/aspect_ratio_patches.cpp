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
#include "global.h"
#include <mod/mod_loader.h>

static const uint8_t CSDVertexCount = 4u; // CSD always use 4 vertex count in this game
bool readyToDebugConstant = false;
bool s_pauseTextRemoved = false;
bool s_ShouldPatchSubtitles = false;
#define widthMin 640.f
#define heightMin 480.f
static Mutex g_pathMutex;
static Mutex g_bufferMutex;
static std::map<const void*, XXH64_hash_t> g_paths;
static std::unordered_map<const uint8_t*, std::vector<const uint8_t*>> g_bufferRegistry;
static uint8_t* g_currentBuffer = nullptr;
static std::optional<CsdModifier> g_sceneModifier{};
static std::optional<CsdModifier> g_castNodeModifier{};
static std::optional<CsdModifier> g_castModifier{};
static uint32_t hudFlagPtr = 0;

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


// Used to detect when we're drawing subtitles, pause menu uses subtitles to display the current mission
// because we move the pause background due to new options count, the subtitles get out of place, so we need to move them as well so they can fit the new BG position
void AdjustSubtitlesPosition(uint32_t pMatrix)
{
	if (pMatrix == 0)
		return;

	auto base = g_memory.base;
	if (isDrawingPauseMenuSubtitle())
	{

		float _42 = X86_LOAD_F32(pMatrix + 0x34);
		_42 -= 0.03f * GetAddedPauseItemsCount(); // move down
		X86_STORE_F32(pMatrix + 0x34, _42);
	}
	else if (s_ShouldPatchSubtitles)
	{
		auto vpWidth = float(Video::s_viewportWidth);
		auto vpHeight = float(Video::s_viewportHeight);
		float aspectRatio = vpWidth / vpHeight;
		float ratio43 = 640.f / 480.f;
		if (aspectRatio > ratio43)
		{
			float _41 = X86_LOAD_F32(pMatrix + 0x30);
			_41 -= 0.05f;
			X86_STORE_F32(pMatrix + 0x30, _41);
		}
	}

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

	g_paths.emplace(key, HashStr(value));

	if (g_currentBuffer)
	{
		g_bufferRegistry[g_currentBuffer].push_back((const uint8_t*)key);

		//printf("Emplace key %x, %s\n", key, value.data());
	}

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

	path = SanitizeName(path.c_str());
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
	std::lock_guard lock2(g_bufferMutex);

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

	auto csdFileAddrPtr = X86_LOAD_U32(g_memory.MapVirtual((const uint8_t*)thisPtr + 4));

	if (!csdFileAddrPtr)
		return;

	// since we can't blindly free g_paths member since no size are stored, we will store the CSD file pointer as a key instead
	// each child (cast, node etc.) will be able to get identified properly this way
	auto csdFilePtr = X86_LOAD_U32(csdFileAddrPtr + 4);
	g_currentBuffer = (uint8_t*)g_memory.Translate(csdFilePtr);

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

	g_currentBuffer = nullptr;
}

static bool s_hudDisabled = false;

void ToggleHud(bool toggle)
{
	auto base = g_memory.base;
	X86_STORE_U16(0x54DE78, toggle ? hudFlagPtr : 0);
	X86_STORE_U8(0x4E4B74, toggle ? 1 : 0);
	s_hudDisabled = toggle == false;
}

void PatchDisableHudMidAsmHook()
{
	if (s_hudDisabled)
	{
		auto base = g_memory.base;
		X86_STORE_U16(0x54DE78, 0x0);
	}
}

// Chao::CSD::Scene::Render
X86_FUNC_IMPL(__imp__sub_38B5E0);
X86_FUNC(sub_38B5E0)
{
	auto res = X86_LOAD_U32(0x54DE78);
	if (res)
		hudFlagPtr = res;

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

			const uint8_t* base_key = (const uint8_t*)(base + ptr);

			std::lock_guard lock(g_pathMutex);

			auto it = g_bufferRegistry.find(base_key);
			if (it != g_bufferRegistry.end())
			{
				for (auto* child : it->second)
					g_paths.erase(child);

				g_bufferRegistry.erase(it);
				LOGFN_UTILITY("CSD freed: 0x{:08X}", (uint64_t)base_key);
			}
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

void Draw(X86Context& ctx, uint8_t* base, X86Func* original, void* vertexData, uint32_t stride)
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
		return;
	}


	auto drawOriginal = [&]()
		{

			ctx.SubEsp(1);
			original(ctx, base);
		};

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

	const bool isPauseBossWithExtendBottom = ((modifier.Flags & CSD_PAUSE_BOSS) != 0 && ((modifier.Flags & CSD_EXTEND_BOTTOM) != 0));

	uint8_t pauseItemCount = GetAddedPauseItemsCount();

	if (isPauseBossWithExtendBottom && pauseItemCount >= 2) // pause boss csd already have a big enough background to support 4 options
		pauseItemCount -= 1;

	float pauseYOffset = 15.f * (float)(pauseItemCount);


	if ((modifier.Flags & CSD_HIDE_PAUSE) != 0)
	{
		for (int i = 0; i < CSDVertexCount; i++)
		{
			auto csdVertex = (RwD3D8Vertex*)vertexData;

			if ((modifier.Flags & CSD_EXTEND_BOTTOM) != 0 && (i == 1 || i == 3))
			{
				csdVertex[i].y += round(pauseYOffset);
			}

			if ((modifier.Flags & CSD_MOVE_BOTTOM) != 0)
			{
				csdVertex[i].y += round(pauseYOffset);
			}
		}
	}

	bool shouldBeSkipped = false;
	// save original vertex values
	float originalX[CSDVertexCount], originalY[CSDVertexCount];
	memset(originalX, 0, sizeof(originalX));
	memset(originalY, 0, sizeof(originalY));
	for (int i = 0; i < CSDVertexCount; i++)
	{
		auto vertex = (RwD3D8Vertex*)vertexData + i;
		if (vertex)
		{
			originalX[i] = vertex->x;
			originalY[i] = vertex->y;

			if (originalX[i] < -10.f || originalX[i] > 640 + 20.f)
				shouldBeSkipped = true;
		}
	}


	drawOriginal();

	if ((modifier.Flags & CSD_HIDE_PAUSE) != 0)
	{
		for (int i = 0; i < CSDVertexCount; i++)
		{
			auto csdVertex = (RwD3D8Vertex*)vertexData;

			if ((modifier.Flags & CSD_EXTEND_BOTTOM) != 0 && (i == 1 || i == 3))
			{
				csdVertex[i].y -= round(pauseYOffset);
			}

			if ((modifier.Flags & CSD_MOVE_BOTTOM) != 0)
			{
				csdVertex[i].y -= round(pauseYOffset);
			}
		}
	}
}

X86_FUNC_IMPL(__imp__sub_2ECC80);
void CPlatformRWDrawPrimitive2DMidAsmHook(X86Register& rEcx)
{
	// ecx = vertex stride
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	Draw(*ctx, base, __imp__sub_2ECC80, g_memory.Translate(rEcx.u32), 28);
}

void CPlatformRWDrawPrimitive2DMidAsmHook2(X86Register& rEsi)
{
	// ecx = vertex stride
	auto ctx = GetX86Context();
	auto base = g_memory.base;
	Draw(*ctx, base, __imp__sub_2ECC80, g_memory.Translate(rEsi.u32), 28);
}

void DrawHudMidAsmHook(X86Register& rEax)
{
	if (Config::UIAlignmentMode.Value != EUIAlignmentMode::Centre || !rEax.u32)
		return;

	auto vpWidth = float(Video::s_viewportWidth);
	auto vpHeight = float(Video::s_viewportHeight);
	float stretchFactor = (vpWidth / vpHeight) / (640.f / 480.f);
	float centreX = 640.f / 2.f;
	float aspectRatio = vpWidth / vpHeight;
	float ratio43 = 640.f / 480.f;
	if (aspectRatio > ratio43)
	{
		RwD3D8Vertex* vertexHud = (RwD3D8Vertex*)(g_memory.Translate(rEax.u32));

		for (int i = 0; i < 4; i++)
		{
			vertexHud[i].x = centreX + (vertexHud[i].x - centreX) / stretchFactor;
		}
	}
}


// the game often use a quad model to draw a picture on the screen, usually to show Shadow memories, 
// but the quad model is not stretched to fit the screen, thus any high resolution won't cover the whole screen
// we can fix that by stretching the vertex
// note: this is inconsistent, some cutscenes have it scaled properly, some don't, because of course they do.

struct QuadStretchCalibration
{
	float calibrationX = 1.0f; // multiplier on top of the base formula
	float calibrationY = 1.0f;
};


thread_local QuadStretchCalibration* g_currentQuadCallib = nullptr;

const xxHashMap<QuadStretchCalibration> g_pictureDff =
{
	{ HashStr("ARK!PICTURE!O!.DFF"), { 2.2f, 1.2f } }, // cosmic fall intro (height is ignored for some reason??)
	{ HashStr("ARK!PICTURE!LOCATOR.DFF"), { 2.2f, 1.2f } },
	{ HashStr("FLASH!BACK!PIC01!O!.DFF"), { 1.6f, 1.1f } },
	{ HashStr("ADV2!FLASH!BACK!PIC01!O!.DFF"), { 1.0f, 1.1f } },
	{ HashStr("ADV2!FLASH!BACK!PIC02!O!.DFF"), { 1.0f, 1.1f } },
	{ HashStr("ADV2!FLASH!BACK!PIC03!O!.DFF"), { 1.0f, 1.1f } },
	{ HashStr("ADV2!FLASH!BACK!PIC04!O!.DFF"), { 1.0f, 1.1f } },
	{ HashStr("BD!SHIP!PIC01!O!.DFF"), { 1.5f, 1.1f } }, // ev 4202 sky troops intro
	{ HashStr("EGG!SHIP!PIC01!O!.DFF"), { 1.5f, 1.1f } }, // ev 4202 sky troops intro
	{ HashStr("EGG!SHIP!PIC02!O!.DFF"), { 1.5f, 1.1f } }, // ev 4202 sky troops intro

};

void FindPictureBackgroundModel(X86Context* ctx, uint8_t* base)
{
	auto pOneFileAsync = (TOneFileAsync*)g_memory.Translate(ctx->ecx.u32);
	if (pOneFileAsync)
	{
		auto pOneFile = pOneFileAsync->pOneFile.get();
		if (pOneFile)
		{
			auto assetName = TOneFileCheckFileName(pOneFile, pOneFileAsync->fid); //pOneFile->fname;
			std::string s = "";

			if (assetName)
				s = assetName;

			auto it = g_pictureDff.find(HashStr(s.c_str()));
			if (it != g_pictureDff.end())
			{
				g_currentQuadCallib = (QuadStretchCalibration*)&it->second;
			}
			else
			{
				g_currentQuadCallib = nullptr;
			}
		}
	}
}


void StretchPictureVertex(uint32_t atomicAddr, uint8_t* base)
{
	if (g_currentQuadCallib)
	{
		auto geometryPtr = X86_LOAD_U32(atomicAddr + 0x18);
		RpGeometry* geometry = (RpGeometry*)g_memory.Translate(geometryPtr);
		if (geometry && geometry->numVertices == 4 && geometry->numMorphTargets == 1) // pic data is always 4 vertex
		{
			auto morphTargetPtr = geometry->morphTarget.get();

			if (morphTargetPtr)
			{
				morphTargetPtr = morphTargetPtr;

				if (morphTargetPtr)
				{
					RwV3d* verts = morphTargetPtr->verts.get();
					for (int i = 0; i < geometry->numVertices; i++)
					{
						verts[i].x *= g_currentQuadCallib->calibrationX;
						verts[i].y *= g_currentQuadCallib->calibrationY;
					}

				}
			}
		}
	}
}

// callbacks used by RpClumpForAllAtomics
// we can use them to stretch the vertex of the picture model
X86_FUNC_IMPL(__imp__sub_29EB20);
X86_FUNC(sub_29EB20)
{
	if (ctx.esp.u32 + 4)
	{
		StretchPictureVertex(X86_LOAD_U32(ctx.esp.u32 + 4), base);
	}
	__imp__sub_29EB20(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_261A0);
X86_FUNC(sub_261A0)
{
	if (ctx.esp.u32 + 4)
	{
		StretchPictureVertex(X86_LOAD_U32(ctx.esp.u32 + 4), base);
	}
	__imp__sub_261A0(ctx, base);
}

X86_FUNC_IMPL(__imp__sub_25DF0);
X86_FUNC(sub_25DF0)
{
	if (ctx.esp.u32 + 4)
	{
		StretchPictureVertex(X86_LOAD_U32(ctx.esp.u32 + 4), base);
	}
	__imp__sub_25DF0(ctx, base);
}

// TOneFileAsyncRpClump::Read
X86_FUNC_IMPL(__imp__sub_1FE00);
X86_FUNC(sub_1FE00)
{
	auto mode = X86_LOAD_U32(ctx.ecx.u32 + 0x28);

	if (mode == 4)
	{
		FindPictureBackgroundModel(&ctx, base);
	}

	__imp__sub_1FE00(ctx, base);

	if (mode >= 4)
	{
		g_currentQuadCallib = nullptr;
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

// -------------- CSD MODIFIERS --------------- //

const xxHashMap<CsdModifier> g_csdModifiers =
{
	// pause
	{ HashStr("csdFiles/pause/pause.xncp/Pause/PauseText"), { CSD_SKIP | CSD_HIDE_PAUSE } },
	{ HashStr("csdFiles/pause/pauseBoss.xncp/Pause/PauseText"), { CSD_SKIP | CSD_HIDE_PAUSE | CSD_PAUSE_BOSS } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Pause/Null_pause/bottom"), { CSD_HIDE_PAUSE | CSD_EXTEND_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Pause/Null_pause/bg"), { CSD_HIDE_PAUSE | CSD_EXTEND_BOTTOM } },
	{ HashStr("csdFiles/pause/pauseBoss.xncp/Pause/Pause/Null_pause/bottom"), { CSD_HIDE_PAUSE | CSD_EXTEND_BOTTOM | CSD_PAUSE_BOSS } },
	{ HashStr("csdFiles/pause/pauseBoss.xncp/Pause/Pause/Null_pause/bg"), { CSD_HIDE_PAUSE | CSD_EXTEND_BOTTOM | CSD_PAUSE_BOSS } },

	// this is to move the mission board vertex below due to the pause menu bg being extended reaching the mission board position
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission_Board/Null_0189/bg1"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission_Board/Null_0189/bg2"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission_Board/Null_0189/bg3"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission_Board/Null_0189/bg4"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission_Board/Null_missionname"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/bg"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/line_top2"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/line_top"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/frame_edge"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/frame_edge___R_s_[(1)"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/frame_edge___R_s_["), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/frame_right"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/frame_left"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/line_side_left"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/line_side___R_s_["), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/Null_0190/line_side_left___R_s_["), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/iconshadow1"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/iconshadow2"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/iconshadow3"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/Mission/mission"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/MIconHero/highlight"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/MIconHero/hero"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/MIconNormal/highlight"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/MIconNormal/normal"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/MIconDark/dark"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/MIconDark/highlight"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/ChangeMission/change"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/ChangeMission/arrow_left"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/ChangeMission/arrow_right"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/SRText/text"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
	{ HashStr("csdFiles/pause/pause.xncp/Pause/SRText/button"), { CSD_HIDE_PAUSE | CSD_MOVE_BOTTOM } },
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
