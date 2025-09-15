#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
    Swapchain::Swapchain(std::shared_ptr<Device> device, std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<Surface> surface) :
        m_Device(device), m_PhysicalDevice(physicalDevice), m_Surface(surface) {

        Recreate();
    }

    Swapchain::~Swapchain() { Destroy(); }

    void Swapchain::Recreate() {
        CreateSwapchain();
        CreateImageViews();
        CreateDepthResources();
    }

    void Swapchain::Destroy() {
        vkDeviceWaitIdle(*m_Device);

        for (auto i{ 0u }; i < m_ImageViews.size(); i++) {
            if (m_ImageViews[i]) {
                delete m_ImageViews[i];
                m_ImageViews[i] = nullptr;
            }
        }

        delete m_DepthImage;
        delete m_DepthImageView;

        delete m_MSAAImage;
        delete m_MSAAImageView;

        delete m_PrePassDepthImage;
        delete m_PrePassDepthImageView;

        delete m_HDRImage;
        delete m_HDRImageView;

        m_ImageViews.clear();

        vkDestroySwapchainKHR(*m_Device, m_SwapChain, nullptr);
    }

    VkSurfaceFormatKHR Swapchain::GetSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) const {
        for (const auto& surfaceFormat : availableFormats) {
            if (surfaceFormat.format == VK_FORMAT_B8G8R8A8_UNORM && surfaceFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return surfaceFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR Swapchain::GetPresentMode(const std::vector<VkPresentModeKHR>& availableModes) const {
        for (const auto& mode : availableModes) {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return mode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D Swapchain::GetSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        }
        else {
            RECT area{};
            GetClientRect(m_Surface->GetWindowHandle(), &area);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(area.right),
                static_cast<uint32_t>(area.bottom)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }

    void Swapchain::CreateSwapchain() {
        const auto& swapChainSupport = m_Surface->QuerySwapchainSupport(m_PhysicalDevice);
        if (swapChainSupport.m_PresentModes.empty() || swapChainSupport.m_SurfaceFormats.empty())
        {
            std::printf("Device does not have any present modes or surface formats\n");
            return;
        }

        const auto familyIndices = m_Surface->FindQueueFamilyIndices(m_PhysicalDevice);
        if (!familyIndices.IsValid()) {
            std::printf("Failed to find valid queue families\n");
            return;
        }

        // TODO: Verify we can sample from the swapchain images.
        VkSurfaceFormatKHR surfaceFormat = GetSurfaceFormat(swapChainSupport.m_SurfaceFormats);
        VkPresentModeKHR presentMode = GetPresentMode(swapChainSupport.m_PresentModes);
        VkExtent2D extent = GetSwapExtent(swapChainSupport.m_SurfaceCapabilities);

        uint32_t imageCount = swapChainSupport.m_SurfaceCapabilities.minImageCount + 1;
        if (swapChainSupport.m_SurfaceCapabilities.maxImageCount > 0 && imageCount > swapChainSupport.m_SurfaceCapabilities.maxImageCount) {
            imageCount = swapChainSupport.m_SurfaceCapabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR swapchainCreateInfo{ VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
        swapchainCreateInfo.surface = *m_Surface;
        swapchainCreateInfo.minImageCount = imageCount;
        swapchainCreateInfo.imageFormat = surfaceFormat.format;
        swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
        swapchainCreateInfo.imageExtent = extent;
        swapchainCreateInfo.imageArrayLayers = 1;
        swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;

        uint32_t queueFamilyIndices[] = { familyIndices.m_GraphicsFamilyIndex, familyIndices.m_PresentFamilyIndex };
        if (familyIndices.m_GraphicsFamilyIndex != familyIndices.m_PresentFamilyIndex) {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            swapchainCreateInfo.queueFamilyIndexCount = 2;
            swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else {
            swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
            swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
            swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
        }

        swapchainCreateInfo.preTransform = swapChainSupport.m_SurfaceCapabilities.currentTransform;
        swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        swapchainCreateInfo.presentMode = presentMode;
        swapchainCreateInfo.clipped = VK_TRUE;
        swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

        if (const auto result = vkCreateSwapchainKHR(*m_Device, &swapchainCreateInfo, nullptr, &m_SwapChain); result != VK_SUCCESS) {
            std::printf("Failed to create swapchain %d\n", result);
            return;
        }

        vkGetSwapchainImagesKHR(*m_Device, m_SwapChain, &imageCount, nullptr);

        m_Images.resize(imageCount);
        vkGetSwapchainImagesKHR(*m_Device, m_SwapChain, &imageCount, m_Images.data());

        m_ImageFormat = surfaceFormat.format;
        m_Extents = extent;
        m_MSAASamples = m_PhysicalDevice->GetMaxUsableSampleCount();

        const auto hdrFormat = GetHDRFormat(); // VK_FORMAT_A2B10G10R10_SNORM_PACK32; // VK_FORMAT_R16G16B16A16_SFLOAT

        m_MSAAImage = new Image(m_Device, m_Extents.width, m_Extents.height, 1, hdrFormat, VK_IMAGE_TILING_OPTIMAL, m_MSAASamples,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_MSAAImageView = new ImageView(m_Device, *m_MSAAImage, 1, hdrFormat);

        m_HDRImage = new Image(m_Device, m_Extents.width, m_Extents.height, 1, hdrFormat, VK_IMAGE_TILING_OPTIMAL, VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        m_HDRImageView = new ImageView(m_Device, *m_HDRImage, 1, hdrFormat);
    }

    void Swapchain::CreateImageViews() {
        m_ImageViews.resize(m_Images.size());
        for (auto i{ 0u }; i < m_ImageViews.size(); i++)
            m_ImageViews[i] = new ImageView(m_Device, m_Images[i], 1, m_ImageFormat);
    }

    void Swapchain::CreateDepthResources()
    {
        const auto& extents = GetExtents();
        m_DepthFormat = m_PhysicalDevice->FindDepthFormat();

        const auto createDepthImageAndView =
            [&](Image** image, ImageView** imageView, VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT)
            {
                *image = new Image(
                    m_Device,
                    extents.width,
                    extents.height,
                    1,
                    m_DepthFormat,
                    VK_IMAGE_TILING_OPTIMAL,
                    samples,
                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                );

                Image* depthImage = *image;

                VkImageViewCreateInfo imageViewCreateInfo = ImageView::GetDefault2DCreateInfo(*depthImage, 1, m_DepthFormat);
                imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

                *imageView = new ImageView(m_Device, *depthImage, 1, m_DepthFormat, &imageViewCreateInfo);
            };

        createDepthImageAndView(&m_PrePassDepthImage, &m_PrePassDepthImageView);
        createDepthImageAndView(&m_DepthImage, &m_DepthImageView, m_MSAASamples);

        m_PrePassNormalsImage = new Image(
            m_Device,
            extents.width,
            extents.height,
            1,
            GetImageFormat(),
            VK_IMAGE_TILING_OPTIMAL,
            VK_SAMPLE_COUNT_1_BIT,
            VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
        );

        m_PrePassNormalsImageView =
            new ImageView(m_Device, *m_PrePassNormalsImage, 1, GetImageFormat());
    }
}