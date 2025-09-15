#pragma once
#include <glm.hpp>

namespace Engine::Core {
	class CollisionSphere {
	public:
		glm::vec3 m_Center{};
		float m_Radius{};

		bool Intersects(const CollisionSphere& sphere);
	};
}