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
// with a fade value that gets added to skinned_pos.w in the vertex shader, causing bugged objects to flash bright during fade-in/fade-out
// Only stub the fade alpha write when called from the object fade-in/fade-out
static thread_local bool g_inSub17E290 = false;

X86_FUNC_IMPL(__imp__sub_17C130);
X86_FUNC(sub_17C130)
{
	if (g_inSub17E290)
	{
		ctx.esp.u32 += 0x8;
		return;
	}
	__imp__sub_17C130(ctx, base);
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

	// Mark when the caller is sub_17E290 (for sub_17C130, to block the crappy DS1 fade bug Xbox has)
	g_inSub17E290 = true;
	__imp__sub_17E290(ctx, base);
	g_inSub17E290 = false;

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
