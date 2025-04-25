#pragma once

static uint32_t RoundUp(uint32_t dwValue, uint32_t dwMult)
{
	if (dwMult == 0) { return dwValue; }

	int remainder = dwValue % dwMult;
	if (remainder == 0)
		return dwValue;

	return dwValue + dwMult - remainder;
}

template<typename TValue>
constexpr size_t FirstBitLow(TValue value)
{
    constexpr size_t nbits = sizeof(TValue) * 8;
    constexpr auto zero = TValue{};
    constexpr auto one = static_cast<TValue>(1);

    for (size_t i = 0; i < nbits; i++)
    {
        if ((value & (one << i)) != zero)
        {
            return i;
        }
    }

    return 0;
}