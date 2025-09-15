#define STB_IMAGE_IMPLEMENTATION
#include "VulkanRenderer.hpp"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

#include "../Debug/DebugRenderer.hpp"

namespace Engine::Renderer
{
	void VulkanRenderer::Setup(const char** requiredExtensions, uint32_t requiredExtensionsCount, const HWND& handle)
	{
		m_Instance = std::make_unique<Instance>(requiredExtensions, requiredExtensionsCount);

		volkLoadInstance(*m_Instance);

		const std::vector<const char*> extensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_BUFFER_EXTENSION_NAME,
			VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
			VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
			VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
		};

		m_PhysicalDevice = std::make_shared<PhysicalDevice>(*m_Instance, extensions);
		m_Surface = std::make_shared<Surface>(*m_Instance, handle);
		m_Device = std::make_shared<Device>(m_Instance, m_PhysicalDevice, m_Surface);
		m_CommandPool = std::make_shared<CommandPool>(*m_Device);
		m_CommandBuffer = std::make_shared<CommandBuffer>(m_Device, m_CommandPool);
		m_ComputeCommandBuffer = std::make_shared<CommandBuffer>(m_Device, m_CommandPool);

		CreateSwapChain();
		CreateSyncObjects();

		m_DebugRenderer = new DebugRenderer();
		m_DebugRenderer->Setup(this);

		for (auto& callback : m_OnStartupCallbacks)
			callback(this);

		for (auto& renderer : m_Renderers)
			renderer->Startup();
	}

	void VulkanRenderer::CreateSwapChain()
	{
		if (!m_SwapChain)
			m_SwapChain = std::make_unique<Swapchain>(m_Device, m_PhysicalDevice, m_Surface);
		else {
			m_SwapChain->Recreate();

			for (auto& renderer : m_Renderers)
				renderer->RecreateSwapchainResources();
		}
	}

	void VulkanRenderer::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreCreateInfo{ VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VkFenceCreateInfo fenceCreateInfo{ VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		if (vkCreateSemaphore(*m_Device, &semaphoreCreateInfo, nullptr, &m_ImageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(*m_Device, &semaphoreCreateInfo, nullptr, &m_RenderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(*m_Device, &fenceCreateInfo, nullptr, &m_InFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create semaphores & fence");
		}

		if (vkCreateSemaphore(*m_Device, &semaphoreCreateInfo, nullptr, &m_ComputeFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(*m_Device, &fenceCreateInfo, nullptr, &m_ComputeInFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create compute emaphores & fence");
		}
	}

	bool VulkanRenderer::BeginFrame()
	{
		vkWaitForFences(*m_Device, 1, &m_InFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());

		const auto result = vkAcquireNextImageKHR(*m_Device, *m_SwapChain, std::numeric_limits<uint64_t>::max(), m_ImageAvailableSemaphore, VK_NULL_HANDLE, &m_CurrentImageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			CleanupSwapChain();
			CreateSwapChain();
			return false;
		}
		else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
		{
			throw std::runtime_error("failed to acquire swap chain image!");
		}

		vkResetCommandBuffer(*m_ComputeCommandBuffer, 0);

		vkResetFences(*m_Device, 1, &m_InFlightFence);
		vkResetCommandBuffer(*m_CommandBuffer, 0);

		m_CommandBuffer->Begin();
		m_ComputeCommandBuffer->Begin();

		return true;
	}

	void VulkanRenderer::EndFrame()
	{
		m_CommandBuffer->End();
		m_ComputeCommandBuffer->End();

		// Compute Submit.
		vkWaitForFences(*m_Device, 1, &m_ComputeInFlightFence, VK_TRUE, std::numeric_limits<uint64_t>::max());
		vkResetFences(*m_Device, 1, &m_ComputeInFlightFence);

		VkSubmitInfo submitInfo{ VK_STRUCTURE_TYPE_SUBMIT_INFO };

		VkCommandBuffer computeCommandBuffer = *m_ComputeCommandBuffer;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &computeCommandBuffer;
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = &m_ComputeFinishedSemaphore;

		if (vkQueueSubmit(m_Device->GetComputeQueue(), 1, &submitInfo, m_ComputeInFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to submit compute command buffer!");
		}

		// Render Submit.
		VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphore, m_ComputeFinishedSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

		std::vector<VkCommandBuffer> commandBuffers = { *m_CommandBuffer };

		submitInfo.waitSemaphoreCount = _countof(waitSemaphores);
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());
		submitInfo.pCommandBuffers = commandBuffers.data();

		VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = _countof(signalSemaphores);
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (const auto result = vkQueueSubmit(m_Device->GetGraphicsQueue(), 1, &submitInfo, m_InFlightFence); result != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to submit draw command buffer: " + std::to_string(result));
		}

		VkPresentInfoKHR presentInfo{ VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		presentInfo.waitSemaphoreCount = _countof(signalSemaphores);
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapchains[] = { *m_SwapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapchains;
		presentInfo.pImageIndices = &m_CurrentImageIndex;

		const auto result = vkQueuePresentKHR(m_Device->GetPresentQueue(), &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			RecreateSwapChain();
		}
		else if (result != VK_SUCCESS)
		{
			throw std::runtime_error("failed to present swap chain image!");
		}
	}

	void VulkanRenderer::Render(float dt)
	{
		for (auto& renderer : m_Renderers)
			renderer->PreRender();

		if (!BeginFrame())
			return;

		for (auto& renderer : m_Renderers)
			renderer->Update(m_CommandBuffer.get());

		EndFrame();
	}

	void VulkanRenderer::CleanupSwapChain()
	{
		vkDeviceWaitIdle(*m_Device);

		m_SwapChain->Destroy();
	}

	void VulkanRenderer::Cleanup( )
	{
		vkDeviceWaitIdle( *m_Device );
		vkDestroySemaphore( *m_Device, m_ImageAvailableSemaphore, nullptr );
		vkDestroySemaphore( *m_Device, m_RenderFinishedSemaphore, nullptr );
		vkDestroyFence( *m_Device, m_InFlightFence, nullptr );

		for ( auto& callback : m_OnShutdownCallbacks )
		{
			callback( this );
		}

		for ( auto& renderer : m_Renderers )
		{
			renderer->Shutdown( );
		}
	}
}