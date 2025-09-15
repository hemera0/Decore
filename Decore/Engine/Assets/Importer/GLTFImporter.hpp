#pragma once
#include "../../Renderer/Scene/Scene.hpp"
#include "../../Renderer/Vulkan/VulkanRenderer.hpp"

namespace Engine::Assets {
	class GLTFImporter {
	public:
		GLTFImporter(Renderer::Scene& scene);

		bool LoadFromFile(const std::string& path);

		void ParseNode(const tinygltf::Node& inputNode, entt::entity parent, glm::mat4 transform);
		void ParseMaterials(entt::entity entity, const tinygltf::Material& srcMaterial);
		void ParseMesh(entt::entity entity, const tinygltf::Primitive& srcPrimitive);

		glm::mat4 GetLocalTransform();
	private:
		tinygltf::Model m_GltfFile{};

		Renderer::Scene& m_Scene;

		std::vector<entt::entity> m_MaterialEntities{};
		std::vector<entt::entity> m_AnimationEntities{};
	};
}