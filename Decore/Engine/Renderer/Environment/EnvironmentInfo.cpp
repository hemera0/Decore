#include "EnvironmentInfo.hpp"

#include "../../Assets/Assets.hpp"

namespace Engine::Renderer {
	EnvironmentInfo::EnvironmentInfo(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::string& envDirectory) {
		Setup(device, commandPool, envDirectory);
	}

	void EnvironmentInfo::Setup(std::shared_ptr<Device> device, std::shared_ptr<CommandPool> commandPool, const std::string& envDirectory) {
		std::string defaultDiffuseTexPath = envDirectory + "\\diffuse.ktx2";
		if (!std::filesystem::exists(defaultDiffuseTexPath))
			return;

		std::string defaultSpecularTexPath = envDirectory + "\\specular.ktx2";
		if (!std::filesystem::exists(defaultSpecularTexPath))
			return;

		std::string defaultLUTTexPath = envDirectory + "\\lut.png";
		if (!std::filesystem::exists(defaultLUTTexPath))
			return;

		Engine::Renderer::Image* image = Engine::Renderer::Image::LoadKTXFromFile(device, commandPool,
			defaultDiffuseTexPath.c_str(), nullptr);
		if (!image)
			return;

		VkImageViewCreateInfo createInfo = Engine::Renderer::ImageView::GetDefault2DCreateInfo(*image, image->GetLevelCount(), image->GetFormat());
		createInfo.viewType = image->GetViewType();
		createInfo.subresourceRange.layerCount = image->GetLayerCount();
		createInfo.subresourceRange.levelCount = image->GetLevelCount();

		Engine::Renderer::ImageView* imageView = new Engine::Renderer::ImageView(
			device,
			*image,
			image->GetLevelCount(),
			image->GetFormat(),
			&createInfo
		);

		if (!imageView)
			return;

		m_Textures.push_back({ image, imageView });

		image = Engine::Renderer::Image::LoadKTXFromFile(device, commandPool,
			defaultSpecularTexPath.c_str(), nullptr);
		if (!image)
			return;

		createInfo = Engine::Renderer::ImageView::GetDefault2DCreateInfo(*image, image->GetLevelCount(), image->GetFormat());
		createInfo.viewType = image->GetViewType();
		createInfo.subresourceRange.layerCount = image->GetLayerCount();
		createInfo.subresourceRange.levelCount = image->GetLevelCount();
		imageView = new Engine::Renderer::ImageView(device, *image, image->GetLevelCount(), image->GetFormat(), &createInfo);

		m_Textures.push_back({ image, imageView });

		image = Engine::Renderer::Image::LoadFromFile(device, commandPool, defaultLUTTexPath.c_str(), nullptr);
		if (!image)
			return;

		imageView = new Engine::Renderer::ImageView(device, *image, image->GetLevelCount(), image->GetFormat());
		if (!imageView)
			return;

		m_Textures.push_back({ image, imageView });

		// Upper bound for the amount of images that could fit. (They are bindless.)
		constexpr const uint32_t MAX_TEXTURES_COUNT = 100;

		VkDescriptorBindingFlagsEXT bindingFlagsExt = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
		VkDescriptorSetLayoutBindingFlagsCreateInfoEXT setLayoutBindingFlagsExt = {
		   VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT, nullptr, 1, &bindingFlagsExt
		};

		m_EnvDescriptor = new Engine::Renderer::Descriptor(device,
			{
				{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_TEXTURES_COUNT, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }
			},

			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			&setLayoutBindingFlagsExt
		);

		std::vector<Engine::Renderer::Descriptor::BindingInfo> bindingInfos{};
		bindingInfos.resize(m_Textures.size());

		m_EnvSampler = new Sampler(device, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_FILTER_LINEAR, 0.f);

		for (auto i = 0; i < m_Textures.size(); i++) {
			auto& textureInfo = m_Textures[i];

			bindingInfos[i] = Engine::Renderer::Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {.m_ImageView = textureInfo.second }, m_EnvSampler };
		}

		m_EnvDescriptor->Bind(bindingInfos);
	}

	EnvironmentInfo::~EnvironmentInfo() {
		for (auto& [image, imageView] : m_Textures) {
			delete imageView;
			delete image;
		}
	}
}