#include <kernel/function.h>
#include <user/config.h>
#include <gpu/video.h>

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

// bloom seems to create some kind of small artifact effect at the bottom of the screen at higher resolution
// rounding the bloom vertex
void DrawBloomMidAsmHook(X86Register& rEcx)
{
	auto addrVertex = rEcx.u32 + 72;

	if (addrVertex)
	{
		auto vertex = (UpVertex*)(g_memory.Translate(addrVertex));

		for (int i = 0; i < 4; i++)
		{
			vertex[i].x = roundf(vertex[i].x);
			vertex[i].y = roundf(vertex[i].y);
			vertex[i].z = roundf(vertex[i].z);
		}
	}
}