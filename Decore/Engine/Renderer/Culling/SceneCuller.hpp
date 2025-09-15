#pragma once
#include "../Vulkan/VulkanRenderer.hpp"
#include "../../Assets/glTF/StaticGLTFAsset.hpp"

#include "glm.hpp"

namespace Engine::Renderer {
	class SceneCuller {
	public:
		struct CullingUniforms {
			glm::vec4 Frustum{};
			glm::mat4 CameraView;
			glm::vec2 NearFar;
		};

		SceneCuller(std::shared_ptr<Device> device, Swapchain* swapchain, const DescriptorLayout& primitiveLayout);

		void Cull(const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const float nearClip, const float farClip, CommandBuffer& commandBuffer, std::vector<Assets::BaseAsset*> staticGeometry, int cascadeIndex = -1);
	private:
		Descriptor* m_Descriptor = nullptr;
		std::array<Buffer*, 5> m_CullDatas = {}; // Culling UBO (Frustum Planes)
		ComputePipeline* m_Pipeline = nullptr;
	};
}