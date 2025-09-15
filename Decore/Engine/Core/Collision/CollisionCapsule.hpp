#pragma once
#include <glm.hpp>

namespace Engine::Core {
	class CollisionBox;

	class CollisionCapsule {
	public:
		glm::vec3 m_Bottom{};
		glm::vec3 m_Top{};

		float m_Radius{};

		// Capsule to Capsule...
		bool Intersects(const CollisionCapsule& capsule);

		// Capsule to AABB...
		bool Intersects(const CollisionBox& aabb);

		// Capsule to Triangle...
		bool Intersects(glm::vec3 p0, glm::vec3 p1, glm::vec3 p2);

		// CollisionBox GetAABB() const;
	};
}