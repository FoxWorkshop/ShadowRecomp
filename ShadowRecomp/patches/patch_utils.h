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
