#pragma once
#include <filesystem>

#include "../Vulkan/VulkanRenderer.hpp"

namespace Engine::Renderer {
	class EnvironmentInfo {
	public:
		EnvironmentInfo(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::string& envDirectory);
		~EnvironmentInfo();

		Descriptor* GetDescriptor() const { return m_EnvDescriptor; }
		Pipeline* GetSkyboxPipeline() const { return m_SkyboxPipeline; }
	private:
		void Setup(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::string& envDirectory);

		Descriptor* m_EnvDescriptor = nullptr;
		Sampler* m_EnvSampler = nullptr;
		Pipeline* m_SkyboxPipeline = nullptr;

		using TexturePair = std::pair<Image*, ImageView*>;
		std::vector<TexturePair> m_Textures{};
	};
}