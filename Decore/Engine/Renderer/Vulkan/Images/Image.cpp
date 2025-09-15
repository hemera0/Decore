#include "../VulkanRenderer.hpp"
#include "../../../Assets/Assets.hpp"

#include <ktxvulkan.h>
#pragma comment(lib, "ktx.lib")

namespace Engine::Renderer {
	Image::Image(
		std::shared_ptr<Device> device,
		uint32_t width,
		uint32_t height,
		uint32_t mipLevels,
		VkFormat format,
		VkImageTiling tiling,
		VkSampleCountFlagBits sampleCount,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties, 
		VkImageCreateInfo* optionalCI
	)
		
		: m_Device( device ), m_Width(width), m_Height(height)
	{
		VkImageCreateInfo imageInfo = optionalCI ? *optionalCI : Image::GetDefault2DCreateInfo( width, height, mipLevels, format, tiling, sampleCount, usage );
		if ( vkCreateImage( *m_Device, &imageInfo, nullptr, &m_Image ) != VK_SUCCESS )
		{
			printf( "Failed to create image\n" );
		}

		VkMemoryRequirements memRequirements{};
		vkGetImageMemoryRequirements( *m_Device, m_Image, &memRequirements );

		VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = m_Device->GetPhysicalDevice( )->FindMemoryType( memRequirements.memoryTypeBits, properties );

		if ( const auto res = vkAllocateMemory(*m_Device, &allocInfo, nullptr, &m_ImageMemory ); res != VK_SUCCESS )
		{
			printf( "Failed to allocate image memory: %d\n", res );
		}

		vkBindImageMemory(*m_Device, m_Image, m_ImageMemory, 0 );
	}

	Image::~Image() {
		vkDeviceWaitIdle(*m_Device);
		vkDestroyImage(*m_Device, m_Image, nullptr);
		vkFreeMemory(*m_Device, m_ImageMemory, nullptr);
	}

