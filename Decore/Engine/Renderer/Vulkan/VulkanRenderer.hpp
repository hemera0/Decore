#pragma once
#include <set>
#include <cstdint>
#include <vector>
#include <array>
#include <string>
#include <algorithm>
#include <windows.h>
#include <functional>
#include <memory>
#include <stb_image.h>

#include <volk.h>

#define DEFINE_IMPLICIT_VK( x ) operator const decltype(x) & () const { return x; }
#define ALIGNED_SIZE( value, align ) ( (value + align - 1) & ~(align - 1) )

#include "Core/Instance.hpp"
#include "Device/PhysicalDevice.hpp"
#include "Core/Surface.hpp"
#include "Device/Device.hpp"
#include "Commands/CommandPool.hpp"
#include "Images/Image.hpp"
#include "Images/ImageView.hpp"
#include "Core/Swapchain.hpp"
#include "Shaders/Shader.hpp"
#include "Shaders/ShaderRegistry.hpp"

#include "RenderPasses/RenderPassAttachment.hpp"
#include "RenderPasses/RenderPassSpecification.hpp"

#include "Pipeline/Pipeline.hpp"
#include "Pipeline/ComputePipeline.hpp"
#include "Buffers/Buffer.hpp"
#include "Commands/CommandBuffer.hpp"
#include "Descriptors/DescriptorLayout.hpp"
#include "Descriptors/Descriptor.hpp"
#include "Images/Sampler.hpp"

namespace Engine::Renderer {
	class BaseRenderer;
	class DebugRenderer;

	class VulkanRenderer {
	public:
		void Setup(const char** requiredExtensions, uint32_t requiredExtensionsCount, const HWND& handle);

		void Render(float dt);
		void Cleanup();

		__forceinline void RecreateSwapChain() {
			CleanupSwapChain();
			CreateSwapChain();
		}

		std::shared_ptr<Instance> m_Instance = nullptr;
		std::shared_ptr<PhysicalDevice> m_PhysicalDevice = nullptr;
		std::shared_ptr<Surface> m_Surface = nullptr;
		std::shared_ptr<Device> m_Device = nullptr;
		std::shared_ptr<CommandPool> m_CommandPool = nullptr;
		std::shared_ptr<CommandBuffer> m_CommandBuffer = nullptr;
		std::shared_ptr<CommandBuffer> m_ComputeCommandBuffer = nullptr;

		std::unique_ptr<Swapchain> m_SwapChain = nullptr;

		// Sync objects.
		VkSemaphore m_ImageAvailableSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_RenderFinishedSemaphore = VK_NULL_HANDLE;
		VkFence m_InFlightFence = VK_NULL_HANDLE;
		uint32_t m_CurrentImageIndex{};

		VkSemaphore m_ComputeFinishedSemaphore = VK_NULL_HANDLE;
		VkFence m_ComputeInFlightFence = VK_NULL_HANDLE;

		DebugRenderer* m_DebugRenderer = nullptr;

		__forceinline void AddOnStartup(const std::function<void(VulkanRenderer*)>& func) {
			m_OnStartupCallbacks.push_back(func);
		}

		__forceinline void AddOnShutdown(const std::function<void(VulkanRenderer*)>& func) {
			m_OnShutdownCallbacks.push_back(func);
		}

		std::vector<std::function<void(VulkanRenderer*)>> m_OnStartupCallbacks{};
		std::vector<std::function<void(VulkanRenderer*)>> m_OnShutdownCallbacks{};

		using RenderPassCallback_t = std::function<void(VulkanRenderer*, CommandBuffer*)>;

		std::vector<BaseRenderer*> m_Renderers{};

		__forceinline void AddRenderer(BaseRenderer* renderer) {
			m_Renderers.push_back(renderer);
		}
	private:
		bool BeginFrame();
		void EndFrame();

		void CreateSyncObjects();

		void CleanupSwapChain();
		void CreateSwapChain();
	};
}

#include "BaseRenderer.hpp"