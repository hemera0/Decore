#pragma once
#include "../Engine/Core/Camera/Camera.hpp"

// Sample Test Scene.
namespace Engine::Tests
{
	class Test1Renderer : public Renderer::BaseRenderer {
		// User Specific Pass Data.

	public:
		Test1Renderer( Renderer::VulkanRenderer* context ) : BaseRenderer( context ) { }

		// BaseRenderPass Interface.
		virtual void Startup( ) override;
		virtual void Shutdown( ) override;
		virtual void PreRender( ) override;

		virtual void RecreateSwapchainResources() override;
		virtual void Update( Renderer::CommandBuffer* commandBuffer ) override;
	};
}