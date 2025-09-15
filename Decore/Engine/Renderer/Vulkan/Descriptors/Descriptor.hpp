#pragma once

namespace Engine::Renderer {
	class Descriptor {

	public:
		struct BindingInfo {
			VkDescriptorType m_Type{};

			union EngineType {
				class ImageView* m_ImageView;
				class Buffer* m_Buffer;
				class Sampler* m_Sampler;
			};

			EngineType m_Data;

			// Optional
			class Sampler* m_CombinedSampler = nullptr;
		};

		Descriptor(std::shared_ptr<Device> device, const std::vector<VkDescriptorSetLayoutBinding>& layoutBindings, const VkBufferUsageFlags bufferUsageFlags, void* optionalNext = nullptr);
		Descriptor(std::shared_ptr<Device> device, const DescriptorLayout& layout, const VkBufferUsageFlags bufferUsageFlags);
		~Descriptor();

		void Bind(const std::vector<BindingInfo>& bindingInfo);

		Buffer* GetBuffer() { return m_Buffer; }

		const VkDeviceSize GetSize() const;
		const std::vector<VkDeviceSize>& GetOffsets() const;
		const VkDescriptorSetLayout GetLayout() const;

		const uint32_t GetFlags() { return m_Flags; }
	private:
		// Needed for descriptor buffers.
		Buffer* m_Buffer = nullptr;

		std::shared_ptr<Device> m_Device;

		DescriptorLayout m_Layout{};

		uint32_t m_Flags{};
	};
}