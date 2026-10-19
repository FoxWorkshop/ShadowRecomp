#include <cstring>
#include <kernel/function.h>
#include <user/config.h>
#include <xbox.h>
#include <patches/global.h>

//HintRingInit aka the capsule with "?" 
X86_FUNC_IMPL(__imp__sub_1A2350);
X86_FUNC(sub_1A2350)
{
	if (Config::Hints.Value == false)
	{
		ctx.AddEsp(1);
	}
	else
	{
		__imp__sub_1A2350(ctx, base);
	}
}

//HintRingExec this handle the collision that trigger the display of the message
X86_FUNC_IMPL(__imp__sub_1A1E60);
X86_FUNC(sub_1A1E60)
{
	if (Config::Hints.Value == false)
	{
		ctx.AddEsp(2); 
	}
	else
	{
		__imp__sub_1A1E60(ctx, base);
	}
}

// sub_17E290 (ObjectUpdateVisibility (gadgets)) calls sub_17C130 (SetFadeAlpha)
// fading moves clumps into world 10 (_AL)
// most gadgets are supposed to use _A not _AL (hence the white flash, applying light when it shouldn't)
// _AL has backface culling while _A doesn't, so we only remove the light and keep the other properties of _AL
static std::unordered_map<uint32_t, uint32_t> g_fadeOriginalWorld;

// RenderWare name, I call these layers personally but 'world' is accurate
static uint32_t GetClumpWorld(uint8_t* base, uint32_t clump)
{
	return X86_LOAD_U32(clump + X86_LOAD_U32(0x55F4D8));
}

// render layers (world) table @ 0x4E50F0 (_O, _OS, _A, _AL etc...)
// Gadgets that have the lit flag should actually use AL, so we skip patching them
static bool IsUnlitLayer(uint8_t* base, uint32_t world)
{
	for (uint32_t layer = 0x4E50F0; layer < 0x4E50F0 + 15 * 0x18; layer += 0x18)
	{
		if (X86_LOAD_U32(layer + 0x8) == world)
		{
			return (X86_LOAD_U32(layer) & 0x1) == 0;
		}
	}
	return false;
}

// SetFadeAlpha
X86_FUNC_IMPL(__imp__sub_17C130);
X86_FUNC(sub_17C130)
{
	const uint32_t gadget = ctx.ecx.u32;
	if ((X86_LOAD_U32(gadget + 0x3C) & 0x40) == 0)
	{
		const uint32_t listEnd = X86_LOAD_U32(gadget + 0x50);
		for (uint32_t node = X86_LOAD_U32(listEnd); node != listEnd; node = X86_LOAD_U32(node))
		{
			const uint32_t clump = X86_LOAD_U32(node + 0x8);
			const uint32_t world = GetClumpWorld(base, clump);
			// only patch if the gadget is supposed to be in _A
			if (world && IsUnlitLayer(base, world))
			{
				g_fadeOriginalWorld[clump] = world;
			}
			else
			{
				g_fadeOriginalWorld.erase(clump);
			}
		}
	}
	__imp__sub_17C130(ctx, base);
}

// Fade end
X86_FUNC_IMPL(__imp__sub_17C1C0);
X86_FUNC(sub_17C1C0)
{
	const uint32_t gadget = ctx.ecx.u32;
	if (X86_LOAD_U32(gadget + 0x3C) & 0x40)
	{
		const uint32_t listEnd = X86_LOAD_U32(gadget + 0x50);
		for (uint32_t node = X86_LOAD_U32(listEnd); node != listEnd; node = X86_LOAD_U32(node))
		{
			const uint32_t clump = X86_LOAD_U32(node + 0x8);
			g_fadeOriginalWorld.erase(clump);
		}
	}
	__imp__sub_17C1C0(ctx, base);
}

// Gadget atomic render
X86_FUNC_IMPL(__imp__sub_17A5C0);
X86_FUNC(sub_17A5C0)
{
	const uint32_t atomic = X86_LOAD_U32(ctx.esp.u32 + 0x4);
	const uint32_t clump = X86_LOAD_U32(atomic + 0x3C);
	const auto originalWorld = g_fadeOriginalWorld.find(clump);
	if (originalWorld == g_fadeOriginalWorld.end() || GetClumpWorld(base, clump) != X86_LOAD_U32(0x4E51E8))
	{
		__imp__sub_17A5C0(ctx, base);
		return;
	}
	// restore the original mode
	const uint32_t curWorld = X86_LOAD_U32(0x57DE04);
	X86_STORE_U32(0x57DE04, originalWorld->second);
	__imp__sub_17A5C0(ctx, base);
	X86_STORE_U32(0x57DE04, curWorld);
}

// Shared AnimTask func (used by a lot of objects)
X86_FUNC_IMPL(__imp__sub_17E290);
X86_FUNC(sub_17E290)
{
	// deltatime patch (used by a lot of objects for fade and anim transitions)
	const float dt = X86_LOAD_F32(ctx.esp.u32 + 4);
	float frames = dt * 60.0f;
	if (!(frames > 0.0f))
	{
		frames = 1.0f;
	}
	frames = std::min(frames, 1.0f);
	const uint32_t rateAddr = ctx.ecx.u32 + 0x48;
	const float restoreRate = X86_LOAD_F32(rateAddr);
	if (std::fabs(frames - 1.0f) >= 0.0001f)
	{
		X86_STORE_F32(rateAddr, restoreRate * frames);
	}

	__imp__sub_17E290(ctx, base);

	// deltatime patch
	X86_STORE_F32(rateAddr, restoreRate);
}

