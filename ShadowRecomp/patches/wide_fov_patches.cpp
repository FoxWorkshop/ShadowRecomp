#include <kernel/function.h>
#include <user/config.h>

// TODO: Replace with the better FOV patch we do in GC
// Widescreen FOV Hack (Simple)
static constexpr uint32_t globalFOV = 0x4E5D6C;

void WideFOVConfigValueChangedCallback(const IConfigDef* config)
{
	if (config == &Config::WideFOV)
	{
		const auto base = g_memory.base;

		if (Config::WideFOV.Value)
		{
			X86_STORE_F32(globalFOV, 1.33333337306976f);
		}
		else
		{
			X86_STORE_F32(globalFOV, 1.0f);
		}
	}
}
