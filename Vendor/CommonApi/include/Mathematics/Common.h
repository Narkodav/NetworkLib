#pragma once
#include "../Namespaces.h"
#include <algorithm>

namespace Mathematics
{
	const float pi = 3.141592653589793238462643383279502884197;

	inline int GaussCircleApproximation(int radius)
	{
		return pi * radius * radius + radius * sqrt(2 * pi) + 1 / 3;
	}

	inline bool IsInCircle(int x, int y, int radius)
	{
		return x * x + y * y <= radius * radius;
	}

	inline float lerp(float a, float b, float alpha)
	{
		return a + alpha * (b - a);
	}

	inline float fade(float alpha)
	{
		return ((6 * alpha - 15) * alpha + 10) * alpha * alpha * alpha;
	}

	

	

};

