#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
	CommandBuffer::CommandBuffer(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool) : m_Device(device), m_CommandPool(commandPool) {
		VkCommandBufferAllocateInfo commandBufferAllocInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		commandBufferAllocInfo.commandPool = *m_CommandPool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = 1;

		if (const auto result = vkAllocateCommandBuffers(*m_Device, &commandBufferAllocInfo, &m_CommandBuffer); result != VK_SUCCESS) {
			printf("failed to allocate command buffer: %d\n", result);
		}
	}

	CommandBuffer::~CommandBuffer() {
		vkDeviceWaitIdle(*m_Device);
		vkFreeCommandBuffers(*m_Device, *m_CommandPool, 1, &m_CommandBuffer);
	}

	void CommandBuffer::Begin(VkCommandBufferUsageFlags flags) {
		VkCommandBufferBeginInfo commandBufferBeginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };

		if (flags != 0)
			commandBufferBeginInfo.flags = flags;

		commandBufferBeginInfo.pInheritanceInfo = nullptr; // Optional

		if (const auto result = vkBeginCommandBuffer(m_CommandBuffer, &commandBufferBeginInfo); result != VK_SUCCESS) {
			printf("failed to begin command buffer %d\n", result);
		}
	}

	void CommandBuffer::End() {
		vkEndCommandBuffer(m_CommandBuffer);
	}

	void CommandBuffer::SubmitToQueue(const VkQueue& queue, bool wait) {
		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &m_CommandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);

		if (wait)
			vkQueueWaitIdle(queue);
	}

	void CommandBuffer::CopyBuffer(const VkBuffer& source, const VkBuffer& destination, const VkDeviceSize& size) const {
		VkBufferCopy bufferCopy = {};
		bufferCopy.srcOffset = 0;
		bufferCopy.dstOffset = 0;
		bufferCopy.size = size;

		vkCmdCopyBuffer(m_CommandBuffer, source, destination, 1, &bufferCopy);
	}

	void CommandBuffer::CopyBufferToImage(const VkBuffer& buffer, const VkImage& image, uint32_t width, uint32_t height) const {
		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = { width, height, 1 };

		vkCmdCopyBufferToImage(m_CommandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
	}

	/*
	void CommandBuffer::BeginRenderPass(RenderPass* renderPass, Swapchain* swapChain, const VkFramebuffer& frameBuffer, VkClearValue* clearValues, VkExtent2D* extents) {
		VkClearValue clr[2] = {};

		if (clearValues) {
			clearValues[0].depthStencil.stencil = 0u;
			clearValues[0].depthStencil.depth = 1.f;
		}

		clr[0].color = { {0.1f, 0.1f, 0.1f, 1.0f} };
		clr[1].depthStencil.depth = 1.f;
		clr[1].depthStencil.stencil = 0u;

		VkRenderPassBeginInfo renderPassInfo{ VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		renderPassInfo.renderPass = *renderPass;
		renderPassInfo.framebuffer = frameBuffer;
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = extents ? *extents : swapChain->GetExtents();

		// Todo: Make optional override. 
		renderPassInfo.clearValueCount = clearValues ? 1 : 2;
		renderPassInfo.pClearValues = clearValues ? clearValues : clr;

		vkCmdBeginRenderPass(m_CommandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	}
	*/
	void CommandBuffer::BeginRendering(VkRenderingInfo* renderingInfo) {
		vkCmdBeginRendering(m_CommandBuffer, renderingInfo);
	}

	void CommandBuffer::EndRendering() {
		vkCmdEndRendering(m_CommandBuffer);
	}

	/*
	void CommandBuffer::EndRenderPass() {
		vkCmdEndRenderPass(m_CommandBuffer);
	}
	*/

	void CommandBuffer::SetScissor(uint32_t width, uint32_t height, int32_t x, int32_t y) {
		VkRect2D scissor{};
		scissor.offset = { x, y };
		scissor.extent.width = width;
		scissor.extent.height = height;
		vkCmdSetScissor(m_CommandBuffer, 0, 1, &scissor);
	}

	void CommandBuffer::SetViewport(float width, float height, float x, float y, float minDepth, float maxDepth) {
		VkViewport viewport{};
		viewport.x = x;
		viewport.y = y;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = minDepth;
		viewport.maxDepth = maxDepth;
		vkCmdSetViewport(m_CommandBuffer, 0, 1, &viewport);
	}

	void CommandBuffer::ImageBarrier(VkImage image, VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask, VkImageLayout oldImageLayout, VkImageLayout newImageLayout, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask, VkImageSubresourceRange subresourceRange)
	{
		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldImageLayout;
		barrier.newLayout = newImageLayout;
		barrier.image = image;
		barrier.subresourceRange = subresourceRange;

		vkCmdPipelineBarrier(
			m_CommandBuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);
	}

	void CommandBuffer::BufferBarrier( VkBuffer buffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask )
	{
		VkBufferMemoryBarrier2 result = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2 };

		result.srcStageMask = srcStageMask;
		result.srcAccessMask = srcAccessMask;
		result.dstStageMask = dstStageMask;
		result.dstAccessMask = dstAccessMask;
		result.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		result.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		result.buffer = buffer;
		result.offset = 0;
		result.size = VK_WHOLE_SIZE;

		VkDependencyInfo dependencyInfo = { VK_STRUCTURE_TYPE_DEPENDENCY_INFO };
		dependencyInfo.dependencyFlags = 0;
		dependencyInfo.bufferMemoryBarrierCount = 1u;
		dependencyInfo.pBufferMemoryBarriers = &result;

		vkCmdPipelineBarrier2( m_CommandBuffer, &dependencyInfo );
	}

	void CommandBuffer::BindDescriptors(const std::vector<Descriptor*>& descriptors) {
		std::vector<VkDescriptorBufferBindingInfoEXT> bindingInfos{};

		for (auto i = 0; i < descriptors.size(); i++) {
			auto* desc = descriptors[i];
			if (!desc)
				continue;

			VkDescriptorBufferBindingInfoEXT bindingInfo{};

			bindingInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_BUFFER_BINDING_INFO_EXT;
			bindingInfo.address = desc->GetBuffer()->GetDeviceAddress();
			bindingInfo.usage = desc->GetFlags() | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT;

			bindingInfos.push_back(bindingInfo);
		}

		vkCmdBindDescriptorBuffersEXT(m_CommandBuffer, static_cast<uint32_t>(bindingInfos.size()), bindingInfos.data());
	}

	void CommandBuffer::SetDescriptorOffsets(const std::vector<class Descriptor*>& descriptors, const Pipeline& pipeline) {
		std::vector<uint32_t> setIndices{};
		std::vector<VkDeviceSize> setOffsets{};

		setIndices.resize(descriptors.size());
		setOffsets.resize(descriptors.size());

		for (auto i = 0; i < descriptors.size(); i++) {
			setIndices[i] = i;
			setOffsets[i] = 0;
		}

		vkCmdSetDescriptorBufferOffsetsEXT(
			m_CommandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipeline.GetPipelineLayout(),
			0,
			static_cast<uint32_t>(setIndices.size()),
			setIndices.data(),
			setOffsets.data()
		);
	}

	void CommandBuffer::SetDescriptorOffsets(const std::vector<class Descriptor*>& descriptors, const ComputePipeline& pipeline) {
		std::vector<uint32_t> setIndices{};
		std::vector<VkDeviceSize> setOffsets{};

		setIndices.resize(descriptors.size());
		setOffsets.resize(descriptors.size());

		for (auto i = 0; i < descriptors.size(); i++) {
			setIndices[i] = i;
			setOffsets[i] = 0;
		}

		vkCmdSetDescriptorBufferOffsetsEXT(
			m_CommandBuffer,
			VK_PIPELINE_BIND_POINT_COMPUTE,
			pipeline.GetPipelineLayout(),
			0,
			static_cast<uint32_t>(setIndices.size()),
			setIndices.data(),
			setOffsets.data()
		);
	}
}