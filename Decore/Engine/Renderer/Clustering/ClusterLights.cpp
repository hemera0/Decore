#include "ClusterLights.hpp"

namespace Engine::Renderer {
	ClusterLights::ClusterLights(const std::vector<PointLight>& lights, const Renderer::Swapchain& swapchain, const Renderer::CommandPool& commandPool) 
		// : m_PointLights(lights)
	{
		/*
		const auto& device = swapchain.GetDevice();
		// const auto& graphicsQueue = device.GetGraphicsQueue();
		// std::unique_ptr<StagingBuffer> clustersStagingBuffer = std::make_unique<StagingBuffer>(device, clustersBufferSize);

		const auto clustersBufferSize = sizeof(LightCluster) * m_NumClusters;
		m_ClustersBuffer = new Buffer(device, clustersBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

		m_ClusterUniforms = new Buffer(device, sizeof(ComputeUniforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

		m_ClustersDescriptor = new Descriptor(
			device,
			{
				{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr },
				{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr }
			},
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		const auto genClustersShader = Shader("../Shaders/Compute/ClusterGenCS.hlsl", VK_SHADER_STAGE_COMPUTE_BIT);
		const auto cullLightsShader = Shader("../Shaders/Compute/ClusterCullCS.hlsl", VK_SHADER_STAGE_COMPUTE_BIT);

		m_GenClusterPipeline = new Renderer::ComputePipeline(genClustersShader, { m_ClustersDescriptor->GetLayout() }, {}, swapchain);
		m_LightCullPipeline = new Renderer::ComputePipeline(cullLightsShader, {}, {}, swapchain);
		*/
	}

	ClusterLights::~ClusterLights() {

	}

	void ClusterLights::Compute(const Core::Camera& camera, Renderer::CommandBuffer& commandBuffer) const {
		/*
		commandBuffer.BindDescriptors({ m_ClustersDescriptor });

		// Dispatc to build clusters.
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_GenClusterPipeline);
		vkCmdDispatch(commandBuffer, m_GridSizeX, m_GridSizeY, m_GridSizeZ);

		ComputeUniforms uniforms{};
		uniforms.NearFar = glm::vec2(camera.GetNearClip(), camera.GetFarClip());
		uniforms.InverseProjection = glm::inverse(camera.GetProjectionMatrix());
		uniforms.GridSize = glm::uvec3(m_GridSizeX, m_GridSizeY, m_GridSizeZ);
		// 	glm::vec2 ScreenDimensions;
	
		m_ClustersBuffer->Patch(&uniforms, sizeof(ComputeUniforms));
		*/
		// Dispatch to cull lights.
		// vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *m_ComputeLightCullPipeline);
		// vkCmdDispatch(commandBuffer, 27, 1, 1);
	}
}