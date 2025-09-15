#pragma once

namespace Engine::Renderer {
	class BaseRenderer {
	protected:
		VulkanRenderer* m_Context{};
	public:
		BaseRenderer(VulkanRenderer* context) : m_Context(context) {}

		virtual void Startup() = 0;
		virtual void Shutdown() = 0;
		virtual void PreRender() = 0;

		virtual void RecreateSwapchainResources() = 0;
		virtual void Update(CommandBuffer* commandBuffer) = 0;
	};
}