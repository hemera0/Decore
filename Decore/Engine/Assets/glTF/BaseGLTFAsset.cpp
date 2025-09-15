#include "../../Renderer/Vulkan/VulkanRenderer.hpp"

// This is a shit spot to put this I think.
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "BaseGLTFAsset.hpp"

namespace Engine::Assets {
	void BaseGLTFAsset::LoadTextures(std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool, const Renderer::DescriptorLayout& textureLayout)
	{
		m_TextureBufferDescriptor = new Renderer::Descriptor(
			device,
			textureLayout,
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		if (m_LoadedModel.textures.empty())
			return;

		m_Textures.resize(m_LoadedModel.textures.size());

		for (auto i = 0; i < m_LoadedModel.textures.size(); i++)
		{
			const auto& texture = m_LoadedModel.textures[i];
			auto& textureInfo = m_Textures[i];

			// TODO: Add a default texture maybe?
			if (texture.source < -1)
				continue;

			auto& textureImage = m_LoadedModel.images[texture.source];

			VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
			if (textureImage.bits == 16)
				format = VK_FORMAT_R16G16B16A16_SFLOAT;
			if (textureImage.bits == 32)
				format = VK_FORMAT_R32G32B32A32_SFLOAT;

			textureInfo.m_Image = Renderer::Image::LoadFromAsset(device, commandPool, textureImage, nullptr);
			textureInfo.m_ImageView = new Renderer::ImageView(device, *textureInfo.m_Image, textureInfo.m_Image->GetLevelCount(), format);
		}

		m_DefaultTextureSampler = new Renderer::Sampler(device, VK_SAMPLER_ADDRESS_MODE_REPEAT);

		std::vector<Renderer::Descriptor::BindingInfo> bindingInfos{};
		bindingInfos.resize(m_Textures.size());

		for (auto i = 0; i < m_Textures.size(); i++)
		{
			auto& textureInfo = m_Textures[i];

			bindingInfos[i] = Renderer::Descriptor::BindingInfo{
				VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				{.m_ImageView = textureInfo.m_ImageView },
				m_DefaultTextureSampler
			};
		}

		if (!bindingInfos.empty())
			m_TextureBufferDescriptor->Bind(bindingInfos);

		m_Textures = {};
	}

	void BaseGLTFAsset::LoadMaterials(std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool, const Renderer::DescriptorLayout& materialLayout)
	{
		m_MaterialBufferDescriptor = new Renderer::Descriptor(
			device,
			materialLayout,
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		if (m_LoadedModel.materials.empty())
			return;

		m_Materials.resize(m_LoadedModel.materials.size());

		for (auto i = 0; i < m_LoadedModel.materials.size(); i++)
		{
			const auto& srcMaterial = m_LoadedModel.materials[i];
			auto& dstMaterial = m_Materials[i];

			if (srcMaterial.alphaMode == "OPAQUE")
				dstMaterial.m_AlphaMode = AlphaMode::AM_OPAQUE;
			if (srcMaterial.alphaMode == "BLEND")
				dstMaterial.m_AlphaMode = AlphaMode::AM_BLEND;
			if (srcMaterial.alphaMode == "MASK")
				dstMaterial.m_AlphaMode = AlphaMode::AM_MASK;

			const auto& pbrMetallicRoughness = srcMaterial.pbrMetallicRoughness;
			const auto& emissiveFactor = srcMaterial.emissiveFactor;

			const auto& baseColorFactor = pbrMetallicRoughness.baseColorFactor;

			dstMaterial.m_BaseColor = RGBAColor_t(baseColorFactor[0], baseColorFactor[1], baseColorFactor[2], baseColorFactor[3]);
			dstMaterial.m_EmissiveFactor = RGBAColor_t(emissiveFactor[0], emissiveFactor[1], emissiveFactor[2], 1.f);

			if (srcMaterial.extensions.find("KHR_materials_emissive_strength") != srcMaterial.extensions.cend()) {
				float emissiveStrength = srcMaterial.extensions.at("KHR_materials_emissive_strength").Get("emissiveStrength").GetNumberAsDouble();
				dstMaterial.m_EmissiveFactor *= emissiveStrength;
			}

			dstMaterial.m_MetallicFactor = static_cast<float>(pbrMetallicRoughness.metallicFactor);
			dstMaterial.m_RoughnessFactor = static_cast<float>(pbrMetallicRoughness.roughnessFactor);

			dstMaterial.m_AlphaCutoff = static_cast<float>(srcMaterial.alphaCutoff);

			dstMaterial.m_AlbedoMapIndex = -1;
			dstMaterial.m_MetalRoughnessMapIndex = -1;
			dstMaterial.m_NormalMapIndex = -1;
			dstMaterial.m_EmissiveMapIndex = -1;
			dstMaterial.m_OcclusionMapIndex.x = -1;
			dstMaterial.m_OcclusionMapIndex.y = srcMaterial.doubleSided;

			if (pbrMetallicRoughness.baseColorTexture.index > -1)
				dstMaterial.m_AlbedoMapIndex = pbrMetallicRoughness.baseColorTexture.index;
			if (pbrMetallicRoughness.metallicRoughnessTexture.index > -1)
				dstMaterial.m_MetalRoughnessMapIndex = pbrMetallicRoughness.metallicRoughnessTexture.index;
			if (srcMaterial.normalTexture.index > -1)
				dstMaterial.m_NormalMapIndex = srcMaterial.normalTexture.index;
			if (srcMaterial.emissiveTexture.index > -1)
				dstMaterial.m_EmissiveMapIndex = srcMaterial.emissiveTexture.index;
			if (srcMaterial.occlusionTexture.index > -1)
				dstMaterial.m_OcclusionMapIndex.x = srcMaterial.occlusionTexture.index;
		}

		std::unique_ptr<Renderer::CommandBuffer> commandBuffer = std::make_unique<Renderer::CommandBuffer>(device, commandPool);

		const auto materialsSize = m_Materials.size() * sizeof(Material);

		std::unique_ptr<Renderer::StagingBuffer> matStagingBuffer = std::make_unique<Renderer::StagingBuffer>(device, materialsSize);

		matStagingBuffer->Patch(m_Materials.data(), materialsSize);

		m_MaterialsBuffer = new Renderer::Buffer(device, materialsSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
		);

		const auto& graphicsQueue = device->GetGraphicsQueue();

		commandBuffer->Begin();
		commandBuffer->CopyBuffer(*matStagingBuffer, *m_MaterialsBuffer, materialsSize);
		commandBuffer->End();
		commandBuffer->SubmitToQueue(graphicsQueue);

		m_MaterialBufferDescriptor->Bind(
			{
				Renderer::Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, {.m_Buffer = m_MaterialsBuffer } }
			}
		);
	}
}