	Image* Image::LoadFromFile(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::string& path, Buffer** imageBuffer )
	{
		int width{}, height{}, texChannels;
		stbi_uc* pixels = stbi_load( path.c_str( ), &width, &height, &texChannels, STBI_rgb_alpha );
		if ( !pixels )
		{
			return nullptr;
		}

		uint32_t mipLevels = static_cast< uint32_t >( std::floor( std::log2( std::max( width, height ) ) ) ) + 1;

		VkDeviceSize imageSize = width * height * 4;
		std::unique_ptr<StagingBuffer> stagingBuffer = std::make_unique<StagingBuffer>( device, imageSize );

		void* data = stagingBuffer->Map( );
		memcpy( data, pixels, static_cast< size_t >( imageSize ) );
		stagingBuffer->Unmap( );

		stbi_image_free( pixels );

		// This is for if we want to store the image contents in a buffer. (Untested right now.)
		if ( imageBuffer )
		{
			*imageBuffer = new Buffer( device, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
									   VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE );

			{
				std::unique_ptr<CommandBuffer> commandBuffer = std::make_unique<CommandBuffer>( device, commandPool );

				const auto& graphicsQueue = device->GetGraphicsQueue( );

				commandBuffer->Begin( );
				commandBuffer->CopyBuffer( *stagingBuffer, **imageBuffer, static_cast< VkDeviceSize >( imageSize ) );
				commandBuffer->End( );
				commandBuffer->SubmitToQueue( graphicsQueue );
			}
		}

		VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;

		Image* image = new Image( device,
								  width,
								  height,
								  mipLevels,
								  format,
								  VK_IMAGE_TILING_OPTIMAL,
								  VK_SAMPLE_COUNT_1_BIT,
								  VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
								  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		image->m_LevelCount = mipLevels;

		// Vulkan tutorial talks about using transition layout to go to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
		// But we can just set this for the VkImageView in the Descriptor.
		image->TransitionLayout( commandPool, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL );

		// Copy image data from staging buffer to the VkImage.
		{
			std::unique_ptr<CommandBuffer> commandBuffer = std::make_unique<CommandBuffer>( device, commandPool );

			const auto& graphicsQueue = device->GetGraphicsQueue( );

			commandBuffer->Begin( );
			commandBuffer->CopyBufferToImage( *stagingBuffer, *image, width, height );
			commandBuffer->End( );
			commandBuffer->SubmitToQueue( graphicsQueue );
		}

		image->GenerateMipmaps( commandPool, width, height, mipLevels );
		image->m_Format = format;

		return image;
	}

	Image* Image::LoadFromAsset(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, tinygltf::Image& sourceImage, Buffer** imageBuffer) {
		// VkDeviceSize imageSize = sourceImage.width * sourceImage.height * sourceImage.component;

		std::unique_ptr<StagingBuffer> stagingBuffer = std::make_unique<StagingBuffer>(device, sourceImage.image.size());

		//if (sourceImage.image.size() != imageSize)
		//{
		//	printf("ImageAsset size mismatch!\n");
		//	return nullptr;
		//}

		stagingBuffer->Patch(reinterpret_cast<void*>(sourceImage.image.data()), sourceImage.image.size());

		// This is for if we want to store the image contents in a buffer. (Untested right now.)
		/*if (imageBuffer) {
			*imageBuffer = new Buffer(device, imageSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE);

			{
				std::unique_ptr<CommandBuffer> commandBuffer = std::make_unique<CommandBuffer>(device, commandPool);

				const auto& graphicsQueue = device.GetGraphicsQueue();

				commandBuffer->Begin();
				commandBuffer->CopyBuffer(*stagingBuffer, **imageBuffer, static_cast<VkDeviceSize>(imageSize));
				commandBuffer->End();
				commandBuffer->SubmitToQueue(graphicsQueue);
			}
		}
		*/

		VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
		if (sourceImage.bits == 16)
			format = VK_FORMAT_R16G16B16A16_SFLOAT;
		if (sourceImage.bits == 32)
			format = VK_FORMAT_R32G32B32A32_SFLOAT;

		uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(sourceImage.width, sourceImage.height)))) + 1;

		Image* image = new Image(
			device, 
			sourceImage.width, 
			sourceImage.height, 
			mipLevels,
			format,
			VK_IMAGE_TILING_OPTIMAL,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		image->m_LevelCount = mipLevels;

		// Vulkan tutorial talks about using transition layout to go to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL.
		// But we can just set this for the VkImageView in the Descriptor.
		image->TransitionLayout(commandPool, format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		// Copy image data from staging buffer to the VkImage.
		{
			std::unique_ptr<CommandBuffer> commandBuffer = std::make_unique<CommandBuffer>(device, commandPool);

			const auto& graphicsQueue = device->GetGraphicsQueue();

			commandBuffer->Begin();
			commandBuffer->CopyBufferToImage(*stagingBuffer, *image, sourceImage.width, sourceImage.height);
			commandBuffer->End();
			commandBuffer->SubmitToQueue(graphicsQueue);
		}

		image->GenerateMipmaps(commandPool, sourceImage.width, sourceImage.height, mipLevels);
		image->m_Format = format;

		return image;
	}

	Image* Image::LoadKTXFromFile(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::string& path, Buffer** imageBuffer) {
		const auto& physicalDevice = device->GetPhysicalDevice();
		const auto& graphicsQueue = device->GetGraphicsQueue();

		ktxVulkanDeviceInfo deviceInfo = {};

		ktx_error_code_e result = ktxVulkanDeviceInfo_Construct(&deviceInfo, *physicalDevice, *device, graphicsQueue, *commandPool, nullptr);
		if (result != ktx_error_code_e::KTX_SUCCESS)
			return nullptr;

		ktxTexture* kTexture = nullptr;
		result = ktxTexture_CreateFromNamedFile(path.c_str(), KTX_TEXTURE_CREATE_NO_FLAGS, &kTexture);
		if (result != ktx_error_code_e::KTX_SUCCESS)
			return nullptr;

		// This gets mad if you don't encode/save the .ktx file in a format Vulkan likes
		ktxVulkanTexture texture = {};
		result = ktxTexture_VkUploadEx(
			kTexture,
			&deviceInfo,
			&texture,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		);

		if (result != ktx_error_code_e::KTX_SUCCESS)
			return nullptr;

		// After loading all textures you don't need these anymore
		ktxTexture_Destroy(kTexture);
		ktxVulkanDeviceInfo_Destruct(&deviceInfo);

		Image* resultImage = new Image(device, texture.image, texture.deviceMemory);

		resultImage->m_Format = texture.imageFormat;
		resultImage->m_ViewType = texture.viewType;
		resultImage->m_LayerCount = texture.layerCount;
		resultImage->m_LevelCount = texture.levelCount;

		return resultImage;
	}

	void Image::TransitionLayout(std::shared_ptr<CommandPool> commandPool, const VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout) {
		const auto& graphicsQueue = m_Device->GetGraphicsQueue();

		std::unique_ptr<CommandBuffer> commandBuffer = std::make_unique<CommandBuffer>(m_Device, commandPool);

		commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = m_Image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = m_LevelCount;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage{}, destinationStage{};

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else {
			throw std::invalid_argument("Unsupported layout transition!");
		}

		vkCmdPipelineBarrier(*commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		commandBuffer->End();
		commandBuffer->SubmitToQueue(graphicsQueue);
	}

	void Image::GenerateMipmaps(std::shared_ptr<CommandPool> commandPool, uint32_t width, uint32_t height, uint32_t mipLevels) {
		const auto& physicalDevice = m_Device->GetPhysicalDevice();
		const auto& graphicsQueue = m_Device->GetGraphicsQueue();

		std::unique_ptr<CommandBuffer> commandBuffer = std::make_unique<CommandBuffer>(m_Device, commandPool);

		commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);

		// Generate mipmaps.
		VkImageMemoryBarrier imageBarrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		imageBarrier.image = m_Image;
		imageBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		imageBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBarrier.subresourceRange.baseArrayLayer = 0;
		imageBarrier.subresourceRange.layerCount = 1;
		imageBarrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = width;
		int32_t mipHeight = height;

		for (uint32_t i = 1u; i < mipLevels; i++) {
			imageBarrier.subresourceRange.baseMipLevel = i - 1;
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr, 0, nullptr, 1, &imageBarrier);

			VkImageBlit imageBlit = {};
			imageBlit.srcSubresource.mipLevel = i - 1;
			imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.srcSubresource.baseArrayLayer = 0;
			imageBlit.srcSubresource.layerCount = 1;
			imageBlit.srcOffsets[0] = { 0, 0, 0 };
			imageBlit.srcOffsets[1] = { mipWidth, mipHeight, 1 };

			imageBlit.dstSubresource.mipLevel = i;
			imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			imageBlit.dstSubresource.baseArrayLayer = 0;
			imageBlit.dstSubresource.layerCount = 1;
			imageBlit.dstOffsets[0] = imageBlit.srcOffsets[0];
			imageBlit.dstOffsets[1] = { mipWidth > 1 ? (mipWidth >> 1) : 1 , mipHeight > 1 ? (mipHeight >> 1) : 1, 1 };

			//Blit the texture
			vkCmdBlitImage(*commandBuffer, m_Image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, m_Image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &imageBlit, VK_FILTER_LINEAR);

			//Set the layout and Access Mask (again) for the shader to read
			imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			//Transfer the layout and Access Mask Information (Again, based on above values)
			vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

			//Reduce the Mip level down by 1 level [By cutting width and height in half]
			if (mipWidth > 1) { mipWidth >>= 1; }
			if (mipHeight > 1) { mipHeight >>= 1; }
		}

		imageBarrier.subresourceRange.baseMipLevel = mipLevels - 1;
		imageBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		imageBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		imageBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(*commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &imageBarrier);

		commandBuffer->End();
		commandBuffer->SubmitToQueue(graphicsQueue);
	}

	VkImageCreateInfo Image::GetDefault2DCreateInfo(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkSampleCountFlagBits sampleCount, VkImageUsageFlags usage) {
		VkImageCreateInfo res{ VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
		res.imageType = VK_IMAGE_TYPE_2D;
		res.extent.width = width;
		res.extent.height = height;
		res.extent.depth = 1;
		res.mipLevels = mipLevels;
		res.arrayLayers = 1;
		res.format = format;
		res.tiling = tiling;
		res.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		res.usage = usage;
		res.samples = sampleCount;
		res.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		return res;
	}
}