#pragma once
#include <entt/entt.hpp>
#include <glm.hpp>

#include "../../Renderer/Vulkan/VulkanRenderer.hpp"

namespace Engine::Core {
	struct Mesh {
		entt::entity m_Material{};

		std::vector<uint32_t> m_Indices{};
		std::vector<float> m_Vertices{};

		std::vector<glm::vec3> m_Positions{};
		std::vector<glm::vec2> m_UVs{};
		std::vector<glm::vec3> m_Normals{};
	};

	struct Material {

	};
}