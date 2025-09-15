#pragma once

namespace Engine::Renderer {
	struct VertexInfo {
		std::vector<VkVertexInputBindingDescription> m_InputBindingDesc{};
		std::vector<VkVertexInputAttributeDescription> m_InputAttributeDesc{};
	};

	class Pipeline {
	private:
		void Create(
			const Swapchain& swapChain,
			const VertexInfo& vertexInfo,
			std::vector<VkFormat> colorFormats,
			VkFormat depthFormat,
			VkPipelineMultisampleStateCreateInfo multisampleState,
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState,
			VkPipelineRasterizationStateCreateInfo rasterizationState,
			VkPipelineDepthStencilStateCreateInfo depthStencilState,
			VkPipelineDynamicStateCreateInfo dynamicState,
			VkPipelineColorBlendStateCreateInfo colorBlendState
		);

		void CreateLayout(const std::vector<VkDescriptorSetLayout>& descriptorSets, const std::vector<VkPushConstantRange>& pushConstants);

		void SetupShaderStages(const std::vector<Shader*>& shaders);
		// Pipeline info setup functions
	public:
		static VkPipelineMultisampleStateCreateInfo SetupMultiSampleState();
		static VkPipelineInputAssemblyStateCreateInfo SetupInputAssemblyState();
		static VkPipelineRasterizationStateCreateInfo SetupRasterizationState();
		static VkPipelineDepthStencilStateCreateInfo SetupDepthStencilState();
		static VkPipelineDynamicStateCreateInfo SetupDynamicState();
		static VkPipelineColorBlendAttachmentState SetupDefaultColorBlendAttachmentState();
	private:
		std::vector<Shader*> m_Shaders{};
		std::vector<VkPipelineShaderStageCreateInfo> m_ShaderStages{};
		std::vector<VkShaderModuleCreateInfo> m_ShaderModules{};

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
		VkPipelineCache m_PipelineCache = VK_NULL_HANDLE;

		VertexInfo m_VertexInfo{};
		std::vector<VkDescriptorSetLayout> m_DescriptorSets{};
		std::vector<VkPushConstantRange> m_PushConstants{};

		VkPipelineMultisampleStateCreateInfo m_MultisampleState{};
		VkPipelineRasterizationStateCreateInfo m_RasterizationState{};
		VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyState{};
		VkPipelineDepthStencilStateCreateInfo m_DepthStencilState{};
		VkPipelineDynamicStateCreateInfo m_DynamicState{};

		std::vector<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachmentStates{};
		VkPipelineColorBlendStateCreateInfo m_ColorBlendState{};

		std::vector<VkFormat> m_ColorFormats{};
		VkFormat m_DepthFormat{};

		const Swapchain& m_Swapchain;
	public:
		Pipeline(const std::vector<Shader*>& shaders,
			const VertexInfo& vertexInfo,
			const std::vector<VkDescriptorSetLayout>& descriptorSets,
			const std::vector<VkPushConstantRange>& pushConstants,
			const Swapchain& swapChain,
			std::vector<VkFormat> colorFormats,
			VkFormat depthFormat,
			VkPipelineCache pipelineCache,
			VkPipelineMultisampleStateCreateInfo multisampleState,
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyState,
			VkPipelineRasterizationStateCreateInfo rasterizationState,
			VkPipelineDepthStencilStateCreateInfo depthStencilState,
			VkPipelineDynamicStateCreateInfo dynamicState,
			VkPipelineColorBlendStateCreateInfo colorBlendState,
			const std::vector<VkPipelineColorBlendAttachmentState>& colorBlendAttachmentStates);
		~Pipeline();

		bool Recreate( );

		const VkPipelineLayout& GetPipelineLayout() const { return m_PipelineLayout; }

		DEFINE_IMPLICIT_VK(m_Pipeline);
	};

	class PipelineBuilder {
	public:
		PipelineBuilder();

		PipelineBuilder& SetShaders(const std::vector<Shader*>& shaders) { m_Shaders = shaders; return *this; }
		PipelineBuilder& SetColorAttachmentFormats(const std::vector<VkFormat>& colorFormats) { m_ColorFormats = colorFormats; return *this; }
		PipelineBuilder& SetDepthAttachmentFormat(VkFormat depthFormat) { m_DepthFormat = depthFormat; return *this; }
		PipelineBuilder& SetDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout>& descriptorSets) { m_DescriptorSetLayouts = descriptorSets; return *this;
		}
		PipelineBuilder& SetPushConstants(const std::vector<VkPushConstantRange>& pushConstants) { m_PushConstants = pushConstants;return *this;
		}
		PipelineBuilder& SetInputAttributeDescriptions(const std::vector<VkVertexInputAttributeDescription>& inputAttributeDescriptions) { m_InputAttributeDescriptions = inputAttributeDescriptions; return *this;
		}
		PipelineBuilder& SetInputBindingDescriptions(const std::vector<VkVertexInputBindingDescription>& inputBindingDescriptions) { m_InputBindingDescriptions = inputBindingDescriptions;return *this;
		}

		PipelineBuilder& SetMultisampleState(const VkPipelineMultisampleStateCreateInfo& multisampleState) { m_MultisampleState = multisampleState;return *this;
		}
		PipelineBuilder& SetInputAssemblyState(const VkPipelineInputAssemblyStateCreateInfo& inputAssemblyState) { m_InputAssemblyState = inputAssemblyState; return *this;
		}
		PipelineBuilder& SetRasterizationState(const VkPipelineRasterizationStateCreateInfo& rasterizationState) { m_RasterizationState = rasterizationState;return *this;
		}
		PipelineBuilder& SetDepthStencilState(const VkPipelineDepthStencilStateCreateInfo& depthStencilState) { m_DepthStencilState = depthStencilState;return *this;
		}
		PipelineBuilder& SetDynamicState(const VkPipelineDynamicStateCreateInfo& dynamicState) { m_DynamicState = dynamicState;return *this;
		}

		PipelineBuilder& SetColorBlendAttachmentStates(const std::vector<VkPipelineColorBlendAttachmentState>& colorBlendAttachmentStates) { m_ColorBlendAttachmentStates = colorBlendAttachmentStates;return *this;
		}
		PipelineBuilder& SetColorBlendState(const VkPipelineColorBlendStateCreateInfo& colorBlendState) { m_ColorBlendState = colorBlendState;return *this;
		}

		Pipeline* Build(const Swapchain& swapchain);
	private:
		std::vector<Shader*> m_Shaders{};
		std::vector<VkFormat> m_ColorFormats{};
		VkFormat m_DepthFormat{};
		std::vector<VkDescriptorSetLayout> m_DescriptorSetLayouts{};
		std::vector<VkPushConstantRange> m_PushConstants{};
		std::vector<VkVertexInputAttributeDescription> m_InputAttributeDescriptions{};
		std::vector<VkVertexInputBindingDescription> m_InputBindingDescriptions{};

		VkPipelineMultisampleStateCreateInfo m_MultisampleState{};
		VkPipelineInputAssemblyStateCreateInfo m_InputAssemblyState{};
		VkPipelineRasterizationStateCreateInfo m_RasterizationState{};
		VkPipelineDepthStencilStateCreateInfo m_DepthStencilState{};
		VkPipelineDynamicStateCreateInfo m_DynamicState{};

		std::vector<VkPipelineColorBlendAttachmentState> m_ColorBlendAttachmentStates{};
		VkPipelineColorBlendStateCreateInfo m_ColorBlendState{};
	};
}