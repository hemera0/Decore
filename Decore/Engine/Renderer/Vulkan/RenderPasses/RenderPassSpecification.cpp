#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
	VkRenderingAttachmentInfo RenderPassSpecification::GetColorAttachmentInfo(const VkImageView view, VkClearValue* clear, VkImageLayout layout, VkResolveModeFlagBits resolveMode, VkImageView resolveImage, VkImageLayout resolveLayout)
	{
		VkRenderingAttachmentInfo colorAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

		colorAttachment.imageView = view;
		colorAttachment.imageLayout = layout;
		colorAttachment.loadOp = clear ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		if (clear) {
			colorAttachment.clearValue = *clear;
		}

		colorAttachment.resolveMode = resolveMode;
		colorAttachment.resolveImageView = resolveImage;
		colorAttachment.resolveImageLayout = resolveLayout;

		return colorAttachment;
	}

	VkRenderingAttachmentInfo RenderPassSpecification::GetDepthAttachmentInfo(const VkImageView view, VkImageLayout layout)
	{
		VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };

		depthAttachment.imageView = view;
		depthAttachment.imageLayout = layout;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.clearValue.depthStencil.depth = 1.f;

		return depthAttachment;
	}

	VkRenderingInfo RenderPassSpecification::CreateRenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo* colorAttachment, VkRenderingAttachmentInfo* depthAttachment, uint32_t colorAttachmentCount)
	{
		VkRenderingInfo renderInfo{};
		renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderInfo.pNext = nullptr;

		renderInfo.renderArea = VkRect2D{ VkOffset2D { 0, 0 }, renderExtent };
		renderInfo.layerCount = 1;

		if (colorAttachment) {
			renderInfo.colorAttachmentCount = colorAttachmentCount;
			renderInfo.pColorAttachments = colorAttachment;
		}
		else {
			renderInfo.colorAttachmentCount = 0;
			renderInfo.pColorAttachments = nullptr;
		}

		if (depthAttachment) {
			renderInfo.pDepthAttachment = depthAttachment;
		}
		
		renderInfo.pStencilAttachment = nullptr;

		return renderInfo;
	}
}