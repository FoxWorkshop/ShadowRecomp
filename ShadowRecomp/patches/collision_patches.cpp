#include <kernel/function.h>
#include <app.h>

static const uint32_t g_groundSlopeCos = 0x448F00;   // cos80 originally
static const uint32_t g_groundReachScale = 0x448F68; // 2.37f originally
static const uint32_t g_groundStickScale = 0x429F74; // 2.0f originally
static constexpr float g_seamRecastOffset = 0.05f;

static uint32_t g_rayFloorPlayer = 0;
static float g_rayFloorNormal[3];

// 0x84FEA runs once per pass on both branches
void CollisionToleranceResetMidAsmHook()
{
	g_rayFloorPlayer = 0;
}

// ground ray at 0x85338 has no normal output, using it for side coli ray slot (esp+0x68)
void CollisionGroundRayNormalMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	X86_STORE_U32(esp.u32 + 0x8, esp.u32 + 0x44 + 0x68);
}

static float Length(const float v[3])
{
	return std::sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

static float Normalize(float v[3])
{
	float len = Length(v);
	if (len > 0.0f)
	{
		for (int i = 0; i < 3; i++)
		{
			v[i] /= len;
		}
	}
	return len;
}

static void Cross(const float a[3], const float b[3], float out[3])
{
	out[0] = a[1] * b[2] - a[2] * b[1];
	out[1] = a[2] * b[0] - a[0] * b[2];
	out[2] = a[0] * b[1] - a[1] * b[0];
}

static bool IsFloorNormal(const float normal[3], const float ground[3])
{
	auto base = g_memory.base;
	float len = Length(normal);
	if (len < 0.5f)
	{
		return false;
	}
	return (normal[0] * ground[0] + normal[1] * ground[1] + normal[2] * ground[2]) / len >= X86_LOAD_F32(g_groundSlopeCos);
}

// same call as 0x85338, shifted by offset
static float RecastGroundRay(uint32_t playerSlot, uint32_t filter, uint32_t stackTop, const float ray[6], const float offset[3], float hit[3], float normal[3])
{
	auto base = g_memory.base;
	X86Context call = *GetX86Context();

	// ray +0, hit +0x18, normal +0x24 (below the live stack)
	uint32_t scratch = (stackTop - 0x40) & ~0xFu;
	for (int i = 0; i < 6; i++)
	{
		X86_STORE_F32(scratch + i * 4, ray[i] + offset[i % 3]);
	}

	call.esp.u32 = scratch;
	X86_PUSH(call, base, 2);
	X86_PUSH(call, base, 0x40000);
	X86_PUSH(call, base, filter);
	X86_PUSH(call, base, 0);
	X86_PUSH(call, base, scratch);
	X86_PUSH(call, base, 0);
	X86_PUSH(call, base, 0);
	X86_PUSH(call, base, 0);
	X86_PUSH(call, base, scratch + 0x24);
	X86_PUSH(call, base, scratch + 0x18);
	X86_PUSH(call, base, playerSlot);
	call.SubEsp(1);
	sub_82AD0(call, base);

	float dist = (float)call.fpu.st0(); // peak at float return (hack)
	for (int i = 0; i < 3; i++)
	{
		hit[i] = X86_LOAD_F32(scratch + 0x18 + i * 4) - offset[i];
		normal[i] = X86_LOAD_F32(scratch + 0x24 + i * 4);
	}
	return dist;
}

// sub_84E50 ground-stick tolerance for collision mesh seams/vertices
// Float imprecision at shared triangle edges/vertices causes the vert bumps
// Westopolis light pole before the goal ring is a good example of this, visible in HPP
// only helps over real floor, uses the ground normal so any gravity works
// after the ground ray, esp+0x14 is the distance (-1 = miss)
void CollisionToleranceMidAsmHook(X86Register& esi, X86Register& ebp, X86Register& esp)
{
	auto base = g_memory.base;
	g_rayFloorPlayer = 0;

	uint32_t player = X86_LOAD_U32(esi.u32);

	// skip if not already grounded
	if ((X86_LOAD_U32(player + 0xa8) & 0x4) == 0)
	{
		return;
	}

	float radius = X86_LOAD_F32(player + 0x274);
	if (radius <= 0.0f)
	{
		return;
	}

	// stored ground normal (0x1C8 - 0x1D0)
	float ground[3] = { X86_LOAD_F32(player + 0x1c8), X86_LOAD_F32(player + 0x1cc), X86_LOAD_F32(player + 0x1d0) };
	if (Normalize(ground) < 0.7f)
	{
		return;
	}

	float reach = radius * X86_LOAD_F32(g_groundReachScale);
	float stick = radius * X86_LOAD_F32(g_groundStickScale);
	float dist = X86_LOAD_F32(esp.u32 + 0x14);
	float rayNormal[3];

	if (dist < 0.0f)
	{
		// ray start/end
		float ray[6];
		for (int i = 0; i < 6; i++)
		{
			ray[i] = X86_LOAD_F32(esp.u32 + 0x74 + i * 4);
		}

		// two tangents of the ground plane
		float axis[3] = { 0.0f, 1.0f, 0.0f };
		if (std::fabs(ground[1]) > 0.9f)
		{
			axis[0] = 1.0f;
			axis[1] = 0.0f;
		}
		float side1[3];
		Cross(ground, axis, side1);
		Normalize(side1);
		float side2[3];
		Cross(ground, side1, side2);

		// recast to both sides on a miss
		const float* sides[4] = { side1, side2, side1, side2 };
		for (int s = 0; s < 4 && dist < 0.0f; s++)
		{
			float shift = s < 2 ? g_seamRecastOffset : -g_seamRecastOffset;
			float offset[3] = { sides[s][0] * shift, sides[s][1] * shift, sides[s][2] * shift };
			float hit[3];

			float recastDist = RecastGroundRay(esi.u32, ebp.u32, esp.u32, ray, offset, hit, rayNormal);
			if (recastDist < 0.0f || recastDist > reach || !IsFloorNormal(rayNormal, ground))
			{
				continue;
			}

			dist = recastDist;
			X86_STORE_F32(esp.u32 + 0x8c, hit[0]);
			X86_STORE_F32(esp.u32 + 0x90, hit[1]);
			X86_STORE_F32(esp.u32 + 0x94, hit[2]);
			X86_STORE_F32(esp.u32 + 0x14, dist);
		}

		// real ledge
		if (dist < 0.0f)
		{
			return;
		}
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			rayNormal[i] = X86_LOAD_F32(esp.u32 + 0x68 + i * 4);
		}

		// fallback to original behavior if our checks fail (walls)
		if (dist > reach || !IsFloorNormal(rayNormal, ground))
		{
			return;
		}
	}

	Normalize(rayNormal);
	for (int i = 0; i < 3; i++)
	{
		g_rayFloorNormal[i] = rayNormal[i];
	}

	g_rayFloorPlayer = player;

	// snap instead of the second ray
	if (dist > stick)
	{
		X86_STORE_F32(esp.u32 + 0x14, stick);
	}
}

