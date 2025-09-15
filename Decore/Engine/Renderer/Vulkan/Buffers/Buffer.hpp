#pragma once

namespace Engine::Renderer {
	class Buffer {
		std::shared_ptr<Device> m_Device = nullptr;

		VkBuffer m_Handle = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;

		VkDeviceSize m_Size{};
	public:
		Buffer(std::shared_ptr<Device> device, const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags memoryFlags, const VkSharingMode sharingMode, const VkMemoryAllocateFlags allocateFlags = 0);
		~Buffer();

		void* Map();
		void Unmap();

		void Patch(void* data, size_t size);

		const VkDeviceMemory& GetMemory() const { return m_Memory; }
		const VkBuffer& GetHandle() const { return m_Handle; }
		const VkDeviceSize GetSize() const { return m_Size; }
		const VkDeviceAddress& GetDeviceAddress() const;

		DEFINE_IMPLICIT_VK(m_Handle);
	};

	class StagingBuffer : public Buffer {
	public:
		StagingBuffer(std::shared_ptr<Device> device, const VkDeviceSize size);
	};
}