#include <gpu/video.h>
#include <kernel/function.h>
#include <patches/global.h>

// --- Performance patches. ---
// Perf pointed us here: Ate roughly ~7% of game-thread in areas
// that made 2P lag. Reimplemented by hand.

// sub_30ECB0(ecx=v*): distance squared. i.e. squared Euclidean length of v, 
// i.e. (|v|^2 = x^2 + y^2 + z^2).
X86_FUNC_IMPL(__imp__sub_30ECB0);
X86_FUNC(sub_30ECB0)
{
	const float x = X86_LOAD_F32(ctx.ecx.u32);
	const float y = X86_LOAD_F32(ctx.ecx.u32 + 0x4);
	const float z = X86_LOAD_F32(ctx.ecx.u32 + 0x8);
	// Speed hack:
	// f64 is enough: f32 squares are exact in it, and rounding to full
	// f32 was what tanked the framerate before (313 -> 298 fps).
	const double xx = (double)x * x;
	const double yy = (double)y * y;
	const double zz = (double)z * z;
	ctx.fpu.push((x87_float)(xx + yy + zz));
	ctx.esp.u32 += 0x4;
}
