#pragma once

namespace Engine::Renderer {
	class Sampler {
		VkSampler m_Sampler = VK_NULL_HANDLE;

		std::shared_ptr<Device> m_Device;
	public:
		Sampler(std::shared_ptr<Device> device, VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VkFilter filter = VK_FILTER_LINEAR, float anisotropy = 4.f, float minLod = 0.f, float maxLod = VK_LOD_CLAMP_NONE);
		~Sampler();

		DEFINE_IMPLICIT_VK(m_Sampler);
	};
}