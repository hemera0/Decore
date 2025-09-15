#pragma once

namespace Engine::Renderer {
	class Swapchain {
	private:
		void CreateSwapchain();
		void CreateImageViews();
		
		void CreateDepthResources();

		VkSurfaceFormatKHR GetSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& available_formats) const;
		VkPresentModeKHR GetPresentMode(const std::vector<VkPresentModeKHR>& available_present_modes) const;
		VkExtent2D GetSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

		std::shared_ptr<Device> m_Device;
		std::shared_ptr<PhysicalDevice> m_PhysicalDevice;
		std::shared_ptr<Surface> m_Surface;

		std::vector<VkImage> m_Images{};
		std::vector<ImageView*> m_ImageViews{};

	public: // TODO: Remove public modifier.
		Image* m_DepthImage = nullptr;
		ImageView* m_DepthImageView = nullptr;

		Image* m_PrePassDepthImage = nullptr;
		ImageView* m_PrePassDepthImageView = nullptr;

		Image* m_PrePassNormalsImage = nullptr;
		ImageView* m_PrePassNormalsImageView = nullptr;

		Image* m_MSAAImage = nullptr;
		ImageView* m_MSAAImageView = nullptr;

		Image* m_HDRImage = nullptr;
		ImageView* m_HDRImageView = nullptr;
	private:
		VkFormat m_ImageFormat{}, m_DepthFormat{};
		VkExtent2D m_Extents{};
		VkSampleCountFlagBits m_MSAASamples{};

		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
	public:
		Swapchain(std::shared_ptr<Device> logicalDevice, std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<Surface> surface);
		~Swapchain();

		void Recreate();
		void Destroy();

		const std::shared_ptr<Device> GetDevice() const { return m_Device; }

		const std::vector<VkImage>& GetImages() const { return m_Images; }
		const std::vector<ImageView*>& GetImageViews() const { return m_ImageViews; }

		const VkFormat GetDepthFormat() const { return m_DepthFormat; }
		const VkFormat GetImageFormat() const { return m_ImageFormat; }
		const VkFormat GetHDRFormat() const { return VK_FORMAT_B10G11R11_UFLOAT_PACK32; }
		
		const VkExtent2D& GetExtents() const { return m_Extents; }
		const VkSampleCountFlagBits GetMSAASamples() const { return m_MSAASamples; }
		
		DEFINE_IMPLICIT_VK(m_SwapChain);
	};
}