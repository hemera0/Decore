#pragma once
#include <optional>
namespace Engine::Renderer {
	class RenderPassSpecification {
	public:
		static VkRenderingAttachmentInfo GetColorAttachmentInfo(const VkImageView view, VkClearValue* clear, VkImageLayout layout, VkResolveModeFlagBits resolveMode = VK_RESOLVE_MODE_NONE, VkImageView resolveImage = VK_NULL_HANDLE, VkImageLayout resolveLayout = VK_IMAGE_LAYOUT_UNDEFINED);
		static VkRenderingAttachmentInfo GetDepthAttachmentInfo(const VkImageView view, VkImageLayout layout);
		static VkRenderingInfo CreateRenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachments, VkRenderingAttachmentInfo* depthAttachment, uint32_t colorAttachmentCount = 1);
	};
}