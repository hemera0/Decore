#pragma once

namespace Engine::Renderer {
	class CommandBuffer;
	class Descriptor;
	class Pipeline;
}

namespace Engine::Assets {
	class BaseAsset {
	public:
		virtual std::string GetName() const = 0;
		virtual void Render(Renderer::CommandBuffer& commandBuffer, Renderer::Pipeline* pipeline, const std::vector<Renderer::Descriptor*>& sceneDescriptors, int bufferIndex = 0) = 0;
	};
}