// Finds BIGFOOTCANOPY.DFF and change ATTR_WORLDPARM from 0x7 to 0x8 to fix broken transparency
void BigfootCanopyOnClumpRead(X86Context* ctx, uint8_t* base)
{
	auto pOneFileAsync = (TOneFileAsync*)g_memory.Translate(ctx->ecx.u32);
	if (pOneFileAsync)
	{
		auto pOneFile = pOneFileAsync->pOneFile.get();
		if (pOneFile)
		{
			const auto assetName = TOneFileCheckFileName(pOneFile, pOneFileAsync->fid);
			const uint32_t userDataOffset = X86_LOAD_U32(0x55FE40); // User Data PLG
			if (assetName && pOneFileAsync->clump && userDataOffset &&
				strcmp(assetName, "BIGFOOTCANOPY.DFF") == 0)
			{
				const uint32_t listEnd = pOneFileAsync->clump + 8;
				uint32_t link = X86_LOAD_U32(listEnd);
				for (int safety = 0; link && link != listEnd && safety < 64; safety++)
				{
					// iterate through all PLG and patch to 0x8
					const uint32_t geometry = X86_LOAD_U32(link - 0x40 + 0x18);
					const auto numEntries = (int32_t)X86_LOAD_U32(geometry + userDataOffset);
					const uint32_t entries = X86_LOAD_U32(geometry + userDataOffset + 4);
					for (int32_t i = 0; i < numEntries && entries; i++)
					{
						const uint32_t entry = entries + (uint32_t)i * 0x10;
						const uint32_t name = X86_LOAD_U32(entry);
						const uint32_t values = X86_LOAD_U32(entry + 0xC);

						if (name && values && X86_LOAD_U32(values) == 0x7 && strcmp((const char*)g_memory.Translate(name), "ATTR_WORLDPARM.ATTR_WORLDPARM") == 0)
						{
							X86_STORE_U32(values, 0x8);
						}
					}
					link = X86_LOAD_U32(link);
				}
			}
		}
	}
}

// Finds CHAOSEYE.DFF to fix the 90 degree bad rotation
void ChaosEyeRotationOnClumpRead(X86Context* ctx, uint8_t* base)
{
	auto pOneFileAsync = (TOneFileAsync*)g_memory.Translate(ctx->ecx.u32);
	if (pOneFileAsync)
	{
		auto pOneFile = pOneFileAsync->pOneFile.get();
		if (pOneFile)
		{
			const auto assetName = TOneFileCheckFileName(pOneFile, pOneFileAsync->fid);
			if (assetName && pOneFileAsync->clump && strcmp(assetName, "CHAOSEYE.DFF") == 0)
			{
				const uint32_t listEnd = pOneFileAsync->clump + 8;
				const uint32_t link = X86_LOAD_U32(listEnd);
				const uint32_t frame = link && link != listEnd ? X86_LOAD_U32(link - 0x40 + 0x4) : 0;
				const uint32_t root = frame ? X86_LOAD_U32(frame + 0xA0) : 0;

				// x is -1.0f for root
				if (root && X86_LOAD_U32(root + 0x4) == 0 && X86_LOAD_F32(root + 0x30) == -1.0f)
				{
					// Grouped
					X86_STORE_F32(root + 0x10, 1.0f);
					X86_STORE_F32(root + 0x14, 0.0f);
					X86_STORE_F32(root + 0x18, 0.0f);
					X86_STORE_F32(root + 0x20, 0.0f);

					X86_STORE_F32(root + 0x24, 1.0f);
					X86_STORE_F32(root + 0x28, 0.0f);
					X86_STORE_F32(root + 0x30, 0.0f);
					X86_STORE_F32(root + 0x34, 0.0f);

					X86_STORE_F32(root + 0x38, 1.0f);
					X86_STORE_F32(root + 0x40, 0.0f);
					X86_STORE_F32(root + 0x44, 0.0f);
					X86_STORE_F32(root + 0x48, 0.0f);

					for (uint32_t i = 0; i < 16; i++)
					{
						X86_STORE_U32(root + 0x50 + i * 4, X86_LOAD_U32(root + 0x10 + i * 4));
					}

					// Call RwFrameUpdateObject after
					const auto eax = ctx->eax;
					const auto ecx = ctx->ecx;
					const auto edx = ctx->edx;
					ctx->esp.u32 -= 4;
					X86_STORE_U32(ctx->esp.u32, root);
					ctx->esp.u32 -= 4;
					sub_2E1300(*ctx, base);
					ctx->esp.u32 += 4;
					ctx->eax = eax;
					ctx->ecx = ecx;
					ctx->edx = edx;
				}
			}
		}
	}
}

// BombingWall filter patch for CB
void BombingWallBreakMidAsmHook(X86Register& ecx)
{
	if (ecx.u32 & 0x10)
	{
		ecx.u32 |= 0x200;
	}
}

// Switch attacker filter callback (Ball switch)
// Extend to include melee swing (0x80) + melee generic (0x2)
bool BallSwitchTriggerMidAsmHook(X86Register& eax)
{
	auto base = g_memory.base;
	return eax.u32 != 0 && (X86_LOAD_U32(eax.u32 + 0x40) & 0x82) != 0;
}

// BKTank only responds to damage once in StopAction
// Ignore it for StopAction (passing to damage callback)
bool BKTankStopActionDamageFixMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	return X86_LOAD_F32(esi.u32 + 0x28) > 0.0f;
}
