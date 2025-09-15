#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
	void ComputePipeline::Build(const Swapchain& swapChain) {
		const auto& device = swapChain.GetDevice();

		VkComputePipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO };
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.stage = m_ShaderStage;
		pipelineInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;

		if (vkCreateComputePipelines(*device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline) != VK_SUCCESS) {
			throw std::runtime_error("failed to create compute pipeline!");
		}
	}

	void ComputePipeline::CreateLayout(const std::vector<VkDescriptorSetLayout>& descriptorSets, const std::vector<VkPushConstantRange>& pushConstants) {
		const auto& device = m_Swapchain.GetDevice();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipelineLayoutInfo.setLayoutCount = descriptorSets.empty() ? 0 : static_cast<uint32_t>(descriptorSets.size());

		pipelineLayoutInfo.pSetLayouts = descriptorSets.empty() ? nullptr : descriptorSets.data();
		pipelineLayoutInfo.pushConstantRangeCount = pushConstants.empty() ? 0 : static_cast<uint32_t>(pushConstants.size());
		pipelineLayoutInfo.pPushConstantRanges = pushConstants.empty() ? nullptr : pushConstants.data();

		if (const auto result = vkCreatePipelineLayout(*device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout); result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create compute pipleline layout: " + std::to_string(result));
		}
	}

	void ComputePipeline::SetupShaderStage(const Shader& shader)
	{
		const auto& bytecode = shader.GetSPIRV();

		m_ShaderModule = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		m_ShaderModule.codeSize = bytecode.size();
		m_ShaderModule.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

		m_ShaderStage = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		m_ShaderStage.stage = shader.GetStageFlags();
		m_ShaderStage.pName = "main";
		m_ShaderStage.pNext = &m_ShaderModule;
	}

	ComputePipeline::ComputePipeline(const Shader& shader,
		const std::vector<VkDescriptorSetLayout>& descriptorSets,
		const std::vector<VkPushConstantRange>& pushConstants,
		const Swapchain& swapChain) : m_Swapchain(swapChain) {

		CreateLayout(descriptorSets, pushConstants);
		SetupShaderStage(shader);
		Build(swapChain);
	}

	ComputePipeline::~ComputePipeline() {
		const auto& device = m_Swapchain.GetDevice();

		vkDeviceWaitIdle(*device);
		vkDestroyPipelineLayout(*device, m_PipelineLayout, nullptr);
		vkDestroyPipeline(*device, m_Pipeline, nullptr);
	}
}