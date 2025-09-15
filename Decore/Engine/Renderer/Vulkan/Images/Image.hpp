#pragma once

namespace tinygltf {
	struct Image;
}

namespace Engine::Renderer {
	class Buffer;

	class Image {
		VkImage m_Image = VK_NULL_HANDLE;
		VkDeviceMemory m_ImageMemory = VK_NULL_HANDLE;

		VkFormat m_Format{};
		VkImageViewType m_ViewType{};
		// Mainly just for the KTX textures.
		uint32_t m_LayerCount{}, m_LevelCount{};

		uint32_t m_Width{}, m_Height{};

		std::shared_ptr<Device> m_Device;
	public:
		Image(std::shared_ptr<Device> device, const VkImage& image, const VkDeviceMemory& imageMemory) : m_Device(device), m_Image(image), m_ImageMemory(imageMemory) {}
		Image(std::shared_ptr<Device> device, uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkSampleCountFlagBits sampleCount, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageCreateInfo* optionalCI = nullptr);
		~Image();

		static VkImageCreateInfo GetDefault2DCreateInfo(uint32_t width, uint32_t height, uint32_t mipLevels, VkFormat format, VkImageTiling tiling, VkSampleCountFlagBits sampleCount, VkImageUsageFlags usage);

		static Image* LoadFromFile(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::string& path, Buffer** imageBuffer);
		static Image* LoadKTXFromFile(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::string& path, Buffer** imageBuffer);
		static Image* LoadFromAsset(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, tinygltf::Image& sourceImage, Buffer** imageBuffer);

		void TransitionLayout(std::shared_ptr<CommandPool> commandPool, const VkFormat format, const VkImageLayout oldLayout, const VkImageLayout newLayout);
		void GenerateMipmaps(std::shared_ptr<CommandPool> commandPool, uint32_t width, uint32_t height, uint32_t mipLevels);

		const VkFormat GetFormat() const { return m_Format; }

		const uint32_t GetLayerCount() const { return m_LayerCount; }
		const uint32_t GetLevelCount() const { return m_LevelCount; }
		const VkImageViewType GetViewType() const { return m_ViewType; }

		const uint32_t GetWidth() const { return m_Width; }
		const uint32_t GetHeight() const { return m_Height; }


		DEFINE_IMPLICIT_VK(m_Image);
	};
}