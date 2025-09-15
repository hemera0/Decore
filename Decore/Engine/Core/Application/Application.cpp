#include "Application.hpp"

#define VOLK_IMPLEMENATION
#include "../../Renderer/Vulkan/VulkanRenderer.hpp"

#include "../Input/InputSystem.hpp"
#include "../Time/TimeSystem.hpp"

#include "../../../Dependencies/imgui/backends/imgui_impl_glfw.h"

namespace Engine::Core {
	static void FramebufferResizeCallback(GLFWwindow* window, int width, int height) {
		if (Application* application = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window))) {
			if (auto* renderer = application->GetRenderer()) {
				renderer->RecreateSwapChain();
			}
		}
	}

	static void MouseCallback(GLFWwindow* window, double xpos, double ypos) {
		InputSystem::SetMousePos(static_cast<float>(xpos), static_cast<float>(ypos));
	}

	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
		InputSystem::SetKeyState(key, action); 
	}

	bool Application::Startup() {
		if (!glfwInit()) {
			fprintf(stderr, "Failed to initialize GLFW\n");
			return false;
		}

		if (volkInitialize() != VK_SUCCESS) {
			fprintf(stderr, "Failed to initialize Volk\n");
			return false;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		m_Window = glfwCreateWindow(m_Width, m_Height, "Decore", nullptr, nullptr);
		if (!m_Window) {
			fprintf(stderr, "Failed to create window\n");
			return false;
		}

		glfwSetFramebufferSizeCallback(m_Window, &FramebufferResizeCallback);
		glfwSetCursorPosCallback(m_Window, &MouseCallback);
		glfwSetKeyCallback(m_Window, &KeyCallback);
		glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		glfwSetWindowUserPointer(m_Window, this);

		m_Renderer = new Engine::Renderer::VulkanRenderer();

		return true;
	}

	void Application::Shutdown() {
		m_Renderer->Cleanup();

		glfwDestroyWindow(m_Window);

		delete m_Renderer;
	}

	void Application::Update() {
		// This is placed here to allow us a spot to bind our external RenderPasses before the render loop starts.
		uint32_t extensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extensionCount);
		m_Renderer->Setup(glfwExtensions, extensionCount, glfwGetWin32Window(m_Window));

		ImGui_ImplGlfw_InitForOther(m_Window, true);

		float t = 0.f;
		int fc = 0;

		while (!glfwWindowShouldClose(m_Window)) {
			static float oldTime = float(glfwGetTime());
			float dt = float(glfwGetTime()) - oldTime;

			TimeSystem::SetDeltaTime(dt);

			oldTime = float(glfwGetTime());

			t += dt;

			if (t >= 1.f)
			{
				char buf[120] = {};
				sprintf_s(buf, "Decore: %d FPS", fc);
				glfwSetWindowTitle(m_Window, buf);

				t = 0.f;
				fc = 0;
			}

			for (auto& callback : m_UpdateCallbacks)
				callback(this);

			m_Renderer->Render(dt);

			if (InputSystem::GetKeyPressed(GLFW_KEY_INSERT))
			{
				auto inputMode = glfwGetInputMode(m_Window, GLFW_CURSOR);
				glfwSetInputMode(m_Window, GLFW_CURSOR, inputMode == GLFW_CURSOR_DISABLED ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
			}

			InputSystem::OldKeyStates = InputSystem::KeyStates;

			glfwPollEvents();

			fc++;
		}
	}
}