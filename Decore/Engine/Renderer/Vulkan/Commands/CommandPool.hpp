#pragma once

namespace Engine::Renderer {
	class CommandPool {
		VkCommandPool m_CommandPool = VK_NULL_HANDLE;

		const Device& m_Device;
	public:
		CommandPool(const Device& device);
		~CommandPool();

		DEFINE_IMPLICIT_VK(m_CommandPool);
	};
}