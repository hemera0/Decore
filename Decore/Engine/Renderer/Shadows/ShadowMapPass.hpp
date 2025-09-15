#pragma once
#include <glm.hpp>
#include "../Vulkan/VulkanRenderer.hpp"

#include "../../Assets/BaseAsset.hpp"
#include "../../Core/Camera/Camera.hpp"

namespace Engine::Renderer
{
	constexpr int SHADOW_MAP_DIMENSIONS = 4096;
	constexpr int SHADOW_MAP_CASCADES = 4;

	struct ShadowCascade {
		VkDescriptorSet m_ImGuiShadowMapView = VK_NULL_HANDLE;
		ImageView* m_ImageView = nullptr;

		glm::mat4 m_ViewMatrix{};
		glm::mat4 m_ProjectionMatrix{};

		float m_SplitDepth{};
		float m_Far{};
	};

	class ShadowMapPass {
	public:
		ShadowMapPass(std::shared_ptr<Device> device, const std::unique_ptr<Swapchain>& swapchain, const std::vector<DescriptorLayout>& objectLayouts );

		void Setup(std::shared_ptr<Device> device, const std::unique_ptr<Swapchain>& swapchain, const std::vector<DescriptorLayout>& objectLayouts);
		void Render( CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer, const Core::Camera& camera, const glm::vec4 lightDirection, class SceneCuller* culler, const std::vector<class Assets::BaseAsset*>& sceneAssets, std::function<void( const std::vector<Descriptor*>&, Renderer::Pipeline*, bool, int )> callback );

		VkDescriptorSet GetImage( uint32_t index ) const { return m_Cascades[index].m_ImGuiShadowMapView; }

		std::shared_ptr<Descriptor> GetDescriptor( ) const { return m_Descriptor; }
		std::shared_ptr<Descriptor> GetCascadeDescriptor() const { return m_MatricesDescriptor; }
	private:
		Swapchain* m_Swapchain;
		
		struct alignas(16) CascadeUBO {
			glm::vec4 CascadeSplits{};
			glm::mat4 CascadeViewMatrices[SHADOW_MAP_CASCADES];
			glm::mat4 CascadeProjMatrices[SHADOW_MAP_CASCADES];
		};

		struct alignas( 16 ) ShadowUBO {
			glm::mat4 ModelMatrix;
		};

		std::shared_ptr<Descriptor> m_Descriptor = nullptr;
		std::shared_ptr<Descriptor> m_MatricesDescriptor = nullptr;

		std::array<ShadowCascade, SHADOW_MAP_CASCADES> m_Cascades{};

		// Layered shadow map image.
		std::unique_ptr<Image> m_ShadowMap = nullptr;
		std::unique_ptr<ImageView> m_ShadowMapView = nullptr;

		std::unique_ptr<Pipeline> m_Pipeline = nullptr;
		std::unique_ptr<Pipeline> m_SkinnedPipeline = nullptr;

		std::unique_ptr<Descriptor> m_SceneDescriptor = nullptr;

		std::unique_ptr<Buffer> m_CascadeMatrixUniformBuffer = nullptr;
		std::unique_ptr<Buffer> m_SceneUniformBuffer = nullptr;

		std::unique_ptr<Sampler> m_Sampler = nullptr;
	};
}