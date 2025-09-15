#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
    PhysicalDevice::PhysicalDevice(const Instance& instance, const std::vector<const char*>& wantedExts) : m_Instance(instance), m_WantedExtensions(wantedExts) {
        m_PhysicalDevice = SelectDevice();

        if (m_PhysicalDevice == VK_NULL_HANDLE) {
            fprintf(stderr, "Failed to find physical device!\n");
        }

        GetDeviceProperties();
    }

    void PhysicalDevice::GetDeviceProperties() {
        VkPhysicalDeviceProperties2 deviceProperties{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2 };
        m_DescriptorBufferProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_PROPERTIES_EXT;
        deviceProperties.pNext = &m_DescriptorBufferProperties;
        vkGetPhysicalDeviceProperties2(m_PhysicalDevice, &deviceProperties);
    }

    bool PhysicalDevice::IsDedicatedGPU(const VkPhysicalDevice& device) {
        VkPhysicalDeviceProperties deviceProps{};
        vkGetPhysicalDeviceProperties(device, &deviceProps);
        return deviceProps.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    }

    bool PhysicalDevice::HasExtensions(const VkPhysicalDevice& device) {
        uint32_t extensionCount{};
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        uint32_t foundExts{};

        for (const char* wantedExt : m_WantedExtensions) {
            for (const VkExtensionProperties& ext : availableExtensions) {
                if (strcmp(ext.extensionName, wantedExt) == 0) {
                    foundExts++;
                }
            }
        }

        printf("Found %d extensions\n", foundExts);

        return static_cast<uint32_t>(m_WantedExtensions.size()) == foundExts;
    }

    VkPhysicalDevice PhysicalDevice::SelectDevice() {
        uint32_t deviceCount{};

        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            fprintf(stderr, "Failed to find vulkan enabled GPUs\n");
            return VK_NULL_HANDLE;
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        if (deviceCount == 0)
        {
            fprintf(stderr, "Found no devices\n");
            return VK_NULL_HANDLE;
        }

        std::printf("Found %d physical devices\n", deviceCount);

        for (const VkPhysicalDevice& device : devices) {
            if (!IsDedicatedGPU(device) || !HasExtensions(device))
                continue;

            VkPhysicalDeviceProperties deviceProps{};
            vkGetPhysicalDeviceProperties(device, &deviceProps);
            fprintf(stderr, "Using %s\n", deviceProps.deviceName);

            return device;
        }

        return VK_NULL_HANDLE;
    }

    uint32_t PhysicalDevice::FindMemoryType(const uint32_t filter, const VkMemoryPropertyFlags propertyFlags) const {
        VkPhysicalDeviceMemoryProperties memProperties{};
        vkGetPhysicalDeviceMemoryProperties(m_PhysicalDevice, &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if (filter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
                return i;
            }
        }

        return 0u;
    }

    VkFormat PhysicalDevice::FindDepthFormat() const {
        std::array<VkFormat, 3> wantedFormats = {
            VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
        };

        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkFormatFeatureFlags features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

        for (VkFormat format : wantedFormats) {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(m_PhysicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        printf("Failed to find depth format\n");

        return {};
    }

    const VkSampleCountFlagBits PhysicalDevice::GetMaxUsableSampleCount() const {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }
}