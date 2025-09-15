#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
    Pipeline::Pipeline(
        const std::vector<Shader*>& shaders,
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
        const std::vector<VkPipelineColorBlendAttachmentState>& colorBlendAttachmentStates) :
        m_Shaders(shaders),
        m_Swapchain(swapChain),
        m_ColorFormats(colorFormats),
        m_DepthFormat(depthFormat),
        m_VertexInfo(vertexInfo),
        m_DescriptorSets(descriptorSets),
        m_PushConstants(pushConstants),
        m_PipelineCache(pipelineCache)
    {
        m_MultisampleState = multisampleState;
        m_InputAssemblyState = inputAssemblyState;
        m_RasterizationState = rasterizationState;
        m_DepthStencilState = depthStencilState;
        m_DynamicState = dynamicState;
        m_ColorBlendState = colorBlendState;
        m_ColorBlendAttachmentStates = colorBlendAttachmentStates;

        CreateLayout(m_DescriptorSets, m_PushConstants);
        SetupShaderStages(m_Shaders);
        Create(
            m_Swapchain,
            m_VertexInfo,
            m_ColorFormats,
            m_DepthFormat,
            m_MultisampleState,
            m_InputAssemblyState,
            m_RasterizationState,
            m_DepthStencilState,
            m_DynamicState,
            m_ColorBlendState
        );
    }

    Pipeline::~Pipeline() {
        const auto& device = m_Swapchain.GetDevice();

        vkDeviceWaitIdle(*device);
        vkDestroyPipelineLayout(*device, m_PipelineLayout, nullptr);
        vkDestroyPipeline(*device, m_Pipeline, nullptr);
    }

    bool Pipeline::Recreate()
    {
        const auto& device = m_Swapchain.GetDevice();

        vkDeviceWaitIdle(*device);
        vkDestroyPipelineLayout(*device, m_PipelineLayout, nullptr);
        vkDestroyPipeline(*device, m_Pipeline, nullptr);

        m_ColorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        m_ColorBlendState.logicOpEnable = VK_FALSE;
        m_ColorBlendState.logicOp = VK_LOGIC_OP_COPY; // Optional
        m_ColorBlendState.attachmentCount = static_cast<uint32_t>(m_ColorBlendAttachmentStates.size());
        m_ColorBlendState.pAttachments = m_ColorBlendAttachmentStates.data();

        CreateLayout(m_DescriptorSets, m_PushConstants);
        SetupShaderStages(m_Shaders);
        Create(
            m_Swapchain,
            m_VertexInfo,
            m_ColorFormats,
            m_DepthFormat,
            m_MultisampleState,
            m_InputAssemblyState,
            m_RasterizationState,
            m_DepthStencilState,
            m_DynamicState,
            m_ColorBlendState
        );

        return true;
    }

    void Pipeline::SetupShaderStages(const std::vector<Shader*>& shaders)
    {
        m_ShaderStages.resize(shaders.size());
        m_ShaderModules.resize(shaders.size());

        // Adapted from zeux - https://github.com/zeux/niagara/blob/master/src/shaders.cpp#L531
        for (auto i = 0; i < shaders.size(); i++)
        {
            const auto& shader = shaders[i];

            shader->AddPipelineRef(this);

            const auto& bytecode = shader->GetSPIRV();

            auto& module = m_ShaderModules[i];
            module.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            module.codeSize = bytecode.size();
            module.pCode = reinterpret_cast<const uint32_t*>(bytecode.data());

            auto& stage = m_ShaderStages[i];
            stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stage.stage = shader->GetStageFlags();
            stage.pName = "main";
            stage.pNext = &module;
        }
    }

    void Pipeline::CreateLayout(const std::vector<VkDescriptorSetLayout>& descriptorSets, const std::vector<VkPushConstantRange>& pushConstants) {
        const auto& device = m_Swapchain.GetDevice();

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
        pipelineLayoutCreateInfo.setLayoutCount = 0; // Optional
        pipelineLayoutCreateInfo.pSetLayouts = nullptr; // Optional
        pipelineLayoutCreateInfo.pushConstantRangeCount = 0;
        pipelineLayoutCreateInfo.pPushConstantRanges = nullptr;

        if (!descriptorSets.empty()) {
            pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSets.size());
            pipelineLayoutCreateInfo.pSetLayouts = descriptorSets.data();
        }

        if (!pushConstants.empty()) {
            pipelineLayoutCreateInfo.pushConstantRangeCount = static_cast<uint32_t>(pushConstants.size());
            pipelineLayoutCreateInfo.pPushConstantRanges = pushConstants.data();
        }

        if (const auto result = vkCreatePipelineLayout(*device, &pipelineLayoutCreateInfo, nullptr, &m_PipelineLayout); result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipleline layout: " + std::to_string(result));
        }
    }

    VkPipelineMultisampleStateCreateInfo Pipeline::SetupMultiSampleState() {
        VkPipelineMultisampleStateCreateInfo out{ VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
        out.sampleShadingEnable = VK_FALSE;
        out.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        out.minSampleShading = 1.0f; // Optional
        out.pSampleMask = nullptr; // Optional
        out.alphaToCoverageEnable = VK_FALSE; // Optional
        out.alphaToOneEnable = VK_FALSE; // Optional
        return out;
    }

    VkPipelineInputAssemblyStateCreateInfo Pipeline::SetupInputAssemblyState() {
        VkPipelineInputAssemblyStateCreateInfo out{ VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
        out.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        out.primitiveRestartEnable = VK_FALSE;
        return out;
    }

    VkPipelineRasterizationStateCreateInfo Pipeline::SetupRasterizationState() {
        VkPipelineRasterizationStateCreateInfo out{ VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
        out.depthClampEnable = VK_TRUE;
        out.rasterizerDiscardEnable = VK_FALSE;
        out.polygonMode = VK_POLYGON_MODE_FILL;
        out.lineWidth = 1.0f;
        out.cullMode = VK_CULL_MODE_BACK_BIT; // Default back face culling
        out.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        out.depthBiasEnable = VK_FALSE;
        out.depthBiasConstantFactor = 0.0f; // Optional
        out.depthBiasClamp = 0.0f; // Optional
        out.depthBiasSlopeFactor = 0.0f; // Optional

        return out;
    }

    VkPipelineDepthStencilStateCreateInfo Pipeline::SetupDepthStencilState() {
        VkPipelineDepthStencilStateCreateInfo out{ VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
        out.depthTestEnable = VK_TRUE;
        out.depthWriteEnable = VK_TRUE;
        out.depthCompareOp = VK_COMPARE_OP_LESS;
        out.depthBoundsTestEnable = VK_FALSE;
        out.minDepthBounds = 0.0f;
        out.maxDepthBounds = 1.0f;
        out.stencilTestEnable = VK_FALSE;
        return out;
    }

    VkPipelineDynamicStateCreateInfo Pipeline::SetupDynamicState() {
        static std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo out{ VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
        out.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        out.pDynamicStates = dynamicStates.data();
        return out;
    }

    VkPipelineColorBlendAttachmentState Pipeline::SetupDefaultColorBlendAttachmentState()
    {
        VkPipelineColorBlendAttachmentState out = { };
        out.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        out.blendEnable = VK_TRUE;
        out.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        out.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        out.colorBlendOp = VK_BLEND_OP_ADD;
        out.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        out.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        out.alphaBlendOp = VK_BLEND_OP_ADD;
        return out;
    }

    void Pipeline::Create(
        const Swapchain& swapChain,
        const VertexInfo& vertexInfo,
        std::vector<VkFormat> colorFormats,
        VkFormat depthFormat,
        VkPipelineMultisampleStateCreateInfo multisampleState,
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState,
        VkPipelineRasterizationStateCreateInfo rasterizationState,
        VkPipelineDepthStencilStateCreateInfo depthStencilState,
        VkPipelineDynamicStateCreateInfo dynamicState,
        VkPipelineColorBlendStateCreateInfo colorBlendState)
    {
        const auto& device = swapChain.GetDevice();
        const auto& swapchainExtents = swapChain.GetExtents();

        VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{ VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

        if (vertexInfo.m_InputBindingDesc.empty() && vertexInfo.m_InputAttributeDesc.empty()) {
            vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
            vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
            vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
            vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;
        }
        else {
            vertexInputCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInfo.m_InputBindingDesc.size());
            vertexInputCreateInfo.pVertexBindingDescriptions = vertexInfo.m_InputBindingDesc.data();
            vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInfo.m_InputAttributeDesc.size());
            vertexInputCreateInfo.pVertexAttributeDescriptions = vertexInfo.m_InputAttributeDesc.data();
        }

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchainExtents.width);
        viewport.height = static_cast<float>(swapchainExtents.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = swapchainExtents;

        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.pViewports = &viewport;
        viewportStateCreateInfo.scissorCount = 1;
        viewportStateCreateInfo.pScissors = &scissor;

        VkPipelineRenderingCreateInfo renderingPipelineCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO };
        renderingPipelineCreateInfo.colorAttachmentCount = static_cast<uint32_t>(colorFormats.size());
        renderingPipelineCreateInfo.pColorAttachmentFormats = colorFormats.empty() ? nullptr : colorFormats.data();
        renderingPipelineCreateInfo.depthAttachmentFormat = depthFormat;

        VkGraphicsPipelineCreateInfo pipelineCreateInfo{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
        pipelineCreateInfo.stageCount = static_cast<uint32_t>(m_ShaderStages.size());
        pipelineCreateInfo.pStages = m_ShaderStages.data();
        pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
        pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
        pipelineCreateInfo.pRasterizationState = &rasterizationState;
        pipelineCreateInfo.pMultisampleState = &multisampleState;
        pipelineCreateInfo.pDepthStencilState = &depthStencilState;
        pipelineCreateInfo.pColorBlendState = &colorBlendState;
        pipelineCreateInfo.pDynamicState = &dynamicState;
        pipelineCreateInfo.layout = m_PipelineLayout;
        pipelineCreateInfo.renderPass = VK_NULL_HANDLE; // Replaced by Dynamic Rendering.
        pipelineCreateInfo.subpass = 0;
        pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineCreateInfo.basePipelineIndex = -1; // Optional
        pipelineCreateInfo.flags = VK_PIPELINE_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
        pipelineCreateInfo.pNext = &renderingPipelineCreateInfo;

        if (const auto result = vkCreateGraphicsPipelines(*device, m_PipelineCache, 1, &pipelineCreateInfo, nullptr, &m_Pipeline); result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create pipeline: " + std::to_string(result));
        }

        printf("Created pipeline: 0x%p\n", m_Pipeline);
    }

    PipelineBuilder::PipelineBuilder()
    {
        m_MultisampleState = Pipeline::SetupMultiSampleState();
        m_InputAssemblyState = Pipeline::SetupInputAssemblyState();
        m_RasterizationState = Pipeline::SetupRasterizationState();
        m_DepthStencilState = Pipeline::SetupDepthStencilState();
        m_DynamicState = Pipeline::SetupDynamicState();
        m_ColorBlendAttachmentStates = { Pipeline::SetupDefaultColorBlendAttachmentState() };
    }

    VkPipelineCache g_PipelineCache = 0;

    Pipeline* PipelineBuilder::Build(const Swapchain& swapchain)
    {
        VertexInfo vi = {
            m_InputBindingDescriptions,
            m_InputAttributeDescriptions
        };

        // Todo remove: This is bad.
        m_ColorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
        m_ColorBlendState.logicOpEnable = VK_FALSE;
        m_ColorBlendState.logicOp = VK_LOGIC_OP_COPY; // Optional
        m_ColorBlendState.attachmentCount = static_cast<uint32_t>(m_ColorBlendAttachmentStates.size());
        m_ColorBlendState.pAttachments = m_ColorBlendAttachmentStates.data();

        return new Pipeline(
            m_Shaders,
            vi,
            m_DescriptorSetLayouts,
            m_PushConstants,
            swapchain,
            m_ColorFormats,
            m_DepthFormat,
            g_PipelineCache,
            m_MultisampleState,
            m_InputAssemblyState,
            m_RasterizationState,
            m_DepthStencilState,
            m_DynamicState,
            m_ColorBlendState,
            m_ColorBlendAttachmentStates
        );
    }
}