#include "../../Renderer/Vulkan/VulkanRenderer.hpp"


#include "StaticGLTFAsset.hpp"
#include "BaseGLTFAsset.hpp"

namespace Engine::Assets {
	StaticGLTFAsset::StaticGLTFAsset(
		const std::string& gltfPath,
		std::shared_ptr<Renderer::Device> device,
		std::shared_ptr<Renderer::CommandPool> commandPool,
		int sceneIndex )
	{
		if ( !LoadAsset( gltfPath, sceneIndex ) )
			throw std::runtime_error( "Failed to load asset " + gltfPath );

		m_Device = device;
		m_CommandPool = commandPool;
	}

	StaticGLTFAsset::~StaticGLTFAsset() {

	}

	bool StaticGLTFAsset::LoadAsset(const std::string& gltfPath, int sceneIndex) {
		UnloadAsset();

		if (!OpenFile(gltfPath))
			return false;

		const auto& scene = m_LoadedModel.scenes[sceneIndex == -1 ? (m_LoadedModel.defaultScene > -1 ? m_LoadedModel.defaultScene : 0) : sceneIndex];
		if (scene.nodes.empty()) {
			return false;
		}

		for (size_t i = 0; i < scene.nodes.size(); i++) {
			GetNodeInfo(m_LoadedModel.nodes[scene.nodes[i]], m_LoadedModel, m_VertexCount, m_IndexCount);
		}

		m_Vertices.resize(m_VertexCount);
		m_Indices.resize(m_IndexCount);

		for (auto i = 0; i < scene.nodes.size(); i++) {
			LoadNode(m_LoadedModel.nodes[scene.nodes[i]], nullptr, scene.nodes[i]);
		}

		return true;
	}

