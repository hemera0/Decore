#pragma once

#include "../Vulkan/VulkanRenderer.hpp"
#include "../../Assets/BaseAsset.hpp"

#include "../../Core/Camera/Camera.hpp"
#include "../../Core/Collision/CollisionCapsule.hpp"

#include "../Culling/SceneCuller.hpp"
#include "../Environment/EnvironmentInfo.hpp"
#include "../Shadows/ShadowMapPass.hpp"

#include "../FFX-CACAO/ffx_cacao_impl.h"

#include <entt/entt.hpp>

namespace Engine::Renderer {
	struct alignas( 16 ) PointLight {
		glm::vec4 Position;
		glm::vec4 Color;

		float Intensity;
		float Range;

		bool Enabled;
	};

	class Scene {
	public:
		Scene(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::unique_ptr<Swapchain>& swapchain, const std::vector<DescriptorLayout>& objectLayouts);
		~Scene();

		template<typename T >
		inline T* AddAsset(T* asset) {
			m_SceneModels.push_back(asset);
			return (T*)m_SceneModels.back();
		}

		template<typename T >
		inline T* AddSkinnedAsset(T* asset) {
			m_SkinnedSceneModels.push_back(asset);
			return (T*)m_SkinnedSceneModels.back();
		}

		entt::registry m_Registry{};

		void Render(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer, std::function<void()> callback);

		bool Intersects(const Core::CollisionCapsule& capsule);
		bool Intersects( const Core::CollisionBox& aabb, glm::vec3& normal );

		Descriptor* GetSceneDescriptor() const { return m_SceneDescriptor; }
		Buffer* GetSceneUniforms() const { return m_SceneUniforms; }

		glm::vec4 LightDirection{};
		glm::vec4 LightColor{};

		Core::Camera& GetMainCamera() { return m_MainCamera; }

		ShadowMapPass* m_ShadowMapPass = nullptr;
		SceneCuller* m_Culler = nullptr;

		bool IsFrustumFrozen() const { return m_FreezeFrustum; }
		bool IsSSAOEnabled( ) const { return m_SSAOEnabled; }

		std::vector<Engine::Assets::BaseAsset*> m_SceneModels{};
		std::vector<Engine::Assets::BaseAsset*> m_SkinnedSceneModels{};

		VkDescriptorSet m_ImguiDepthPrePass = VK_NULL_HANDLE;

		void RecreateSwapchainResources( );
	private:
		// Called before the main scene rendering occurs.
		void PreRender(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer, std::function<void()> callback);
		
		// Called after the main scene rendering is ended.
		void EndRender(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer, std::function<void()> callback);

		void RenderDepthPrepass(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer);
		void RenderSSAOPass(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer);
		void RenderBloomPass( uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer );
		void RenderFinalPass(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer);

		void RenderSceneObjects(CommandBuffer& commandBuffer, const std::vector<Descriptor*>& sceneDescriptors, Renderer::Pipeline* pipeline, bool isStatic, int bufferIndex);

		void Setup(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::unique_ptr<Swapchain>& swapchain, const std::vector<DescriptorLayout>& objectLayouts);
		void SetupSkybox(const std::vector<VkFormat>& colorFormats, VkFormat depthFormat, const std::vector<DescriptorLayout>& objectLayouts, const std::vector<VkDescriptorSetLayout>& setLayouts);

		void SetupDetphPrepass(const std::vector<VkFormat>& colorFormats, VkFormat depthFormat, const std::vector<DescriptorLayout>& objectLayouts, const std::vector<VkDescriptorSetLayout>& setLayouts);
		void SetupSSAOPass();
		void SetupBloomPasses( );

		// Create bloom mip chain.
		void DestroyBloomImages();
		void CreateBloomImages(bool createDescriptors);

		bool RenderCollisions();

		bool m_FreezeFrustum{};
		bool m_SSAOEnabled{ true };

		struct alignas(16) SceneUniforms {
			glm::mat4 ModelMatrix;
			glm::mat4 ViewMatrix;
			glm::mat4 ProjectionMatrix;

			glm::vec4 CamPos;
			glm::vec4 LightDirection;
			glm::vec4 LightColor;
		} m_Uniforms;

		std::shared_ptr<Device> m_Device;
		std::shared_ptr<CommandPool> m_CommandPool;

		Swapchain* m_Swapchain = nullptr;
		
		Descriptor* m_SceneDescriptor = nullptr;
		Buffer* m_SceneUniforms = nullptr;

		Pipeline* m_SkyboxPipeline = nullptr;

		Pipeline* m_CollisionDebugPipeline = nullptr;
		Buffer* m_CollisionDebugVertexBuffer = nullptr;

		// Rendering of Opaque glTF Objects.
		Pipeline* m_OpaqueGLTFPipeline = nullptr;
		Pipeline* m_SkinnedGLTFPipeline = nullptr;

		// Depth Prepass
		Pipeline* m_PrePassOpaqueGLTFPipeline = nullptr;
		Pipeline* m_PrePassSkinnedGLTFPipeline = nullptr;
		Sampler* m_PrePassDepthSampler = nullptr;

		// SSAO Pass
		uint32_t m_SSAOImageWidth{}, m_SSAOImageHeight{};
		Image* m_SSAOImage = nullptr;
		ImageView* m_SSAOImageView = nullptr;
		VkImageCreateInfo m_SSAOImageCreateInfo{};
		Descriptor* m_SSAOImageDescriptor = nullptr;

		// Bloom Pass
		uint32_t m_BloomImageWidth{}, m_BloomImageHeight{};
		Image* m_BloomImage = nullptr;
		ImageView* m_BloomImageView = nullptr;

		Pipeline* m_HDRBloomPipeline = nullptr;
		Pipeline* m_BloomUpscalePipeline = nullptr;
		Pipeline* m_BloomDownscalePipeline = nullptr;

		Sampler* m_BloomSampler = nullptr;

		std::vector< ImageView* > m_EmissionImageViews{};
		std::vector< ImageView* > m_DownsampleImageViews{};
		std::vector< ImageView* > m_UpsampleImageViews{};
		std::vector< Descriptor* > m_BloomMipDescriptors{};

		// Fullscreen Utilities
		Pipeline* m_FullscreenPipeline = nullptr;
		Descriptor* m_FullscreenDescriptor = nullptr;
		Descriptor* m_ApplyBloomFullscreenDescriptor = nullptr;
		Sampler* m_RTSampler = nullptr;

		EnvironmentInfo* m_ActiveEnvironment = nullptr;
		Assets::BaseAsset* m_SkyCube = nullptr;

		Core::Camera m_MainCamera{};

		std::vector<Descriptor*> m_SceneDescriptors{};

		// Point Lights.
		std::vector<PointLight> m_PointLights{};

		// TODO: Replace.
		Buffer* m_PointLightsBuffer = nullptr;
		Descriptor* m_PointLightsDescriptor = nullptr;

		FFX_CACAO_VkContext* m_CacaoContext{};
	};
}