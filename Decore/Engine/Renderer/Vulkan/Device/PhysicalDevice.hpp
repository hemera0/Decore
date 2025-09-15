#pragma once

namespace Engine::Renderer {
	class PhysicalDevice {
		const Instance& m_Instance;

		std::vector<const char*> m_WantedExtensions;
		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;

		VkPhysicalDeviceDescriptorBufferPropertiesEXT m_DescriptorBufferProperties{};

		bool IsDedicatedGPU(const VkPhysicalDevice& device);
		bool HasExtensions(const VkPhysicalDevice& device);

		void GetDeviceProperties();

		// attempts to find a suitable physical device...
		VkPhysicalDevice SelectDevice();
	public:
		PhysicalDevice(const Instance& instance, const std::vector<const char*>& wantedExts);

		uint32_t FindMemoryType(const uint32_t filter, const VkMemoryPropertyFlags propertyFlags) const;
		VkFormat FindDepthFormat() const;

		const std::vector<const char*>& GetExtensions() const { return m_WantedExtensions; }

		const VkPhysicalDeviceDescriptorBufferPropertiesEXT& GetDescriptorBufferProperties() const { return m_DescriptorBufferProperties; }
		const VkSampleCountFlagBits GetMaxUsableSampleCount() const;


		DEFINE_IMPLICIT_VK(m_PhysicalDevice);
	};
}