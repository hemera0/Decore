#pragma once

namespace Engine::Renderer {
	class DescriptorLayout {
	public:
		DescriptorLayout() = default;
		DescriptorLayout(std::shared_ptr<Device> device, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings, void* optionalNext = nullptr);

		VkDeviceSize m_Size{};
		std::vector<VkDeviceSize> m_Offsets{};

		VkDescriptorSetLayout GetLayout() const { return m_Layout; }
	private:
		VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
		
	public:
		DEFINE_IMPLICIT_VK(m_Layout);
	};
}