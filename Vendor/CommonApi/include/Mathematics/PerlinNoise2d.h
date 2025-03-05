#pragma once
#include "../Namespaces.h"
#include "Common.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <random>
#include <array>

namespace Mathematics
{
	class PerlinNoise2d
	{
	private:
		static const int gradientTableSize = 256;
		std::array<glm::vec2, gradientTableSize> gradientTable;
		unsigned int seed = 0;
		std::mt19937 rng;

		void generateGradientTable()
		{
			float fraction = 2 * pi / gradientTableSize;
			float angle = 0.0f;
			for (int i = 0; i < gradientTableSize; i++)
			{
				gradientTable[i] = glm::vec2(cos(angle), sin(angle));
				angle += fraction;
			}
			std::shuffle(gradientTable.begin(), gradientTable.end(), rng);
		}

		inline int hash(int x, int y) const {
			int h = x * 1619 + y * 31337 + seed;
			h = (h << 13) ^ h;
			return h * (h * h * 60493 + 19990303) + 1376312589;
		}

	public:

		void setSeed(unsigned int newSeed)
		{
			seed = newSeed;
			rng.seed(seed);
			generateGradientTable();
		}

		inline glm::vec2 getGradient(int x, int y) const
		{
			return gradientTable[hash(x, y) & (gradientTableSize - 1)];
		}

		inline float getDot(glm::vec2 v, int X, int Y) const
		{
			return glm::dot(getGradient(X, Y), v);
		}

		float getOctave(float x, float y) const
		{
			int xCellCoord = (int)x;
			int yCellCoord = (int)y;
			if (x < 0)
				xCellCoord -= 1;
			if (y < 0)
				yCellCoord -= 1;

			float X = x - xCellCoord;
			float Y = y - yCellCoord;

			float dotTopRight = getDot(glm::vec2(1 - X, 1 - Y), xCellCoord + 1, yCellCoord + 1);
			float dotTopLeft = getDot(glm::vec2(X, 1 - Y), xCellCoord, yCellCoord + 1);
			float dotBottomRight = getDot(glm::vec2(1 - X, Y), xCellCoord + 1, yCellCoord);
			float dotBottomLeft = getDot(glm::vec2(X, Y), xCellCoord, yCellCoord);

			float u = fade(X);
			float v = fade(Y);

			return lerp(lerp(dotBottomLeft, dotBottomRight, u), lerp(dotTopLeft, dotTopRight, u), v);
		}

		float getFbm(float x, float y, int octaves, float frequency = 1.0f, float amplitude = 1.0f, float persistence = 0.5f, float lacunarity = 2.0f) const
		{
			float total = 0.0f;
			float maxValue = 0.0f;  // Used for normalizing the result

			for (int i = 0; i < octaves; i++)
			{
				// Get noise value at current frequency
				total += getOctave(x * frequency, y * frequency) * amplitude;

				maxValue += amplitude;
				amplitude *= persistence;  // Amplitude decreases with each octave
				frequency *= lacunarity;  // Frequency increases with each octave
			}

			// Normalize the result to keep it roughly in [-1, 1] range
			return total / maxValue;
		}

	};
}
