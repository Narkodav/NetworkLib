#pragma once
#include "../Namespaces.h"

#include <vector>
#include <algorithm>

namespace Utilities
{
	class BinPacker
	{
	public:
		struct Box
		{
			unsigned int id;

			unsigned int width = 0;
			unsigned int height = 0;

			unsigned int TopLeftX = 0;
			unsigned int TopLeftY = 0;
		};

		struct Bin
		{
			std::vector<Box> boxes;
			unsigned int width = 0;
			unsigned int height = 0;
		};

	private:
		static bool collisionCheck(const Box& box, const Bin& bin, unsigned int x, unsigned int y)
		{
			if (x + box.width > bin.width || y + box.height > bin.height)
				return 1;

			// Check for overlap with existing boxes
			for (const auto& placedBox : bin.boxes) {
				bool overlapX = (x < placedBox.TopLeftX + placedBox.width) &&
					(x + box.width > placedBox.TopLeftX);
				bool overlapY = (y < placedBox.TopLeftY + placedBox.height) &&
					(y + box.height > placedBox.TopLeftY);

				if (overlapX && overlapY)
					return 1;
			}
			return 0;
		}

		static bool firstFitIterate(Box& box, Bin& bin)
		{
			for (unsigned int x = 0; x < bin.width; x++)
				for (unsigned int y = 0; y < bin.height; y++)
					if (!collisionCheck(box, bin, x, y)) {
						// Place the box at this position
						box.TopLeftX = x;
						box.TopLeftY = y;
						bin.boxes.push_back(box);
						return 1;
					}
			return 0;
		}

	public:
		static Bin firstFit(std::vector<Box> boxes)
		{
			std::sort(boxes.begin(), boxes.end(), [](const Box& a, const Box& b) {
				return (a.width * a.height) > (b.width * b.height); // Sort by area, largest first
				});

			Bin bin;
			if (boxes.empty()) return bin;

			unsigned int totalArea = 0;
			unsigned int maxWidth = 0;
			unsigned int maxHeight = 0;

			for (const auto& box : boxes) {
				totalArea += box.width * box.height;
				maxWidth = std::max(maxWidth, box.width);
				maxHeight = std::max(maxHeight, box.height);
			}

			bin.width = maxWidth * 2;
			bin.height = maxHeight * 2;

			bool packed = 0;
			do
			{
				packed = 1;
				for (auto& box : boxes) {
					// If we couldn't place the box, increase bin size and try again
					if (!firstFitIterate(box, bin)) {
						bin.width *= 1.5;
						bin.height *= 1.5;
						bin.boxes.clear();
						packed = 0;
						break;
					}
				}
			} while (!packed);

			return bin;
		}
	};
}
