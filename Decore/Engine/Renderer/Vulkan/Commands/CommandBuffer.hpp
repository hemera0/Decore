#pragma once
#include <variant>

namespace Engine::Renderer {
	class CommandBuffer {
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;

		std::shared_ptr<Device> m_Device;
		std::shared_ptr<CommandPool> m_CommandPool;
	public:
		CommandBuffer(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool);
		~CommandBuffer();

		void Begin(VkCommandBufferUsageFlags flags = 0);
		void End();
		void SubmitToQueue(const VkQueue& queue, bool wait = true);
		void CopyBuffer(const VkBuffer& source, const VkBuffer& destination, const VkDeviceSize& size) const;
		void CopyBufferToImage(const VkBuffer& buffer, const VkImage& image, uint32_t width, uint32_t height) const;

		void SetScissor(uint32_t width, uint32_t height, int32_t x = 0, int32_t y = 0);
		void SetViewport(float width, float height, float x = 0.f, float y = 0.f, float minDepth = 0.f, float maxDepth = 1.f);

		void ImageBarrier(VkImage image,
			VkAccessFlags srcAccessMask,
			VkAccessFlags dstAccessMask,
			VkImageLayout oldImageLayout,
			VkImageLayout newImageLayout,
			VkPipelineStageFlags srcStageMask,
			VkPipelineStageFlags dstStageMask,
			VkImageSubresourceRange subresourceRange);

		void BufferBarrier( VkBuffer buffer, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask );

		void BeginRendering(VkRenderingInfo* renderingInfo);
		void EndRendering();

		// void BeginRenderPass(RenderPass* renderPass, Swapchain* swapChain, const VkFramebuffer& frameBuffer, VkClearValue* clearValues = nullptr, VkExtent2D* extents = nullptr);
		// void EndRenderPass();

		void BindDescriptors(const std::vector<class Descriptor*>& descriptors);
		void SetDescriptorOffsets(const std::vector<class Descriptor*>& descriptors, const Pipeline& pipeline);
		void SetDescriptorOffsets(const std::vector<class Descriptor*>& descriptors, const ComputePipeline& pipeline);

		DEFINE_IMPLICIT_VK(m_CommandBuffer);
	};

}