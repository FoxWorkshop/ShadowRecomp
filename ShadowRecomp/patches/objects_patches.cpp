#include <kernel/function.h>
#include <user/config.h>

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

// Reads ATTR_WORLDPARM UserData, RWAnalyze Chunk 0x11F, in DFF and LAND (Geometry) and maps to the correct render type (at 0x503620)
// 0x7 -> maps to 9 "AL" (This is the bad/broken transparency mode on Xbox for BigFoot's Canopy)
// 0x8 -> maps to 11 "M"
// To fix the BigFoot, map all 0x7 to 0x8 to every DFF in the whole game
X86_FUNC_IMPL(__imp__sub_181CF0);
X86_FUNC(sub_181CF0)
{
	if (X86_LOAD_U32(ctx.esp.u32 + 4) == 0x7)
	{
		X86_STORE_U32(ctx.esp.u32 + 4, 0x8);
	}
	__imp__sub_181CF0(ctx, base);
}
