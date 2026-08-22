#include <kernel/function.h>

// sub_84E50 ground-stick tolerance for collision mesh seams/vertices
// Float imprecision at shared triangle edges/vertices causes the vert bumps
// Westopolis light pole before the goal ring is a good example of this, visible in HPP
static uint32_t g_rayHitPlayer = 0;
void CollisionToleranceMidAsmHook(X86Register& esi, X86Register& esp)
{
	auto base = g_memory.base;
	g_rayHitPlayer = 0;

	uint32_t player = X86_LOAD_U32(esi.u32);

	// Skip if not already grounded
	if ((X86_LOAD_U32(player + 0xa8) & 0x4) == 0)
		return;

	float radius = X86_LOAD_F32(player + 0x274);
	if (radius <= 0.0f)
		return;

	// Stored ground normal (0x1C8 - 0x1D0)
	float nx = X86_LOAD_F32(player + 0x1c8);
	float ny = X86_LOAD_F32(player + 0x1cc);
	float nz = X86_LOAD_F32(player + 0x1d0);
	if (nx * nx + ny * ny + nz * nz < 0.5f)
		return;

	float dist = X86_LOAD_F32(esp.u32 + 0x14); // get raycast result (0 = failed)

	if (dist >= 0.0f && dist <= radius * 2.37f)
	{
		g_rayHitPlayer = player;
	}

	if (dist < 0.0f)
	{
		// When ray miss, recheck with bigger radius
		float d = radius * 2.0f;
		float hx = X86_LOAD_F32(esp.u32 + 0x18) - nx * d;
		float hy = X86_LOAD_F32(esp.u32 + 0x1c) - ny * d;
		float hz = X86_LOAD_F32(esp.u32 + 0x20) - nz * d;
		X86_STORE_F32(esp.u32 + 0x8c, hx);
		X86_STORE_F32(esp.u32 + 0x90, hy);
		X86_STORE_F32(esp.u32 + 0x94, hz);
		X86_STORE_F32(esp.u32 + 0xa4, hx);
		X86_STORE_F32(esp.u32 + 0xa8, hy);
		X86_STORE_F32(esp.u32 + 0xac, hz);
		X86_STORE_F32(esp.u32 + 0x14, d);
	}
	else if (dist > radius * 2.0f && dist <= radius * 2.37f)
	{
		X86_STORE_F32(esp.u32 + 0x14, radius * 2.0f);
	}
}

// Fallback for zero movement instead of a state reset (should be rare)
bool CollisionToleranceSnapFallbackMidAsmHook(X86Register& esi, X86Register& eax, X86Register& esp)
{
	auto base = g_memory.base;
	uint32_t player = X86_LOAD_U32(esi.u32);

	if (g_rayHitPlayer != player)
		return false;

	if (eax.u8_lo != 0)
		return false;

	if ((X86_LOAD_U32(player + 0xa8) & 0x4) == 0)
		return false; // skip if not grounded

	g_rayHitPlayer = 0;

	// 0x856B1 (write ignoring state for coli and player)
	X86_STORE_F32(esp.u32 + 0x68, X86_LOAD_F32(esp.u32 + 0x18));
	X86_STORE_F32(esp.u32 + 0x6c, X86_LOAD_F32(esp.u32 + 0x1c));
	X86_STORE_F32(esp.u32 + 0x70, X86_LOAD_F32(esp.u32 + 0x20));
	X86_STORE_F32(esp.u32 + 0x4c, X86_LOAD_F32(player + 0x1c8));
	X86_STORE_F32(esp.u32 + 0x50, X86_LOAD_F32(player + 0x1cc));
	X86_STORE_F32(esp.u32 + 0x54, X86_LOAD_F32(player + 0x1d0));

	return true;
}

// 0x84E50 slope re-check @ 0x856AF
// Rely on 0x7FB30 to correct rotation
bool CollisionToleranceSlopeGateMidAsmHook(X86Register& esi)
{
	auto base = g_memory.base;
	uint32_t player = X86_LOAD_U32(esi.u32);
	if (g_rayHitPlayer != player)
		return false;

	if ((X86_LOAD_U32(player + 0xa8) & 0x4) == 0)
		return false;

	g_rayHitPlayer = 0;
	return true;
}
