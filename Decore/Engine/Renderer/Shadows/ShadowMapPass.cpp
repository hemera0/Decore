#include "ShadowMapPass.hpp"

#include "../../Assets/Assets.hpp"
#include "../../../../Dependencies/imgui/backends/imgui_impl_vulkan.h"
#include "../Scene/Scene.hpp"

namespace Engine::Renderer {

	ShadowMapPass::ShadowMapPass(std::shared_ptr<Device> device, const std::unique_ptr<Swapchain>& swapchain, const std::vector<DescriptorLayout>& objectLayouts) : m_Swapchain(swapchain.get()) {
		Setup(device, swapchain, objectLayouts);
	}

	void ShadowMapPass::Setup(std::shared_ptr<Device> device, const std::unique_ptr<Swapchain>& swapchain, const std::vector<DescriptorLayout>& objectLayouts)
	{
		const auto& physicalDevice = device->GetPhysicalDevice();
		const auto depthFormat = VK_FORMAT_D16_UNORM; // physicalDevice->FindDepthFormat();

		// No color attachments.
		std::vector<VkFormat> colorFormats{};

		m_Sampler = std::make_unique<Sampler>(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, 1.f, 0.f, 1.f);

		// Yes this is kind of ugly.
		auto shadowMapCreateInfo = Image::GetDefault2DCreateInfo(
			SHADOW_MAP_DIMENSIONS,
			SHADOW_MAP_DIMENSIONS,
			1,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		);

		shadowMapCreateInfo.arrayLayers = SHADOW_MAP_CASCADES;

		m_ShadowMap = std::make_unique<Image>(
			device,
			SHADOW_MAP_DIMENSIONS,
			SHADOW_MAP_DIMENSIONS,
			1,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			&shadowMapCreateInfo
		);

		VkImageViewCreateInfo imageViewCI = ImageView::GetDefault2DCreateInfo(*m_ShadowMap, 1, depthFormat);
		imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
		imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		imageViewCI.subresourceRange.layerCount = SHADOW_MAP_CASCADES;

		m_ShadowMapView = std::make_unique<ImageView>(device, *m_ShadowMap, 1, depthFormat, &imageViewCI);

		for (auto i = 0; i < m_Cascades.size(); i++)
		{
			auto& cascade = m_Cascades[i];

			VkImageViewCreateInfo imageViewCI = ImageView::GetDefault2DCreateInfo(*m_ShadowMap, 1, depthFormat);
			imageViewCI.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
			imageViewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
			imageViewCI.subresourceRange.layerCount = 1;
			imageViewCI.subresourceRange.baseArrayLayer = i;

			cascade.m_ImageView = new ImageView(device, *m_ShadowMap, 1, depthFormat, &imageViewCI);
			cascade.m_ImGuiShadowMapView = ImGui_ImplVulkan_AddTexture(*m_Sampler, *cascade.m_ImageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
		}

		m_SceneUniformBuffer = std::make_unique<Buffer>(device, sizeof(ShadowUBO),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

		std::vector<VkDescriptorSetLayoutBinding> bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, nullptr }
		};

		m_SceneDescriptor = std::make_unique<Descriptor>(device, bindings, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		m_SceneDescriptor->Bind({ Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {.m_Buffer = m_SceneUniformBuffer.get() }} });

		m_CascadeMatrixUniformBuffer = std::make_unique<Buffer>(device, sizeof(CascadeUBO),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

		bindings = { { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, nullptr } };
		m_MatricesDescriptor = std::make_shared<Descriptor>(device, bindings, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		m_MatricesDescriptor->Bind(
			{
				Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, { .m_Buffer = m_CascadeMatrixUniformBuffer.get() }}
			}
		);

		bindings = { { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr } };
		m_Descriptor = std::make_shared<Descriptor>(device, bindings, VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT);
		m_Descriptor->Bind(
			{
				Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, { .m_ImageView = m_ShadowMapView.get() }, m_Sampler.get() }
			}
		);

		VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterizationState.cullMode = VK_CULL_MODE_NONE;
		rasterizationState.depthBiasEnable = VK_TRUE;
		rasterizationState.depthClampEnable = VK_TRUE;
		rasterizationState.lineWidth = 1.f;

		std::vector<VkDescriptorSetLayout> setLayouts = { m_SceneDescriptor->GetLayout(), m_MatricesDescriptor->GetLayout() };

		for (const auto& layout : objectLayouts)
		{
			setLayouts.push_back(layout.GetLayout());
		}

		std::vector<Shader*> shaders = {
			ShaderRegistry::Get( )->Register( "..\\Shaders\\ShadowMapVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT ),
			ShaderRegistry::Get( )->Register( "..\\Shaders\\ShadowMapPS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT )
		};

		std::vector<VkPushConstantRange> pushConstants = { VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t) } };

		m_Pipeline = std::unique_ptr<Pipeline>(
			PipelineBuilder()
			.SetShaders(shaders)
			.SetColorAttachmentFormats(colorFormats)
			.SetDepthAttachmentFormat(depthFormat)
			.SetInputAttributeDescriptions(Assets::StaticGLTFAsset::VertexType::GetInputAttributeDescriptions())
			.SetInputBindingDescriptions({ Assets::StaticGLTFAsset::VertexType::GetBindingDescription() })
			.SetDescriptorSetLayouts(setLayouts)
			.SetPushConstants(pushConstants)
			.SetRasterizationState(rasterizationState)
			.Build(*swapchain)
		);

		shaders = { ShaderRegistry::Get()->Register("..\\Shaders\\SkinnedShadowMapVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT) };
		pushConstants = { VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t) } };

		setLayouts.push_back(Renderer::DescriptorLayout(device, { { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr} }));

		m_SkinnedPipeline = std::unique_ptr<Pipeline>(
			PipelineBuilder()
			.SetShaders(shaders)
			.SetColorAttachmentFormats(colorFormats)
			.SetDepthAttachmentFormat(depthFormat)
			.SetInputAttributeDescriptions(Assets::SkinnedGLTFAsset::VertexType::GetInputAttributeDescriptions())
			.SetInputBindingDescriptions({ Assets::SkinnedGLTFAsset::VertexType::GetBindingDescription() })
			.SetDescriptorSetLayouts(setLayouts)
			.SetPushConstants(pushConstants)
			.SetRasterizationState(rasterizationState)
			.Build(*swapchain)
		);
	}

