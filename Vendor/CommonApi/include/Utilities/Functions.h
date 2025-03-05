#pragma once
#include "../Namespaces.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <vector>
#include <algorithm>
#include <string>

namespace Utilities
{
    float parseFloat(const std::string& str, char endValue = '\0', unsigned int start = 0, unsigned int* stopValue = nullptr);

    glm::ivec2 tileToChunkCoord(glm::ivec3 tileCoord, int chunkSize);

    glm::ivec3 globalToLocal(glm::ivec3 tileCoord, int chunkSize);

    std::vector<glm::ivec2> getSortedCircleCoords(unsigned int radius, float padding);
};

