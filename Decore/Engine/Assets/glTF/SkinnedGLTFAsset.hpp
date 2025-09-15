#pragma once
#include "BaseGLTFAsset.hpp"

namespace Engine::Assets {
	enum class ChannelType : unsigned char {
		None,
		Translation,
		Rotation,
		Scale
	};

	inline ChannelType GetChannelType(const std::string& string)
	{
		if (string == "translation") return ChannelType::Translation;
		if (string == "rotation") return ChannelType::Rotation;
		if (string == "scale") return ChannelType::Scale;
		return ChannelType::None;
	}

	class SkinnedGLTFAsset : public BaseGLTFAsset {
	public:
		SkinnedGLTFAsset(
			const std::string& gltfPath,
			std::shared_ptr<Renderer::Device> device,
			std::shared_ptr<Renderer::CommandPool> commandPool,
			int sceneIndex = -1
		);

		struct VertexAttribute {
			glm::vec3 Position{};
			glm::vec3 Normal{};
			glm::vec2 UV{};
			glm::vec4 Tangent{};
			glm::ivec4 Joints{};
			glm::vec4 Weights{};

			static VkVertexInputBindingDescription GetBindingDescription( )
			{
				return { 0, sizeof( VertexAttribute ), VK_VERTEX_INPUT_RATE_VERTEX };
			}

			static std::vector<VkVertexInputAttributeDescription> GetInputAttributeDescriptions( )
			{
				return {
					{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( VertexAttribute, Position ) },
					{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof( VertexAttribute, Normal ) },
					{ 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof( VertexAttribute, UV ) },
					{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof( VertexAttribute, Tangent ) },
					{ 4, 0, VK_FORMAT_R32G32B32A32_UINT, offsetof( VertexAttribute, Joints ) },
					{ 5, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof( VertexAttribute, Weights ) },
				};
			}
		};

		using VertexType = SkinnedGLTFAsset::VertexAttribute;

		struct AnimationSampler {
			std::string m_Interpolation{};
			std::vector<float> m_Inputs{};
			std::vector<glm::vec4> m_Outputs{};
		};

		struct AnimationChannel {
			ChannelType m_Type{};
			Node* m_Node{};
			uint32_t m_Sampler{}; // Sampler Index.
		};

		struct Animation {
			std::string m_Name{};
			std::vector<AnimationSampler> m_Samplers{};
			std::vector<AnimationChannel> m_Channels{};
			float m_Start{ FLT_MAX }, m_End{ FLT_MIN };
			float m_CurTime{};

			bool m_IsPaused{};
			// TODO: Animation event system.

			bool Update( float dt );
		};

		struct Skin {
			Node* m_SkeletonRootJoint{};
			std::vector<Node*> m_Joints{};
			std::vector<glm::mat4> m_InverseBindMatrices{};

			Renderer::Buffer* m_Buffer;
		};

		std::vector<Skin> m_Skins{};
		std::vector<Animation> m_Animations{};
		int m_ActiveAnimIndex{};

		__forceinline int GetAnimationIndexByName(const std::string& name) {
			if (m_Animations.empty())
				return -1;

			for (auto i = 0; i < m_Animations.size(); i++) {
				if (m_Animations[i].m_Name == name)
					return i;
			}

			return -1;
		}

		virtual void UnloadAsset() override {
			// m_Skins.clear();
			// m_Animations.clear();
		}

		virtual void Render(Renderer::CommandBuffer& commandBuffer, Renderer::Pipeline* pipeline, const std::vector<Renderer::Descriptor*>& sceneDescriptors, int bufferIndex) override;
		virtual void SetupDevice(const std::vector<Renderer::DescriptorLayout>& descriptorLayouts) override;
		
		void UpdateAnimation(int animIndex, float dt);

		glm::mat4 m_WorldMatrix{ 1.f };
	protected:

		// Vertex & Index Buffers
		std::vector<VertexType> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		virtual bool LoadAsset(const std::string& gltfPath, int sceneIndex = -1) override;
		virtual void LoadNode(const tinygltf::Node& inputNode, BaseGLTFAsset::Node* parent, uint32_t nodeIndex) override;

		virtual void CreateGeometryBuffers(std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool) override
		{
			const auto vertexSize = m_Vertices.size() * sizeof(VertexType);
			const auto indexSize = m_Indices.size() * sizeof(uint32_t);

			std::unique_ptr<Renderer::StagingBuffer> vertexStagingBuffer = std::make_unique<Renderer::StagingBuffer>(device, vertexSize);
			vertexStagingBuffer->Patch(m_Vertices.data(), vertexSize);

			std::unique_ptr<Renderer::StagingBuffer> indexStagingBuffer = std::make_unique<Renderer::StagingBuffer>(device, indexSize);
			indexStagingBuffer->Patch(m_Indices.data(), indexSize);

			m_VertexBuffer = new Renderer::Buffer(device, vertexSize,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE);
			m_IndexBuffer = new Renderer::Buffer(device, indexSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE);

			std::unique_ptr<Renderer::CommandBuffer> commandBuffer = std::make_unique<Renderer::CommandBuffer>(device, commandPool);

			const auto& graphicsQueue = device->GetGraphicsQueue();

			commandBuffer->Begin();
			commandBuffer->CopyBuffer(*vertexStagingBuffer, *m_VertexBuffer, static_cast<VkDeviceSize>(vertexSize));
			commandBuffer->CopyBuffer(*indexStagingBuffer, *m_IndexBuffer, static_cast<VkDeviceSize>(indexSize));
			commandBuffer->End();
			commandBuffer->SubmitToQueue(graphicsQueue);
		}
	private:
		Renderer::Descriptor* m_JointDescriptor = nullptr;
		Renderer::Buffer* m_JointBuffer = nullptr;

		struct IndirectPrimitiveData {
			glm::ivec4 MaterialIndex;
			glm::mat4 NodeMatrix;
			glm::vec4 NodePos; // Node bounding sphere.
		};

		std::vector<VkDrawIndexedIndirectCommand> m_IndirectCommands{};
		std::vector<IndirectPrimitiveData> m_PerPrimitiveData{};

		Renderer::Buffer* m_IndirectCommandsBuffer = nullptr;
		Renderer::Buffer* m_PrimitiveStorageBuffer = nullptr;
		Renderer::Descriptor* m_PrimitiveBufferDescriptor = nullptr;

		void BuildIndirectBatches(std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool, const Renderer::DescriptorLayout& primitiveLayout);

		void LoadSkins();
		void LoadAnimations();
		
		void LoadBoneData(std::shared_ptr<Renderer::Device> device);

		void UpdateJoints(Node* node);
	};
}