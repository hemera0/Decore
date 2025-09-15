#pragma once

namespace Engine::Renderer {
	class ComputePipeline {
	private:
		void Build(const Swapchain& swapChain);
		void CreateLayout(const std::vector<VkDescriptorSetLayout>& descriptorSets, const std::vector<VkPushConstantRange>& pushConstants);

		void SetupShaderStage(const Shader& shader);
	public:
		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;

		const Swapchain& m_Swapchain;

		VkPipelineShaderStageCreateInfo m_ShaderStage{};
		VkShaderModuleCreateInfo m_ShaderModule{};
	public:
		ComputePipeline(const Shader& shader,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			const std::vector<VkPushConstantRange>& pushConstants,
			const Swapchain& swapChain);
		~ComputePipeline();

		const VkPipelineLayout& GetPipelineLayout() const { return m_PipelineLayout; }

		DEFINE_IMPLICIT_VK(m_Pipeline);
	};
}