	void StaticGLTFAsset::LoadNode(const tinygltf::Node& inputNode, Node* parent, uint32_t nodeIndex) {
		Node* node = new Node(inputNode.name, parent, nodeIndex, inputNode.skin);

		node->ParseOrientation(inputNode);

		m_AllNodes.push_back(node);

		for (auto i = 0; i < inputNode.children.size(); i++) {
			LoadNode(m_LoadedModel.nodes[inputNode.children[i]], node, inputNode.children[i]);
		}

		if (inputNode.mesh > -1) {
			node->m_Mesh = new Mesh();

			const auto& srcMesh = m_LoadedModel.meshes[inputNode.mesh];

			node->m_Mesh->m_Primitives.resize(srcMesh.primitives.size());

			for (auto i = 0; i < srcMesh.primitives.size(); i++) {
				uint32_t vertexStart = static_cast<uint32_t>(m_LastVertex);
				uint32_t indexStart = static_cast<uint32_t>(m_LastIndex);

				uint32_t indexCount = 0;

				const auto& primitive = srcMesh.primitives[i];

				// Position attribute is required.
				const auto posAttribute = primitive.attributes.find("POSITION");
				const tinygltf::Accessor& posAccessor = m_LoadedModel.accessors[posAttribute->second];
				const tinygltf::BufferView& posView = m_LoadedModel.bufferViews[posAccessor.bufferView];

				const float* bufferPos = reinterpret_cast<const float*>(&(m_LoadedModel.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));

				glm::vec3 posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
				glm::vec3 posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
				
				uint32_t vertexCount = static_cast<uint32_t>(posAccessor.count);
				int posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

				// Normals
				const float* bufferNormals = nullptr;
				int normByteStride{};

				if (const auto normAttribute = primitive.attributes.find("NORMAL"); normAttribute != primitive.attributes.end()) {
					const auto& normAccessor = m_LoadedModel.accessors[normAttribute->second];
					const auto& normView = m_LoadedModel.bufferViews[normAccessor.bufferView];

					bufferNormals = reinterpret_cast<const float*>(&(m_LoadedModel.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
					normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
				}

				// UVs (We only support TEXCOORD_0)
				const float* bufferUV = nullptr;
				int uvByteStride;

				if (const auto uvAttribute = primitive.attributes.find("TEXCOORD_0"); uvAttribute != primitive.attributes.end()) {
					const auto& uvAccessor = m_LoadedModel.accessors[uvAttribute->second];
					const auto& uvView = m_LoadedModel.bufferViews[uvAccessor.bufferView];

					bufferUV = reinterpret_cast<const float*>(&(m_LoadedModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					uvByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
				}

				// Tangents
				const float* bufferTangents = nullptr;
				int tangentByteStride;

				if (const auto tangentAttribute = primitive.attributes.find("TANGENT"); tangentAttribute != primitive.attributes.end()) {
					const auto& tangentAccessor = m_LoadedModel.accessors[tangentAttribute->second];
					const auto& tangentView = m_LoadedModel.bufferViews[tangentAccessor.bufferView];

					bufferTangents = reinterpret_cast<const float*>(&(m_LoadedModel.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
					tangentByteStride = tangentAccessor.ByteStride(tangentView) ? (tangentAccessor.ByteStride(tangentView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				for (auto j = 0; j < posAccessor.count; j++) {
					VertexAttribute& v = m_Vertices[m_LastVertex];

					v.Position = glm::make_vec3(&bufferPos[j * posByteStride]);
					v.Normal = glm::normalize(bufferNormals ? glm::make_vec3(&bufferNormals[j * normByteStride]) : glm::vec3(0.f));
					v.Tangent = glm::vec4(bufferTangents ? glm::make_vec4(&bufferTangents[j * tangentByteStride]) : glm::vec4(0.f));
					v.UV = glm::make_vec2(bufferUV ? glm::make_vec2(&bufferUV[j * uvByteStride]) : glm::vec2(0.f));

					m_LastVertex++;
				}

				if (primitive.indices > -1) {
					const auto& accessor = m_LoadedModel.accessors[primitive.indices > -1 ? primitive.indices : 0];
					const auto& bufferView = m_LoadedModel.bufferViews[accessor.bufferView];
					const auto& buffer = m_LoadedModel.buffers[bufferView.buffer];

					indexCount = static_cast<uint32_t>(accessor.count);
					const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT) {
						const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							m_Indices[m_LastIndex++] = buf[index] + vertexStart;
						}
					}

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)
					{
						const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							m_Indices[m_LastIndex++] = buf[index] + vertexStart;
						}
					}

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE) {
						const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++) {
							m_Indices[m_LastIndex++] = buf[index] + vertexStart;
						}
					}
				}

				Primitive& newPrimitive = node->m_Mesh->m_Primitives[i];
				newPrimitive.m_MaterialIndex = primitive.material > -1 ? primitive.material : 0; // Default to first material...
				newPrimitive.m_IndexOffset = indexStart;
				newPrimitive.m_IndexCount = indexCount;
				newPrimitive.m_VertexCount = vertexCount;

				newPrimitive.m_Bounds.m_Mins = glm::min( posMin, newPrimitive.m_Bounds.m_Mins );
				newPrimitive.m_Bounds.m_Maxs = glm::max( posMax, newPrimitive.m_Bounds.m_Maxs );
				 
				//printf( "mins: %f %f %f\n", newPrimitive.m_Bounds.m_Mins.x, newPrimitive.m_Bounds.m_Mins.y, newPrimitive.m_Bounds.m_Mins.z );
				//printf( "maxs: %f %f %f\n", newPrimitive.m_Bounds.m_Maxs.x, newPrimitive.m_Bounds.m_Maxs.y, newPrimitive.m_Bounds.m_Maxs.z );
			}
		}

		if (parent) {
			parent->m_Children.push_back(node);
		}
		else {
			m_Nodes.push_back(node);
		}
	}

	void StaticGLTFAsset::BuildIndirectBatches(std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool, const Renderer::DescriptorLayout& primitiveLayout) {
		m_IndirectCommands.clear();

		// Build the indirect commands.
		uint32_t m = 0;
		for (auto i = 0; i < m_AllNodes.size(); i++) {
			auto* node = m_AllNodes[i];

			if (node->m_Mesh) {
				for (const auto& primitive : node->m_Mesh->m_Primitives) {
					VkDrawIndexedIndirectCommand cmd{};
					IndirectPrimitiveData data{};

					const auto& bounds = primitive.m_Bounds;
					const auto origin = (bounds.m_Maxs + bounds.m_Mins) * 0.5f;
					const auto extents = (bounds.m_Maxs - bounds.m_Mins) * 0.5f;
					const auto radius = glm::length(extents);

					// TODO: Replace with model matrix.
					glm::mat4 inv = glm::mat4(1.f);
					inv[2][2] *= -1.f;

					data.NodeMatrix = node->GetMatrix();
					data.MaterialIndex.x = primitive.m_MaterialIndex;
					data.MaterialIndex.y = i;
					data.NodePos = glm::vec4(origin, 1.f) * inv * data.NodeMatrix;
					data.NodePos.w = radius;

					cmd.indexCount = primitive.m_IndexCount;
					cmd.instanceCount = 1;
					cmd.firstIndex = static_cast<uint32_t>(primitive.m_IndexOffset);
					cmd.vertexOffset = 0;
					cmd.firstInstance = m++;

					m_IndirectCommands.push_back(cmd);
					m_PerPrimitiveData.push_back(data);
				}
			}
		}

		// Setup and transfer the data to the GPU.
		const auto& graphicsQueue = device->GetGraphicsQueue();

		std::unique_ptr<Renderer::CommandBuffer> commandBuffer = std::make_unique<Renderer::CommandBuffer>(device, commandPool);

		const auto cmdBufferSize = m_IndirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand);
		const auto storageBufferSize = m_PerPrimitiveData.size() * sizeof(IndirectPrimitiveData);

		std::unique_ptr<Renderer::StagingBuffer> cmdStagingBuffer = std::make_unique<Renderer::StagingBuffer>(device, cmdBufferSize);

		cmdStagingBuffer->Patch(m_IndirectCommands.data(), m_IndirectCommands.size() * sizeof(VkDrawIndexedIndirectCommand));

		std::unique_ptr<Renderer::StagingBuffer> storageStagingBuffer = std::make_unique<Renderer::StagingBuffer>(device, storageBufferSize);

		storageStagingBuffer->Patch(m_PerPrimitiveData.data(), m_PerPrimitiveData.size() * sizeof(IndirectPrimitiveData));

		for (auto i = 0; i < m_IndirectCommandsBuffers.size(); i++) {
			m_IndirectCommandsBuffers[i] = new Renderer::Buffer(device, cmdBufferSize,
				VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
				VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
				VK_SHARING_MODE_EXCLUSIVE,
				VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
			);
		}

		m_PrimitiveStorageBuffer = new Renderer::Buffer(device, storageBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
		);

		commandBuffer->Begin();
		for (auto i = 0; i < m_IndirectCommandsBuffers.size(); i++)
			commandBuffer->CopyBuffer(*cmdStagingBuffer, *m_IndirectCommandsBuffers[i], static_cast<VkDeviceSize>(cmdBufferSize));

		commandBuffer->CopyBuffer(*storageStagingBuffer, *m_PrimitiveStorageBuffer, static_cast<VkDeviceSize>(storageBufferSize));
		commandBuffer->End();
		commandBuffer->SubmitToQueue(graphicsQueue);

		m_PrimitiveBufferDescriptor = new Renderer::Descriptor(device,
			primitiveLayout,
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		m_PrimitiveBufferDescriptor->Bind(
			{
				Renderer::Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, {.m_Buffer = m_PrimitiveStorageBuffer } }
			}
		);

		auto indirectLayout = Renderer::DescriptorLayout(device, { { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr} });

		for (auto i = 0; i < m_IndirectCommandsBuffers.size(); i++) {
			m_IndirectBufferDescriptors[i] = new Renderer::Descriptor(device,
				indirectLayout,
				VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
			);

			m_IndirectBufferDescriptors[i]->Bind(
				{
					Renderer::Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, {.m_Buffer = m_IndirectCommandsBuffers[i] }}
				}
			);
		}
	}

	void StaticGLTFAsset::Render( Renderer::CommandBuffer& commandBuffer, Renderer::Pipeline* pipeline, const std::vector<Renderer::Descriptor*>& sceneDescriptors, int bufferIndex )
	{
		vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline );

		std::vector<Renderer::Descriptor*> descriptors = sceneDescriptors;
	
		std::vector<Renderer::Descriptor*> assetDescriptors = { m_MaterialBufferDescriptor, m_TextureBufferDescriptor, m_PrimitiveBufferDescriptor };
		descriptors.insert( descriptors.end( ), assetDescriptors.cbegin( ), assetDescriptors.cend( ) );

		commandBuffer.BindDescriptors( descriptors );
		commandBuffer.SetDescriptorOffsets( descriptors, *pipeline );

		const VkDeviceSize offsets[ 1 ] = { 0 };
		const VkBuffer vertexBuffer = *m_VertexBuffer;
		vkCmdBindVertexBuffers( commandBuffer, 0, 1, &vertexBuffer, offsets );
		vkCmdBindIndexBuffer( commandBuffer, *m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32 );

		// TODO: Check if device supports MultiDrawIndirect.
		vkCmdDrawIndexedIndirect( commandBuffer, *m_IndirectCommandsBuffers[ bufferIndex ], 0, static_cast< uint32_t >( m_IndirectCommands.size( ) ), sizeof( VkDrawIndexedIndirectCommand ) );
	}

