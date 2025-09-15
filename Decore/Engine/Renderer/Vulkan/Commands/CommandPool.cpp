#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
    CommandPool::CommandPool(const Device& device) : m_Device(device) {
        const auto& queueFamilyIndices = m_Device.GetQueueFamilyIndices();

        VkCommandPoolCreateInfo pool_create_info{};
        pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        pool_create_info.queueFamilyIndex = queueFamilyIndices.m_GraphicsFamilyIndex;

        if (const auto result = vkCreateCommandPool(m_Device, &pool_create_info, nullptr, &m_CommandPool); result != VK_SUCCESS) {
            printf("Failed to create CommandPool: %d\n", result);
        }
    }

    CommandPool::~CommandPool() {
        vkDeviceWaitIdle(m_Device);
        vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
    }
}