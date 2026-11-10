#pragma once

#include <kernel/memory.h>
#include <input/padstate.h>

// helper to match rotation turn
inline float WrapAngle(float angle)
{
	if (angle > 3.14159265f)
	{
		angle -= 6.28318531f;
	}
	else if (angle < -3.14159265f)
	{
		angle += 6.28318531f;
	}
	return angle;
}

// PAD manager slot
inline PJSPeripheral::ControllerSlot* GetPadSlot(uint32_t port)
{
	return reinterpret_cast<PJSPeripheral::ControllerSlot*>(MmGetHostAddress(0x535550 + 0x50 + port * 0x28));
}

inline uint32_t GetPlayerPort(uint32_t player)
{
	auto base = g_memory.base;
	const uint32_t input = player ? X86_LOAD_U32(player + 0x218) : 0;
	if (input == 0)
	{
		return UINT32_MAX; // AI detect fallback (NpcSonic)
	}
	const uint32_t pad = X86_LOAD_U32(input + 0x10);
	const uint32_t port = (pad - 0x541A68) / 0x2C;
	return pad >= 0x541A68 && (pad - 0x541A68) % 0x2C == 0 && port < 4 ? port : X86_LOAD_U32(input + 0x14);
}
