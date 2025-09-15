#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
    ImageView::ImageView(std::shared_ptr<Device> device, const VkImage& image, const uint32_t mipLevels, const VkFormat& format, VkImageViewCreateInfo* createInfo) : m_Device(device), m_Image(image) {
        VkImageViewCreateInfo imageViewCreateInfo = ImageView::GetDefault2DCreateInfo(image, mipLevels, format);

        if (const auto result = vkCreateImageView(*device, createInfo ? createInfo : &imageViewCreateInfo, nullptr, &m_ImageView); result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image view: " + std::to_string(result));
        }
    }

    VkImageViewCreateInfo ImageView::GetDefault2DCreateInfo(const VkImage& image, const uint32_t mipLevels, const VkFormat& format) {
        VkImageViewCreateInfo imageViewCreateInfo{ VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        imageViewCreateInfo.image = image;
        imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        imageViewCreateInfo.format = format;
        imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imageViewCreateInfo.subresourceRange.levelCount = mipLevels;
        imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imageViewCreateInfo.subresourceRange.layerCount = 1;
        return imageViewCreateInfo;
    }

    ImageView::~ImageView() {
        vkDeviceWaitIdle(*m_Device);
        vkDestroyImageView(*m_Device, m_ImageView, nullptr);
    }
}