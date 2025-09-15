#pragma once
#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

#include <functional>
#include <vector>

// Forward Declare Renderer.
namespace Engine::Renderer { 
	class VulkanRenderer;
	class DebugRenderer;
}

namespace Engine::Core {
	class Application {
		GLFWwindow* m_Window = nullptr;
		Renderer::VulkanRenderer* m_Renderer = nullptr;
		Renderer::DebugRenderer* m_DebugRenderer = nullptr;

		int m_Width{}, m_Height{};

		using OnUpdate_t = std::function<void(Application*)>;

		std::vector<OnUpdate_t> m_UpdateCallbacks{};
	public:
		Application() = default;
		Application(int width, int height) : m_Width(width), m_Height(height) {}

		virtual bool Startup();
		virtual void Shutdown();
		virtual void Update();

		Renderer::VulkanRenderer* GetRenderer() { return m_Renderer; }
		Renderer::DebugRenderer* GetDebugRenderer() { return m_DebugRenderer; }

		void Bind_OnUpdate(OnUpdate_t callback) {
			m_UpdateCallbacks.push_back(callback);
		}
	};
}