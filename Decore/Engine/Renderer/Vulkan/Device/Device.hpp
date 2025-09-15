#pragma once

namespace Engine::Renderer {
	class Device {
	private:
		std::shared_ptr<Instance> m_Instance;
		std::shared_ptr<PhysicalDevice> m_PhysicalDevice;
		std::shared_ptr<Surface> m_Surface;

		void CreateDevice();

		VkDevice m_Device = VK_NULL_HANDLE;
		VkQueue m_GraphicsQueue = VK_NULL_HANDLE, m_PresentQueue = VK_NULL_HANDLE, m_ComputeQueue = VK_NULL_HANDLE;

		QueueFamilyIndices_t m_QueueFamilyIndices{};
	public:
		Device(std::shared_ptr<Instance> instance, std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<Surface> surface);
		~Device();

		const std::shared_ptr<PhysicalDevice> GetPhysicalDevice() const { return m_PhysicalDevice; }
		const std::shared_ptr<Surface> GetSurface() const { return m_Surface; }
		const VkQueue& GetGraphicsQueue() const { return m_GraphicsQueue; }
		const VkQueue& GetPresentQueue() const { return m_PresentQueue; }
		const VkQueue& GetComputeQueue() const { return m_ComputeQueue; }

		const QueueFamilyIndices_t& GetQueueFamilyIndices() const { return m_QueueFamilyIndices; }

		DEFINE_IMPLICIT_VK(m_Device);
	};
}