	void StaticGLTFAsset::SetupDevice( const std::vector<Renderer::DescriptorLayout>& descriptorLayouts )
	{
		CreateGeometryBuffers( m_Device, m_CommandPool );
		LoadMaterials( m_Device, m_CommandPool, descriptorLayouts[ 0 ] );
		LoadTextures( m_Device, m_CommandPool, descriptorLayouts[ 1 ] );
		BuildIndirectBatches( m_Device, m_CommandPool, descriptorLayouts[ 2 ] );
	}

	bool StaticGLTFAsset::Intersects(const Core::CollisionCapsule& capsule) {
		return true;
	}

	bool StaticGLTFAsset::Intersects(const Core::CollisionBox& aabb, glm::vec3& normal ) {
		for (auto node : m_AllNodes) {
			if (node->m_Mesh) {
				for (auto& primitive : node->m_Mesh->m_Primitives) 
				{
					glm::vec3 mins = primitive.m_Bounds.m_Mins;
					glm::vec3 maxs = primitive.m_Bounds.m_Maxs;

					glm::mat4 m = glm::mat4( 1.f );
					m[ 2 ][ 2 ] *= -1.f;

					mins = glm::vec3( node->GetMatrix( ) * glm::vec4( mins, 1.f ) );
					mins = glm::vec3( m * glm::vec4( mins, 1.f ) );

					maxs = glm::vec3( node->GetMatrix( ) * glm::vec4( maxs, 1.f ) );
					maxs = glm::vec3( m * glm::vec4( maxs, 1.f ) );

					if ( mins.x > maxs.x )
						std::swap( mins.x, maxs.x );
					if ( mins.y > maxs.y )
						std::swap( mins.y, maxs.y );
					if ( mins.z > maxs.z )
						std::swap( mins.z, maxs.z );

					/*
					glm::vec3 mins( std::numeric_limits<float>::max( ) );
					glm::vec3 maxs( std::numeric_limits<float>::min( ) );
					
					for ( auto i = 0; i < primitive.m_IndexCount; i++ )
					{
						glm::vec3 p0 = m_Vertices[ m_Indices[ primitive.m_IndexOffset + i ] ].Position;

						glm::mat4 m = glm::mat4( 1.f );
						m[ 2 ][ 2 ] *= -1.f;

						p0 = glm::vec3( node->GetMatrix( ) * glm::vec4( p0, 1.f ) );
						p0 = glm::vec3( m * glm::vec4( p0, 1.f ) );

						// TODO: Debug why these results are incorrect.
						mins = glm::min( p0, mins );
						maxs = glm::max( p0, maxs );
					}*/

					Core::CollisionBox primBox(mins, maxs);

					if ( aabb.Intersects( primBox ) )
					{
						glm::vec3 c1 = ( aabb.GetMaxs( ) + aabb.GetMins( ) ) * 0.5f;
						
						normal = ( c1 - primBox.GetClosestPoint( c1 ) );
						return true;
					}
				}
			}
		}

		/*for (auto i = 0; i < m_Indices.size() / 3; i += 3) {
			glm::vec3 p0 = m_Vertices[m_Indices[i]].Position;
			glm::vec3 p1 = m_Vertices[m_Indices[i + 1]].Position;
			glm::vec3 p2 = m_Vertices[m_Indices[i + 2]].Position;

			glm::mat4 m = glm::mat4(1.f);
			m[2][2] *= -1.f;

			p0 = glm::vec4(p0, 1.f) * m;
			p1 = glm::vec4(p1, 1.f) * m;
			p2 = glm::vec4(p2, 1.f) * m;

		
		}*/

		return false;
	}
}