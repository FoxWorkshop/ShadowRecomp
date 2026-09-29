#pragma once

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
