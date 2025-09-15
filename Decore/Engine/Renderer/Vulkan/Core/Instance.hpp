#pragma once

namespace Engine::Renderer {
	class Instance {
	private:
		VkInstance instance = VK_NULL_HANDLE;
	public:
		Instance(const char** requiredExtensions = nullptr, uint32_t requiredExtensionsCount = 0);
		~Instance();

		DEFINE_IMPLICIT_VK(instance);
	};
}