#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
	Sampler::Sampler(std::shared_ptr<Device> device, VkSamplerAddressMode addressMode, VkFilter filter, float anisotropy, float minLod, float maxLod) : m_Device(device) {
		VkSamplerCreateInfo samplerInfo = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		samplerInfo.flags = 0;
		samplerInfo.addressModeU = addressMode;
		samplerInfo.addressModeV = addressMode;
		samplerInfo.addressModeW = addressMode;
		samplerInfo.minFilter = filter;
		samplerInfo.magFilter = filter;
		samplerInfo.mipmapMode = (filter == VK_FILTER_NEAREST) ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0;
		samplerInfo.minLod = minLod;
		samplerInfo.maxLod = maxLod;
		samplerInfo.anisotropyEnable = (anisotropy >= 1.0f) ? VK_TRUE : VK_FALSE;
		samplerInfo.maxAnisotropy = anisotropy;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_LESS;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.pNext = nullptr;

		vkCreateSampler(*m_Device, &samplerInfo, nullptr, &m_Sampler);
	}

	Sampler::~Sampler() {
		vkDestroySampler(*m_Device, m_Sampler, nullptr);
	}
}