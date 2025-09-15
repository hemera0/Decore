#pragma once
#include "BaseGLTFAsset.hpp"

namespace Engine::Assets {

	constexpr const int MAX_MATERIAL_TEXTURE_COUNT = 1000;

	class StaticGLTFAsset : public BaseGLTFAsset {
	public:
		StaticGLTFAsset(
			const std::string& gltfPath, 
			std::shared_ptr<Renderer::Device> device, 
			std::shared_ptr<Renderer::CommandPool> commandPool, 
			int sceneIndex = -1
		);
		~StaticGLTFAsset();

		struct VertexAttribute {
			glm::vec3 Position{};
			glm::vec3 Normal{};
			glm::vec2 UV{};
			glm::vec4 Tangent{};

			static Renderer::VertexInfo GetVertexInfo() {
				static Renderer::VertexInfo vi = {};
				vi.m_InputAttributeDesc = GetInputAttributeDescriptions();
				vi.m_InputBindingDesc = { GetBindingDescription() };
				return vi;
			};

			static VkVertexInputBindingDescription GetBindingDescription() {
				return { 0, sizeof(VertexAttribute), VK_VERTEX_INPUT_RATE_VERTEX };
			}

			static std::vector<VkVertexInputAttributeDescription> GetInputAttributeDescriptions()
			{
				return {
					{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexAttribute, Position) },
					{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VertexAttribute, Normal) },
					{ 2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(VertexAttribute, UV) },
					{ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(VertexAttribute, Tangent) }
				};
			}
		};

		using VertexType = StaticGLTFAsset::VertexAttribute;

		virtual void Render(Renderer::CommandBuffer& commandBuffer, Renderer::Pipeline* pipeline, const std::vector<Renderer::Descriptor*>& sceneDescriptors, int bufferIndex) override;
		virtual void SetupDevice(const std::vector<Renderer::DescriptorLayout>& descriptorLayouts) override;
	
		bool Intersects(const Core::CollisionCapsule& capsule);
		bool Intersects(const Core::CollisionBox& aabb, glm::vec3& normal);

		size_t GetIndirectCommandsCount() { return m_IndirectCommands.size(); }
		Renderer::Descriptor* GetIndirectDescriptor(int index = 0) { return m_IndirectBufferDescriptors[index]; }
		Renderer::Descriptor* GetPrimitiveDescriptor() { return m_PrimitiveBufferDescriptor; }
	public: // TODO: Remove.
		// Vertex & Index Buffers
		std::vector<VertexType> m_Vertices{};
		std::vector<uint32_t> m_Indices{};

		glm::mat4 m_WorldMatrix{ 1.f };
	protected:
		virtual bool LoadAsset(const std::string& gltfPath, int sceneIndex = -1) override;
		virtual void LoadNode(const tinygltf::Node& inputNode, Node* parent, uint32_t nodeIndex) override;

		virtual void CreateGeometryBuffers(std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool) override
		{
			const auto vertexSize = m_Vertices.size() * sizeof(VertexAttribute);
			const auto indexSize = m_Indices.size() * sizeof(uint32_t);

			std::unique_ptr<Renderer::StagingBuffer> vertexStagingBuffer = std::make_unique<Renderer::StagingBuffer>(device, vertexSize);
			vertexStagingBuffer->Patch(m_Vertices.data(), vertexSize);

			std::unique_ptr<Renderer::StagingBuffer> indexStagingBuffer = std::make_unique<Renderer::StagingBuffer>(device, indexSize);
			indexStagingBuffer->Patch(m_Indices.data(), indexSize);

			m_VertexBuffer = new Renderer::Buffer(device, vertexSize,
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE);
			m_IndexBuffer = new Renderer::Buffer(device, indexSize,
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, VK_SHARING_MODE_EXCLUSIVE);

			std::unique_ptr<Renderer::CommandBuffer> commandBuffer = std::make_unique<Renderer::CommandBuffer>(device, commandPool);

			const auto& graphicsQueue = device->GetGraphicsQueue();

			commandBuffer->Begin();
			commandBuffer->CopyBuffer(*vertexStagingBuffer, *m_VertexBuffer, static_cast<VkDeviceSize>(vertexSize));
			commandBuffer->CopyBuffer(*indexStagingBuffer, *m_IndexBuffer, static_cast<VkDeviceSize>(indexSize));
			commandBuffer->End();
			commandBuffer->SubmitToQueue(graphicsQueue);
		}

		struct IndirectPrimitiveData {
			glm::ivec4 MaterialIndex;
			glm::mat4 NodeMatrix;
			glm::vec4 NodePos; // Node bounding sphere.
		};

		std::vector<VkDrawIndexedIndirectCommand> m_IndirectCommands{};
		std::vector<IndirectPrimitiveData> m_PerPrimitiveData{};

		std::array<Renderer::Buffer*, 5> m_IndirectCommandsBuffers{};
		std::array<Renderer::Descriptor*, 5> m_IndirectBufferDescriptors{}; // Used for culling.

		Renderer::Buffer* m_PrimitiveStorageBuffer = nullptr;
		Renderer::Descriptor* m_PrimitiveBufferDescriptor = nullptr;

		void BuildIndirectBatches(std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool, const Renderer::DescriptorLayout& primitiveLayout);

		virtual void UnloadAsset() override {
			for (auto i{ 0u }; i < m_AllNodes.size(); i++) {
				if (m_AllNodes[i]) { delete m_AllNodes[i]; m_AllNodes[i] = nullptr; }
			}

			m_AllNodes.clear();
			m_Nodes.clear();

			m_Textures.clear();
			m_Materials.clear();
		}
	};
}