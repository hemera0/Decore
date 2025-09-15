#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
    Buffer::Buffer(
        std::shared_ptr<Device> device,
        const VkDeviceSize size,
        const VkBufferUsageFlags usage,
        const VkMemoryPropertyFlags memoryFlags,
        const VkSharingMode sharingMode,
        const VkMemoryAllocateFlags allocateFlags) : m_Device(device), m_Size(size) {
        VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = sharingMode;

        if (vkCreateBuffer(*m_Device, &bufferInfo, nullptr, &m_Handle) != VK_SUCCESS) {
            printf("Failed to create buffer!\n");
            return;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(*m_Device, m_Handle, &memoryRequirements);

        VkMemoryAllocateInfo memoryAllocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        memoryAllocInfo.allocationSize = memoryRequirements.size;
        memoryAllocInfo.memoryTypeIndex = m_Device->GetPhysicalDevice()->FindMemoryType(memoryRequirements.memoryTypeBits, memoryFlags);

        VkMemoryAllocateFlagsInfo flagsInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO };

        if (allocateFlags != 0) {
            flagsInfo.flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT;

            memoryAllocInfo.pNext = &flagsInfo;
        }

        if (vkAllocateMemory(*m_Device, &memoryAllocInfo, nullptr, &m_Memory) != VK_SUCCESS) {
            printf("failed to allocate vertex buffer memory!\n");
            return;
        }

        vkBindBufferMemory(*m_Device, m_Handle, m_Memory, 0);
    }

    Buffer::~Buffer() {
        vkDestroyBuffer(*m_Device, m_Handle, nullptr);
        vkFreeMemory(*m_Device, m_Memory, nullptr);
    }

    void* Buffer::Map() {
        void* data = nullptr;
        vkMapMemory(*m_Device, m_Memory, 0, VK_WHOLE_SIZE, 0, &data);
        return data;
    }

    void Buffer::Unmap() {
        vkUnmapMemory(*m_Device, m_Memory);
    }

    void Buffer::Patch(void* data, size_t size) {
        void* ptr = Map();
        memcpy(ptr, data, size);
        Unmap();
    }

    const VkDeviceAddress& Buffer::GetDeviceAddress() const {
        VkBufferDeviceAddressInfo deviceAddrInfo{ VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO };
        deviceAddrInfo.buffer = GetHandle();
        return vkGetBufferDeviceAddress(*m_Device, &deviceAddrInfo);
    }

    StagingBuffer::StagingBuffer(std::shared_ptr<Device> device, const VkDeviceSize size) :
        Buffer(device, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, VK_SHARING_MODE_EXCLUSIVE) {
    }
}