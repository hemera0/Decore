#pragma once
#include "../Vulkan/VulkanRenderer.hpp"
#include "../../Core/Camera/Camera.hpp"

#include "glm.hpp"

namespace Engine::Renderer {
	constexpr const int LightsPerCluster = 100;

	struct PointLight;
	struct alignas(16) LightCluster {
		glm::vec4 MinPos;
		glm::vec4 MaxPos;
		uint32_t LightCount;
		uint32_t LightIndices[LightsPerCluster];
	};

	struct alignas(16) ComputeUniforms {
		glm::vec2 NearFar;
		glm::mat4 InverseProjection;
		glm::vec3 GridSize;
		glm::vec2 ScreenDimensions;
	};

	class ClusterLights {
	private:
		// std::vector<PointLight> m_PointLights{};
		int m_GridSizeX = 12, m_GridSizeY = 12, m_GridSizeZ = 24;
		int m_NumClusters = m_GridSizeX * m_GridSizeY * m_GridSizeZ;
	public:
		Renderer::Descriptor* m_ClustersDescriptor = nullptr;
		Renderer::Buffer* m_ClustersBuffer = nullptr;
		Renderer::Buffer* m_ClusterUniforms = nullptr;

		Renderer::ComputePipeline* m_GenClusterPipeline = nullptr;
		Renderer::ComputePipeline* m_LightCullPipeline = nullptr;

		ClusterLights(const std::vector<PointLight>& lights, const Renderer::Swapchain& swapchain, const Renderer::CommandPool& commandPool);
		~ClusterLights();

		void Compute(const Core::Camera& camera, Renderer::CommandBuffer& commandBuffer) const;
	};
}