	void ShadowMapPass::Render(CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer, const Core::Camera& camera, const glm::vec4 lightDirection, class SceneCuller* culler, const std::vector<Assets::BaseAsset*>& sceneAssets, std::function<void(const std::vector<Descriptor*>&, Renderer::Pipeline*, bool, int)> callback) {
		float cascadeSplits[SHADOW_MAP_CASCADES] = {};
		float lastSplitDist = 0.f;

		float nearClip = camera.GetNearClip();
		float farClip = camera.GetFarClip();
		float clipRange = farClip - nearClip;

		float minZ = nearClip;
		float maxZ = nearClip + clipRange;

		float range = maxZ - minZ;
		float ratio = maxZ / minZ;

		for (auto i = 0; i < SHADOW_MAP_CASCADES; i++) {
			float p = (i + 1) / static_cast<float>(SHADOW_MAP_CASCADES);
			float log = minZ * std::pow(ratio, p);
			float uniform = minZ + range * p;
			float d = 0.95f * (log - uniform) + uniform;
			cascadeSplits[i] = (d - nearClip) / clipRange;
		}

		for (auto i = 0; i < m_Cascades.size(); i++) {
			auto& cascade = m_Cascades[i];
			auto splitDist = cascadeSplits[i];
			auto frustumCorners = camera.CalculateFrustumCorners();

			for (uint32_t j = 0; j < 4; j++) {
				glm::vec4 dist = frustumCorners[j + 4] - frustumCorners[j];
				frustumCorners[j + 4] = frustumCorners[j] + (dist * splitDist);
				frustumCorners[j] = frustumCorners[j] + (dist * lastSplitDist);
			}

			glm::vec4 frustumCenter = glm::vec4(0.0f);
			for (uint32_t j = 0; j < 8; j++) {
				frustumCenter += frustumCorners[j];
			}

			frustumCenter /= 8.0f;

			float radius = 0.f;
			for (auto j = 0; j < 8; j++) {
				float dist = glm::length(frustumCorners[j] - frustumCenter);
				radius = std::max(radius, dist);
			}

			radius = std::ceil(radius * 16.0f) / 16.0f;

			glm::vec3 frustumMaxs = glm::vec3(radius);
			glm::vec3 frustumMins = -frustumMaxs;

			glm::mat4 lightViewMatrix = glm::lookAtLH(glm::vec3(frustumCenter) - glm::vec3(lightDirection) * -frustumMins.z, glm::vec3(frustumCenter), glm::vec3(0.0f, 1.0f, 0.0f));
			glm::mat4 lightProjMatrix = glm::orthoLH(frustumMins.x, frustumMaxs.x, frustumMins.y, frustumMaxs.y, 0.f, frustumMaxs.z - frustumMins.z);

			lightProjMatrix[1][1] *= -1.f;

			cascade.m_SplitDepth = (camera.GetNearClip() + splitDist * clipRange) * -1.0f;
			cascade.m_ViewMatrix = lightViewMatrix;
			cascade.m_ProjectionMatrix = lightProjMatrix;
			cascade.m_Far = frustumMaxs.z - frustumMins.z;

			lastSplitDist = cascadeSplits[i];
		}

		CascadeUBO cascadeUbo{};

		for (auto i = 0; i < m_Cascades.size(); i++) {
			cascadeUbo.CascadeSplits[i] = m_Cascades[i].m_SplitDepth;
			cascadeUbo.CascadeProjMatrices[i] = m_Cascades[i].m_ProjectionMatrix;
			cascadeUbo.CascadeViewMatrices[i] = m_Cascades[i].m_ViewMatrix;
		}

		m_CascadeMatrixUniformBuffer->Patch(&cascadeUbo, sizeof(CascadeUBO));

		//for ( auto i = 0; i < m_Cascades.size( ); i++ )
		//{
		//	culler->Cull( m_Cascades[ i ].m_ViewMatrix, m_Cascades[ i ].m_ProjectionMatrix, 0.f, m_Cascades[ i ].m_Far, computeCommandBuffer, sceneAssets, i );
		//}

		for (auto i = 0; i < m_Cascades.size(); i++) {
			auto& cascade = m_Cascades[i];

			ShadowUBO shadowScene{};
			shadowScene.ModelMatrix = glm::mat4(1.f);
			shadowScene.ModelMatrix[2][2] *= -1.f;

			m_SceneUniformBuffer->Patch(&shadowScene, sizeof(shadowScene));

			VkExtent2D extents = { SHADOW_MAP_DIMENSIONS, SHADOW_MAP_DIMENSIONS };
			VkClearValue val = { .depthStencil = { 1.0f, 0 } };

			VkImageView depthImageView = *cascade.m_ImageView;

			auto depthAttachment = RenderPassSpecification::GetDepthAttachmentInfo(depthImageView, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			commandBuffer.ImageBarrier(*
				m_ShadowMap,
				0,
				VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, uint32_t(i), 1 }
			);

			VkRenderingInfo renderingInfo = RenderPassSpecification::CreateRenderingInfo(extents, nullptr, &depthAttachment);
			
			commandBuffer.BeginRendering(&renderingInfo);			
			commandBuffer.SetViewport(static_cast<float>(SHADOW_MAP_DIMENSIONS), static_cast<float>(SHADOW_MAP_DIMENSIONS));
			commandBuffer.SetScissor(SHADOW_MAP_DIMENSIONS, SHADOW_MAP_DIMENSIONS);

			std::vector<Descriptor*> sceneDescriptors = { m_SceneDescriptor.get(), m_MatricesDescriptor.get() };

			uint32_t cascadeIndex = i;
			vkCmdPushConstants(commandBuffer, m_Pipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &cascadeIndex);
			callback(sceneDescriptors, m_Pipeline.get(), true, 1 + i);

			vkCmdPushConstants(commandBuffer, m_SkinnedPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(uint32_t), &cascadeIndex);
			callback(sceneDescriptors, m_SkinnedPipeline.get(), false, 1 + i);

			commandBuffer.EndRendering();
		}
	}
}