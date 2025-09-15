#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
	DescriptorLayout::DescriptorLayout(std::shared_ptr<Device> device, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings, void* optionalNext) {
		VkDescriptorSetLayoutCreateInfo descriptorLayoutCreateInfo{ VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		descriptorLayoutCreateInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_DESCRIPTOR_BUFFER_BIT_EXT;
		descriptorLayoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
		descriptorLayoutCreateInfo.pBindings = layoutBindings.data();

		if (optionalNext)
			descriptorLayoutCreateInfo.pNext = optionalNext;

		if (const auto result = vkCreateDescriptorSetLayout(*device, &descriptorLayoutCreateInfo, nullptr, &m_Layout); result != VK_SUCCESS) {
			printf("Failed to create descriptor layout: %d\n", result);
			return;
		}

		const auto& physicalDevice = device->GetPhysicalDevice();
		const auto& descriptorBufferProperties = physicalDevice->GetDescriptorBufferProperties();

		vkGetDescriptorSetLayoutSizeEXT(*device, m_Layout, &m_Size);

		m_Size = ALIGNED_SIZE(m_Size, descriptorBufferProperties.descriptorBufferOffsetAlignment);

		m_Offsets.resize(static_cast<uint32_t>(layoutBindings.size()));
		for (auto i = 0; i < m_Offsets.size(); i++)
			vkGetDescriptorSetLayoutBindingOffsetEXT(*device, m_Layout, i, &m_Offsets[i]);
	}
}