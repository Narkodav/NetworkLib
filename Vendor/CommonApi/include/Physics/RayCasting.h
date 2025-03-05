#pragma once
#include "../Namespaces.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <vector>

namespace Physics
{
	class RayCasting // works in right hand system
	{
	public:

		enum class axis
		{
			X,
			Y,
			Z,
		};

		struct IntersectResult
		{
			bool intersects;
			glm::vec3 intersectionPoint;
			glm::vec3 intersectionNormal;
			float distance;
		};

		struct Ray
		{
			glm::vec3 origin;
			glm::vec3 direction;
		};

		struct Plane
		{
			glm::vec3 normal;
			glm::vec3 point;
		};

		struct Rectangle
		{
			glm::vec3 normal;
			glm::vec3 center;

			float height;
			float width;
			float rotation;
		};

		struct AxisAlignedRectangle
		{
			glm::vec3 center;
			glm::vec3 cornerMin;
			glm::vec3 cornerMax;
		};

		struct AxisAlignedBox
		{
			glm::vec3 position;
			
			float width;
			float height;
			float depth;
		};

		struct RaycastTiled3dResult
		{
			std::vector<glm::ivec3> intersectedTiles;
			std::vector<float> intersections;
		};

	private:

		static inline bool AxisAlignedRectOptimizationX(glm::vec3 point, AxisAlignedRectangle rect)
		{
			return pointIsInRectangle(glm::vec2(point.y, point.z),
				glm::vec2(rect.cornerMin.y, rect.cornerMin.z), glm::vec2(rect.cornerMax.y, rect.cornerMax.z));
		}

		static inline bool AxisAlignedRectOptimizationY(glm::vec3 point, AxisAlignedRectangle rect)
		{
			return pointIsInRectangle(glm::vec2(point.z, point.x),
				glm::vec2(rect.cornerMin.z, rect.cornerMin.x), glm::vec2(rect.cornerMax.z, rect.cornerMax.x));
		}

		static inline bool AxisAlignedRectOptimizationZ(glm::vec3 point, AxisAlignedRectangle rect)
		{
			return pointIsInRectangle(glm::vec2(point.x, point.y),
				glm::vec2(rect.cornerMin.x, rect.cornerMin.y), glm::vec2(rect.cornerMax.x, rect.cornerMax.y));
		}

	public:

		static IntersectResult intersectsPlane(Ray ray, Plane plane)
		{
			IntersectResult result;

			plane.normal = glm::normalize(plane.normal);

			// Calculate denominator
			float denom = glm::dot(plane.normal, ray.direction);

			// Check if ray is parallel
			if (abs(denom) < 0.0001f) {
				result.intersects = 0;
				return result;
			}

			float t = glm::dot(plane.point - ray.origin, plane.normal) / denom;
		
			// Check if plane is behind the ray
			if (t < 0) {
				result.intersects = 0;
				return result;
			}

			// Calculate intersection point
			result.intersects = 1;
			result.distance = t;
			result.intersectionPoint = ray.origin + ray.direction * t;
			result.intersectionNormal = (denom > 0) ? plane.normal : -plane.normal;
			return result;

		}

		static bool pointIsInRectangle(glm::vec2 point, glm::vec2 minCorner, glm::vec2 maxCorner) {
			return point.x >= minCorner.x && point.x <= maxCorner.x &&
				point.y >= minCorner.y && point.y <= maxCorner.y;
		}

		// if y is normal replaces height with depth and assumes plane point is the center
		static IntersectResult intersectsAxisAlignedRectangle(Ray ray, AxisAlignedRectangle rect)
		{
			axis alignedAxis;
			Plane plane;
			bool(*optimization)(glm::vec3, AxisAlignedRectangle);
			if (rect.cornerMin.x == rect.cornerMax.x)
			{
				plane.normal = glm::vec3(1, 0, 0);
				optimization = AxisAlignedRectOptimizationX;
			}
			else if (rect.cornerMin.y == rect.cornerMax.y)
			{
				plane.normal = glm::vec3(0, 1, 0);
				optimization = AxisAlignedRectOptimizationY;
			}
			else if (rect.cornerMin.z == rect.cornerMax.z)
			{
				plane.normal = glm::vec3(0, 0, 1);
				optimization = AxisAlignedRectOptimizationZ;
			}
			else return { 0 };

			IntersectResult result = intersectsPlane(ray, plane);
			if (!result.intersects)
				return result;

			result.intersects = optimization(result.intersectionPoint, rect);
			return result;
		}

