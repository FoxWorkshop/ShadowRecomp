#include <algorithm>
#include <stdexcept>

static uint32_t RoundUp(uint32_t dwValue, uint32_t dwMult)
{
	if (dwMult == 0) { return dwValue; }

	int remainder = dwValue % dwMult;
	if (remainder == 0)
		return dwValue;

	return dwValue + dwMult - remainder;
}