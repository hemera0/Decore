#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
    Device::Device(std::shared_ptr<Instance> instance, std::shared_ptr<PhysicalDevice> physicalDevice, std::shared_ptr<Surface> surface) :
        m_Instance(instance), m_PhysicalDevice(physicalDevice), m_Surface(surface) {
        CreateDevice();
    }

    Device::~Device() {
        vkDeviceWaitIdle(m_Device);
        vkDestroyDevice(m_Device, nullptr);
    }

    void Device::CreateDevice() {
        const auto& swapChainSupport = m_Surface->QuerySwapchainSupport(m_PhysicalDevice);
        if (swapChainSupport.m_PresentModes.empty() || swapChainSupport.m_SurfaceFormats.empty())
        {
            std::printf("Device does not have any present modes or surface formats\n");
            return;
        }

        m_QueueFamilyIndices = m_Surface->FindQueueFamilyIndices(m_PhysicalDevice);
        if (!m_QueueFamilyIndices.IsValid()) {
            std::printf("Failed to find valid queue families\n");
            return;
        }

        float queuePriority = 1.0f;

        VkPhysicalDeviceFeatures deviceFeatures{};
        
        VkPhysicalDeviceVulkan12Features testFeatures12{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };

        VkPhysicalDeviceFeatures2 deviceFeatures2{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2 };
        deviceFeatures2.pNext = &testFeatures12;

        vkGetPhysicalDeviceFeatures2(*m_PhysicalDevice, &deviceFeatures2);

        if (deviceFeatures2.features.samplerAnisotropy) {
            printf("Device Supports SamplerAnisotropy\n");
            deviceFeatures.samplerAnisotropy = VK_TRUE;
        }

        if (deviceFeatures2.features.multiDrawIndirect) {
            printf("Device Supports MultiDrawIndirect\n");
            deviceFeatures.multiDrawIndirect = VK_TRUE;
        }

        if (deviceFeatures2.features.depthClamp) {
            printf("Device Supports DepthClamp\n");
            deviceFeatures.depthClamp = VK_TRUE;
        }

        if (deviceFeatures2.features.drawIndirectFirstInstance) {
            printf("Device Supports DrawIndirectFirstInstance\n");
            deviceFeatures.drawIndirectFirstInstance = VK_TRUE;
        }

        bool fidelityFXSupport =
            testFeatures12.shaderStorageBufferArrayNonUniformIndexing  &&
            testFeatures12.shaderFloat16 &&
            deviceFeatures2.features.shaderImageGatherExtended;

        if (fidelityFXSupport) {
            deviceFeatures.shaderStorageImageMultisample = VK_TRUE;
            deviceFeatures.shaderImageGatherExtended = VK_TRUE;
            deviceFeatures.shaderInt16 = VK_TRUE;
        }

        VkDeviceCreateInfo deviceCreateInfo{ VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
        deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

        const auto& extensions = m_PhysicalDevice->GetExtensions();

        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        deviceCreateInfo.ppEnabledExtensionNames = extensions.data();

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = { m_QueueFamilyIndices.m_GraphicsFamilyIndex, m_QueueFamilyIndices.m_PresentFamilyIndex, m_QueueFamilyIndices.m_ComputeFamilyIndex };

        for (const uint32_t familyIndex : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = familyIndex;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        deviceCreateInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();

        // Shouldn't matter as this should match the VkInstance layers
        deviceCreateInfo.enabledLayerCount = 0;

        // Implement 1.4 features.
        VkPhysicalDeviceVulkan14Features features14 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES };
        features14.maintenance5 = VK_TRUE;
       
        // Implement 1.3 features.
        VkPhysicalDeviceVulkan13Features features13 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
        features13.dynamicRendering = VK_TRUE;
        features13.synchronization2 = VK_TRUE;
        
        // Implement 1.2 features.
        VkPhysicalDeviceVulkan12Features features12 = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };

        if (fidelityFXSupport) {
            features12.shaderFloat16 = testFeatures12.shaderFloat16;
            features12.shaderStorageBufferArrayNonUniformIndexing = testFeatures12.shaderStorageBufferArrayNonUniformIndexing;
        }

        features12.descriptorIndexing = VK_TRUE;
        features12.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
        features12.shaderStorageBufferArrayNonUniformIndexing = VK_TRUE;
        features12.shaderUniformBufferArrayNonUniformIndexing = VK_TRUE;
        features12.runtimeDescriptorArray = VK_TRUE;
        features12.descriptorBindingVariableDescriptorCount = VK_TRUE;
        features12.descriptorBindingPartiallyBound = VK_TRUE;

        // Buffer Device Addresses.
        features12.bufferDeviceAddress = VK_TRUE;

        features13.pNext = &features12;

        // Implement Descriptor Buffer.
        VkPhysicalDeviceDescriptorBufferFeaturesEXT featuresDescriptorBuffer = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_BUFFER_FEATURES_EXT };
        featuresDescriptorBuffer.descriptorBuffer = VK_TRUE;
        featuresDescriptorBuffer.pNext = &features13;
        

        features14.pNext = &featuresDescriptorBuffer;
        deviceCreateInfo.pNext = &features14;

        if (const auto result = vkCreateDevice(*m_PhysicalDevice, &deviceCreateInfo, nullptr, &m_Device); result != VK_SUCCESS) {
            fprintf(stderr, "Failed to create logical device: %d\n", result);
        }

        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.m_GraphicsFamilyIndex, 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.m_PresentFamilyIndex, 0, &m_PresentQueue);
        vkGetDeviceQueue(m_Device, m_QueueFamilyIndices.m_ComputeFamilyIndex, 0, &m_ComputeQueue);

        std::printf("LogicalDevice: 0x%p\n", m_Device);
        std::printf("GfxQueue: 0x%p, PresentQueue: 0x%p, ComputeQueue: 0x%p\n", m_GraphicsQueue, m_PresentQueue, m_ComputeQueue);
    }
}