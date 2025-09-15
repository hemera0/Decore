#pragma once

namespace Engine::Renderer {
	class ImageView {
		VkImageView m_ImageView = VK_NULL_HANDLE;
		VkImage m_Image = VK_NULL_HANDLE;

		std::shared_ptr<Device> m_Device;
	public:
		ImageView(std::shared_ptr<Device> device, const VkImage& image, const uint32_t mipLevels, const VkFormat& format, VkImageViewCreateInfo* createInfo = nullptr);
		~ImageView();

		static VkImageViewCreateInfo GetDefault2DCreateInfo(const VkImage& image, const uint32_t mipLevels, const VkFormat& format);

		VkImage GetImage() const { return m_Image; }

		DEFINE_IMPLICIT_VK(m_ImageView);
	};
}