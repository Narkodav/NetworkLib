#pragma once
#include "../Namespaces.h"
#include "../Utilities/Functions.h"
#include "RayCasting.h"

#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"

#include <vector>
#include <string>
#include <memory>
#include <fstream>
#include <iostream>

namespace Physics
{
	class Hitbox
	{
	public:

		enum class HitboxTypes
		{
			HITBOX_PARALLELOGRAM,
			HITBOX_CYLINDER,
			HITBOX_SPHERE,
			HITBOX_COMPOUND,
			HITBOX_NUM
		};

		Hitbox() = default;
		Hitbox(const Hitbox&) = default;
		Hitbox(Hitbox&&) = default;
		Hitbox& operator=(const Hitbox&) = default;
		Hitbox& operator=(Hitbox&&) = default;

		static const std::vector<std::string> hitboxTypes;
		virtual ~Hitbox() = default;
		virtual bool intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const = 0;
		virtual Physics::RayCasting::IntersectResult intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const = 0;
		virtual bool contains(const glm::vec3& point, glm::vec3 thisPosition) const = 0;

	};

	//origin are in the center for static hitboxes
	//All hitboxes are static if not explicitly called dynamic

	class ParallelogramHitbox : public Hitbox
	{
	private:
		float m_width;
		float m_height;
		float m_depth;

		float m_halfWidth;
		float m_halfHeight;
		float m_halfDepth;

	public:
		ParallelogramHitbox(float width, float height, float depth) :
			m_width(width), m_height(height), m_depth(depth),
			m_halfWidth(width / 2), m_halfHeight(height / 2), m_halfDepth(depth / 2) {};

		bool intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const override;
		Physics::RayCasting::IntersectResult intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const override;
		bool contains(const glm::vec3& point, glm::vec3 thisPosition) const override;

		float getWidth() const { return m_width; };
		float getHeight() const { return m_height; };
		float getDepth() const { return m_depth; };

		friend struct IntersectionFunctions;
	};

	class CylinderHitbox : public Hitbox
	{
	private:
		float m_height;
		float m_radius;

		float m_halfHeight;
	public:
		CylinderHitbox(float height, float radius) :
			m_height(height), m_radius(radius), m_halfHeight(height / 2) {};

		bool intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const override;
		Physics::RayCasting::IntersectResult intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const override;
		bool contains(const glm::vec3& point, glm::vec3 thisPosition) const override;

		float getHeight() const { return m_height; };
		float getRadius() const { return m_radius; };

		friend struct IntersectionFunctions;
	};

	class SphereHitbox : public Hitbox
	{
	private:
		float m_radius;

	public:
		SphereHitbox(float radius) :
			m_radius(radius) {};

		bool intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const override;
		Physics::RayCasting::IntersectResult intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const override;
		bool contains(const glm::vec3& point, glm::vec3 thisPosition) const override;

		float getRadius() const { return m_radius; };

		friend struct IntersectionFunctions;
	};

	class CompoundHitbox : public Hitbox
	{
		std::vector<std::unique_ptr<Hitbox>> m_hitboxes;
		std::vector<glm::vec3> m_positions; //hitbox positions relative to center
	public:
		CompoundHitbox() = default;
		CompoundHitbox(const CompoundHitbox&) = default;
		CompoundHitbox(CompoundHitbox&&) = default;
		CompoundHitbox& operator=(const CompoundHitbox&) = default;
		CompoundHitbox& operator=(CompoundHitbox&&) = default;

		void addHitbox(std::unique_ptr<Hitbox> hitbox, glm::vec3 position);
		bool intersects(const Hitbox& other, glm::vec3 thisPosition, glm::vec3 otherPosition) const override;
		Physics::RayCasting::IntersectResult intersectsRay(Physics::RayCasting::Ray ray, glm::vec3 hitboxPosition) const override;
		bool contains(const glm::vec3& point, glm::vec3 thisPosition) const override;


	};


	struct HitboxFactory
	{
		static std::unique_ptr<Hitbox> createHitbox(const std::string& type, const std::vector<float>& params);
		static std::unique_ptr<Hitbox> createHitbox(const std::vector<std::string>& types, const std::vector<float>& params, const std::vector<glm::vec3>& positions);
		static std::unique_ptr<Hitbox> loadHitbox(const std::string& filepath);
	};

	struct IntersectionFunctions
	{
		static inline bool intersectsParallelogram(
			const ParallelogramHitbox& para1, glm::vec3 para1Position,
			const ParallelogramHitbox& para2, glm::vec3 para2Position);

		static inline bool intersectsCylinder(
			const CylinderHitbox& cyl1, glm::vec3 cyl1Position,
			const CylinderHitbox& cyl2, glm::vec3 cyl2Position);

		static inline bool intersectsSphere(
			const SphereHitbox& sph1, glm::vec3 sph1Position,
			const SphereHitbox& sph2, glm::vec3 sph2Position);

		static inline bool intersectsParallelogramCylinder(
			const ParallelogramHitbox& para, glm::vec3 paraPosition,
			const CylinderHitbox& cyl, glm::vec3 cylPosition);

		static inline bool intersectsCylinderParallelogram(const CylinderHitbox& cyl, glm::vec3 cylPosition,
			const ParallelogramHitbox& para, glm::vec3 paraPosition) {
			return intersectsParallelogramCylinder(para, paraPosition, cyl, cylPosition);
		};

		static inline bool intersectsParallelogramSphere(
			const ParallelogramHitbox& para, glm::vec3 paraPosition,
			const SphereHitbox& sph, glm::vec3 sphPosition);

		static inline bool intersectsCylinderSphere(
			const CylinderHitbox& cyl, glm::vec3 cylPosition,
			const SphereHitbox& sph, glm::vec3 sphPosition);
	};
}