static constexpr double g_snapFallbackTime = 0.75 / 60.0; // 1 frame at 60fps
static constexpr double g_snapStreakTime = 1.5 / 60.0;

struct SnapFallbackState
{
	double streakStart = 0.0;
	double missStart = 0.0;
	bool missing = false;
	bool covered = false;
};

static std::unordered_map<uint32_t, SnapFallbackState> g_snapFallback;

// ray found floor but the sweep missed, stay grounded (with timer check to prevent wall exploits and fix ottotto)
bool CollisionToleranceSnapFallbackMidAsmHook(X86Register& esi, X86Register& eax, X86Register& esp)
{
	auto base = g_memory.base;
	uint32_t player = X86_LOAD_U32(esi.u32);
	auto& state = g_snapFallback[player];

	// sweep hit
	if (eax.u8_lo != 0)
	{
		if (state.missing)
		{
			state.missing = false;
			state.streakStart = App::s_time;
		}
		return false;
	}

	if (g_rayFloorPlayer != player)
	{
		return false;
	}

	if (!state.missing)
	{
		state.missing = true;
		state.missStart = App::s_time;
		state.covered = App::s_time - state.streakStart >= g_snapStreakTime;
	}

	// first frame is always covered (mainly for below 60fps)
	if (!state.covered || App::s_time - state.missStart >= g_snapFallbackTime)
	{
		return false;
	}

	// 0x856B1 (write ignoring state for coli and player)
	X86_STORE_F32(esp.u32 + 0x68, X86_LOAD_F32(esp.u32 + 0x18));
	X86_STORE_F32(esp.u32 + 0x6c, X86_LOAD_F32(esp.u32 + 0x1c));
	X86_STORE_F32(esp.u32 + 0x70, X86_LOAD_F32(esp.u32 + 0x20));
	X86_STORE_F32(esp.u32 + 0x4c, g_rayFloorNormal[0]);
	X86_STORE_F32(esp.u32 + 0x50, g_rayFloorNormal[1]);
	X86_STORE_F32(esp.u32 + 0x54, g_rayFloorNormal[2]);
	return true;
}

// 0x84E50 slope re-check @ 0x856AF
// sweep sometimes return an edge normal at a seam (Westopolis)
// now uses the ray floor normal as a fallback (shifted)
bool CollisionToleranceSlopeGateMidAsmHook(X86Register& esi, X86Register& esp)
{
	auto base = g_memory.base;
	uint32_t player = X86_LOAD_U32(esi.u32);
	if (g_rayFloorPlayer != player)
	{
		return false;
	}

	g_rayFloorPlayer = 0;
	float ground[3] = { X86_LOAD_F32(player + 0x1c8), X86_LOAD_F32(player + 0x1cc), X86_LOAD_F32(player + 0x1d0) };
	float sweep[3] = { X86_LOAD_F32(esp.u32 + 0x4c), X86_LOAD_F32(esp.u32 + 0x50), X86_LOAD_F32(esp.u32 + 0x54) };
	if (IsFloorNormal(sweep, ground))
	{
		return false;
	}

	X86_STORE_F32(esp.u32 + 0x4c, g_rayFloorNormal[0]);
	X86_STORE_F32(esp.u32 + 0x50, g_rayFloorNormal[1]);
	X86_STORE_F32(esp.u32 + 0x54, g_rayFloorNormal[2]);
	return true;
}

// Patch Coaster's bar collision to allow bullets
void CoasterHangbarCollisionMidAsmHook(X86Register& esp)
{
	auto base = g_memory.base;
	X86_STORE_U32(esp.u32, 0x40000);
}
