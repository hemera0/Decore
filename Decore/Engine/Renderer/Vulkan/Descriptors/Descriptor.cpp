#include "../VulkanRenderer.hpp"

namespace Engine::Renderer {
	Descriptor::Descriptor(std::shared_ptr<Device> device, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings, const VkBufferUsageFlags bufferUsageFlags, void* optionalNext) : m_Device(device) {		
		m_Layout = DescriptorLayout(device, layoutBindings, optionalNext);
		m_Buffer = new Buffer(m_Device, m_Layout.m_Size,
			bufferUsageFlags,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
	}

	Descriptor::Descriptor(std::shared_ptr<Device> device, const DescriptorLayout& layout, const VkBufferUsageFlags bufferUsageFlags) : m_Device(device) {
		m_Layout = layout;
		m_Buffer = new Buffer(m_Device, m_Layout.m_Size,
			bufferUsageFlags,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
	}

	const VkDeviceSize Descriptor::GetSize() const {
		return m_Layout.m_Size;
	}
	const std::vector<VkDeviceSize>& Descriptor::GetOffsets() const {
		return m_Layout.m_Offsets;
	}

	const VkDescriptorSetLayout Descriptor::GetLayout() const {
		return m_Layout.GetLayout();
	}

	Descriptor::~Descriptor() {
		if (m_Buffer)
			delete m_Buffer;

		// nono.
		//vkDestroyDescriptorSetLayout(m_Device, m_Layout.GetLayout(), nullptr);
	}

	// TODO: Look into. 
	// There is a really nasty bug that is occurring for me when I have a Descriptor that contains a combined image sampler (binding 0) 
	// followed by a uniform buffer (binding 1) The contents of the uniform buffer appeared to be inaccessible in the VS/PS.
	void Descriptor::Bind(const std::vector<BindingInfo>& bindingInfo) {
		const auto& physicalDevice = m_Device->GetPhysicalDevice();
		const auto& descriptorBufferProperties = physicalDevice->GetDescriptorBufferProperties();

		char* ptr = reinterpret_cast<char*>(GetBuffer()->Map());

		VkDeviceSize totalOffset{ };

		for (auto i = 0; i < bindingInfo.size(); i++) {
			const auto& info = bindingInfo[i];

			VkDescriptorGetInfoEXT descGetInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_GET_INFO_EXT };
			VkDescriptorAddressInfoEXT descAddrInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_ADDRESS_INFO_EXT };
			VkDescriptorImageInfo descImageInfo = {};
			VkDeviceSize descSize = {};

			if (info.m_Type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
				descImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				descImageInfo.imageView = *info.m_Data.m_ImageView;
				descImageInfo.sampler = *info.m_CombinedSampler;

				descGetInfo.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descGetInfo.data.pCombinedImageSampler = &descImageInfo;

				descSize = descriptorBufferProperties.combinedImageSamplerDescriptorSize;

				// Used in CommandBuffer binding.
				m_Flags |= VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT;
			}

			if (info.m_Type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
				descAddrInfo.address = info.m_Data.m_Buffer->GetDeviceAddress();
				descAddrInfo.range = info.m_Data.m_Buffer->GetSize();
				descAddrInfo.format = VK_FORMAT_UNDEFINED;

				descGetInfo.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descGetInfo.data.pUniformBuffer = &descAddrInfo;

				descSize = descriptorBufferProperties.uniformBufferDescriptorSize;
			}

			if (info.m_Type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
				descAddrInfo.address = info.m_Data.m_Buffer->GetDeviceAddress();
				descAddrInfo.range = info.m_Data.m_Buffer->GetSize();
				descAddrInfo.format = VK_FORMAT_UNDEFINED;

				descGetInfo.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				descGetInfo.data.pStorageBuffer = &descAddrInfo;

				descSize = descriptorBufferProperties.storageBufferDescriptorSize;
			}

			vkGetDescriptorEXT(*m_Device, &descGetInfo, descSize, ptr );

			ptr = ptr + descSize;
		}

		GetBuffer()->Unmap();
	}
}