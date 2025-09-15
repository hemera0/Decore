#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
	RenderPassAttachment::RenderPassAttachment(const VkImage image, const VkImageView imageView, const VkImageLayout imageLayout, const VkFormat imageFormat, const VkAttachmentLoadOp loadOp, const VkAttachmentStoreOp storeOp, VkClearValue clearValue) :
		m_Image(image),
		m_ImageFormat(imageFormat),
		m_ImageView(imageView),
		m_ImageLayout(imageLayout),
		m_LoadOp(loadOp),
		m_StoreOp(storeOp),
		m_ClearValue(clearValue) {
	}

	VkRenderingAttachmentInfo RenderPassAttachment::GetInfo() const
	{
		VkRenderingAttachmentInfo res = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

		res.imageView = m_ImageView;
		res.imageLayout = m_ImageLayout;
		res.loadOp = m_LoadOp;
		res.storeOp = m_StoreOp;
		res.clearValue = m_ClearValue;

		return res;
	}
}