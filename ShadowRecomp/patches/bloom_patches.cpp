#include <kernel/function.h>
#include <user/config.h>

static const uint32_t bloomAddr = 0x52E5C4;

// change bloom intensity based on user's config.

void BloomConfigValueChangedCallback(IConfigDef* config)
{
	if (config == &Config::BloomIntensity)
	{
		float rounded = std::roundf(Config::BloomIntensity.Value * 100.0f) / 100.0f;

		Config::BloomIntensity.Value = rounded;

		if (Config::BloomIntensity.Value < 0.01f)
			Config::BloomIntensity.Value = 0.0f;

		if (Config::BloomIntensity.Value > 1.f)
			Config::BloomIntensity.Value = 1.0f;

		auto base = g_memory.base;
		X86_STORE_F32(bloomAddr, Config::BloomIntensity.Value);
	}
}
