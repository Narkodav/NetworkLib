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
	class PerlinNoise3d
	{
	private:
		static const int gridCellSize = 64;
		static const int gradientTableSize = 256;
		std::array<glm::vec3, gradientTableSize> gradientTable;
		static const unsigned int permutationTableSize = 256;
		std::array<unsigned int, permutationTableSize * 2> permutationTable;
		unsigned int seed = 0;
		std::mt19937 rng;

		void generateGradientTable() //allocates on equal intervals on a sphere
		{
			int root = sqrt(gradientTableSize);
			float azimuthalFraction = 2 * pi / root;
			float polarFraction = 2 * pi / root;
			float azimuthalAngle = 0.0f;
			float polarAngle = 0.0f;
			for (int i = 0; i < root; i++)
			{
				azimuthalAngle = 0.0f;
				for (int j = 0; j < root; j++)
				{
					gradientTable[i * root + j] = glm::vec3(glm::cos(azimuthalAngle) * glm::sin(polarAngle),
						glm::sin(azimuthalAngle) * glm::sin(polarAngle),
						glm::cos(polarAngle));
					azimuthalAngle += azimuthalFraction;
				}
				polarAngle += azimuthalFraction;
			}
			std::shuffle(gradientTable.begin(), gradientTable.end(), rng);
		}

		void generatePermutationTableTable() //allocates on equal intervals on a sphere
		{
			for (int i = 0; i < permutationTableSize; i++)
				permutationTable[i] = i;

			std::shuffle(permutationTable.begin(), permutationTable.begin() + permutationTableSize, rng);

			for (unsigned int i = 0; i < permutationTableSize; i++)
				permutationTable[permutationTableSize + i] = permutationTable[i];
		}

		inline int hash(int x, int y, int z) const {

			return permutationTable[permutationTable[permutationTable[x & 255] + (y & 255)] + (z & 255)];
		}

	public:

		void setSeed(unsigned int newSeed)
		{
			seed = newSeed;
			rng.seed(seed);
			generateGradientTable();
			generatePermutationTableTable();
		}

		inline glm::vec3 getGradient(int x, int y, int z) const
		{
			return gradientTable[hash(x, y, z)];
		}

		inline float getDot(glm::vec3 v, int X, int Y, int Z) const
		{
			return glm::dot(getGradient(X, Y, Z), v);
		}

		float getOctave(float x, float y, float z) const
		{
			int xCellCoord = x;
			int yCellCoord = y;
			int zCellCoord = z;
			if (x < 0)
				xCellCoord -= 1;
			if (y < 0)
				yCellCoord -= 1;
			if (z < 0)
				zCellCoord -= 1;

			float X = x - xCellCoord;
			float Y = y - yCellCoord;
			float Z = z - zCellCoord;

			float dotTopRightFront = getDot(glm::vec3(1 - X, 1 - Y, 1 - Z), xCellCoord + 1, yCellCoord + 1, zCellCoord + 1);
			float dotTopRightBack = getDot(glm::vec3(1 - X, 1 - Y, Z), xCellCoord + 1, yCellCoord + 1, zCellCoord);

			float dotTopLeftFront = getDot(glm::vec3(X, 1 - Y, 1 - Z), xCellCoord, yCellCoord + 1, zCellCoord + 1);
			float dotTopLeftBack = getDot(glm::vec3(X, 1 - Y, Z), xCellCoord, yCellCoord + 1, zCellCoord);

			float dotBottomRightFront = getDot(glm::vec3(1 - X, Y, 1 - Z), xCellCoord + 1, yCellCoord, zCellCoord + 1);
			float dotBottomRightBack = getDot(glm::vec3(1 - X, Y, Z), xCellCoord + 1, yCellCoord, zCellCoord);

			float dotBottomLeftFront = getDot(glm::vec3(X, Y, 1 - Z), xCellCoord, yCellCoord, zCellCoord + 1);
			float dotBottomLeftBack = getDot(glm::vec3(X, Y, Z), xCellCoord, yCellCoord, zCellCoord);

			float u = fade(X);
			float v = fade(Y);
			float w = fade(Z);

			float lerpFront = lerp(lerp(dotBottomLeftFront, dotBottomRightFront, u), lerp(dotTopLeftFront, dotTopRightFront, u), v);
			float lerpBack = lerp(lerp(dotBottomLeftBack, dotBottomRightBack, u), lerp(dotTopLeftBack, dotTopRightBack, u), v);

			return lerp(lerpBack, lerpFront, w);
		}

		float getFbm(float x, float y, float z, int octaves, float frequency = 1.0f, float amplitude = 1.0f, float persistence = 0.5f, float lacunarity = 2.0f) const
		{
			float total = 0.0f;
			float maxValue = 0.0f;  // Used for normalizing the result

			for (int i = 0; i < octaves; i++)
			{
				// Get noise value at current frequency
				total += getOctave(x * frequency, y * frequency, z * frequency) * amplitude;

				maxValue += amplitude;
				amplitude *= persistence;  // Amplitude decreases with each octave
				frequency *= lacunarity;  // Frequency increases with each octave
			}
			if (abs(total) > 1)
				__debugbreak();
			// Normalize the result to keep it roughly in [-1, 1] range
			return total / maxValue;
		}

	};
}