		static IntersectResult intersectsAxisAlignedBox(Ray ray, AxisAlignedBox box)
		{

			glm::vec3 dirfrac = 1.0f / ray.direction;

			float halfWidth = box.width / 2;
			float halfHeight = box.height / 2;
			float halfDepth = box.depth / 2;

			// Calculate intersections with box slabs
			float t1 = (box.position.x - halfWidth - ray.origin.x) * dirfrac.x;
			float t2 = (box.position.x + halfWidth - ray.origin.x) * dirfrac.x;

			float t3 = (box.position.y - halfHeight - ray.origin.y) * dirfrac.y;
			float t4 = (box.position.y + halfHeight - ray.origin.y) * dirfrac.y;

			float t5 = (box.position.z - halfDepth - ray.origin.z) * dirfrac.z;
			float t6 = (box.position.z + halfDepth - ray.origin.z) * dirfrac.z;

			float tEnter = glm::max(glm::max(glm::min(t1, t2), glm::min(t3, t4)), glm::min(t5, t6));
			float tExit = glm::min(glm::min(glm::max(t1, t2), glm::max(t3, t4)), glm::max(t5, t6));

			// Check if there's a valid intersection
			if (tExit < 0 || tEnter > tExit) {
				return { 0 };
			}

			// Calculate intersection point and normal
			float t = tEnter < 0 ? tExit : tEnter;
			glm::vec3 intersectionPoint = ray.origin + ray.direction * t;

			// Calculate normal based on which face was hit
			glm::vec3 normal(0);
			if (t == t1) normal = glm::vec3(-1, 0, 0);
			else if (t == t2) normal = glm::vec3(1, 0, 0);
			else if (t == t3) normal = glm::vec3(0, -1, 0);
			else if (t == t4) normal = glm::vec3(0, 1, 0);
			else if (t == t5) normal = glm::vec3(0, 0, -1);
			else if (t == t6) normal = glm::vec3(0, 0, 1);

			return { 1, intersectionPoint, normal, t };
		}

		static RaycastTiled3dResult RaycastTiled3d(Ray ray, float rayLength)
		{
			ray.direction = glm::normalize(ray.direction);
			RaycastTiled3dResult result;
			// Calculate the step size for each axis
			glm::ivec3 step = glm::sign(ray.direction);
			
			glm::vec3 tDelta = glm::vec3(
				(std::abs(ray.direction.x) < 0.000001f) ? std::numeric_limits<float>::infinity() : std::abs(1.0f / ray.direction.x),
				(std::abs(ray.direction.y) < 0.000001f) ? std::numeric_limits<float>::infinity() : std::abs(1.0f / ray.direction.y),
				(std::abs(ray.direction.z) < 0.000001f) ? std::numeric_limits<float>::infinity() : std::abs(1.0f / ray.direction.z)
			);

			glm::ivec3 currentBlock = glm::ivec3(floor(ray.origin.x), floor(ray.origin.y), floor(ray.origin.z));

			glm::vec3 sideDist(
				(ray.direction.x > 0) ? (currentBlock.x + 1.0f - ray.origin.x) * tDelta.x : (ray.origin.x - currentBlock.x) * tDelta.x,
				(ray.direction.y > 0) ? (currentBlock.y + 1.0f - ray.origin.y) * tDelta.y : (ray.origin.y - currentBlock.y) * tDelta.y,
				(ray.direction.z > 0) ? (currentBlock.z + 1.0f - ray.origin.z) * tDelta.z : (ray.origin.z - currentBlock.z) * tDelta.z
			);

			result.intersectedTiles.push_back(currentBlock);
			result.intersections.push_back(0.f);

			// Perform DDA
			float distance = 0.0f;
			while (distance < rayLength) {
				// Find the axis with the shortest distance to next boundary
				if (sideDist.x < sideDist.y && sideDist.x < sideDist.z) {
					distance = sideDist.x;
					currentBlock.x += step.x;
					sideDist.x += tDelta.x;
					result.intersectedTiles.push_back(currentBlock);
					result.intersections.push_back(distance);
				}
				else if (sideDist.y < sideDist.z) {
					distance = sideDist.y;
					currentBlock.y += step.y;
					sideDist.y += tDelta.y;
					result.intersectedTiles.push_back(currentBlock);
					result.intersections.push_back(distance);
				}
				else {
					distance = sideDist.z;
					currentBlock.z += step.z;
					sideDist.z += tDelta.z;
					result.intersectedTiles.push_back(currentBlock);
					result.intersections.push_back(distance);
				}
			}

			return result;
		}
	};


}

