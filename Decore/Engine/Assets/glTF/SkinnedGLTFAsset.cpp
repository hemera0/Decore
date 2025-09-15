#include "../../Renderer/Vulkan/VulkanRenderer.hpp"
#include "../../Core/Time/TimeSystem.hpp"

#include "SkinnedGLTFAsset.hpp"

namespace Engine::Assets
{

	SkinnedGLTFAsset::SkinnedGLTFAsset(
		const std::string& gltfPath,
		std::shared_ptr<Renderer::Device> device,
		std::shared_ptr<Renderer::CommandPool> commandPool,
		int sceneIndex
	)
	{
		if (!LoadAsset(gltfPath, sceneIndex))
			throw std::runtime_error("Failed to load asset: " + gltfPath);

		m_Device = device;
		m_CommandPool = commandPool;
	}

	bool SkinnedGLTFAsset::LoadAsset(const std::string& gltfPath, int sceneIndex)
	{
		if (!OpenFile(gltfPath))
			return false;

		const auto& scene = m_LoadedModel.scenes[sceneIndex == -1 ? (m_LoadedModel.defaultScene > -1 ? m_LoadedModel.defaultScene : 0) : sceneIndex];
		if (scene.nodes.empty())
		{
			return false;
		}

		for (size_t i = 0; i < scene.nodes.size(); i++)
		{
			GetNodeInfo(m_LoadedModel.nodes[scene.nodes[i]], m_LoadedModel, m_VertexCount, m_IndexCount);
		}

		m_Vertices.resize(m_VertexCount);
		m_Indices.resize(m_IndexCount);

		for (auto i = 0; i < scene.nodes.size(); i++)
		{
			LoadNode(m_LoadedModel.nodes[scene.nodes[i]], nullptr, scene.nodes[i]);
		}

		m_WorldMatrix = glm::mat4(1.f);

		return true;
	}

	void SkinnedGLTFAsset::LoadNode(const tinygltf::Node& inputNode, Node* parent, uint32_t nodeIndex)
	{
		Node* node = new Node(inputNode.name, parent, nodeIndex, inputNode.skin);

		node->ParseOrientation(inputNode);

		m_AllNodes.push_back(node);

		for (auto i = 0; i < inputNode.children.size(); i++)
		{
			LoadNode(m_LoadedModel.nodes[inputNode.children[i]], node, inputNode.children[i]);
		}

		if (inputNode.mesh > -1)
		{
			node->m_Mesh = new Mesh();

			const auto& srcMesh = m_LoadedModel.meshes[inputNode.mesh];

			node->m_Mesh->m_Primitives.resize(srcMesh.primitives.size());

			for (auto i = 0; i < srcMesh.primitives.size(); i++)
			{
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

				if (const auto normAttribute = primitive.attributes.find("NORMAL"); normAttribute != primitive.attributes.end())
				{
					const auto& normAccessor = m_LoadedModel.accessors[normAttribute->second];
					const auto& normView = m_LoadedModel.bufferViews[normAccessor.bufferView];

					bufferNormals = reinterpret_cast<const float*>(&(m_LoadedModel.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
					normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
				}

				// UVs (We only support TEXCOORD_0)
				const float* bufferUV = nullptr;
				int uvByteStride;

				if (const auto uvAttribute = primitive.attributes.find("TEXCOORD_0"); uvAttribute != primitive.attributes.end())
				{
					const auto& uvAccessor = m_LoadedModel.accessors[uvAttribute->second];
					const auto& uvView = m_LoadedModel.bufferViews[uvAccessor.bufferView];

					bufferUV = reinterpret_cast<const float*>(&(m_LoadedModel.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					uvByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
				}

				// Tangents
				const float* bufferTangents = nullptr;
				int tangentByteStride;

				if (const auto tangentAttribute = primitive.attributes.find("TANGENT"); tangentAttribute != primitive.attributes.end())
				{
					const auto& tangentAccessor = m_LoadedModel.accessors[tangentAttribute->second];
					const auto& tangentView = m_LoadedModel.bufferViews[tangentAccessor.bufferView];

					bufferTangents = reinterpret_cast<const float*>(&(m_LoadedModel.buffers[tangentView.buffer].data[tangentAccessor.byteOffset + tangentView.byteOffset]));
					tangentByteStride = tangentAccessor.ByteStride(tangentView) ? (tangentAccessor.ByteStride(tangentView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				// Skinning
				// Joints
				const void* bufferJoints = nullptr;
				int jointByteStride;
				int jointComponentType;

				if (const auto jointsAttribute = primitive.attributes.find("JOINTS_0"); jointsAttribute != primitive.attributes.end())
				{
					const auto& jointAccessor = m_LoadedModel.accessors[jointsAttribute->second];
					const auto& jointView = m_LoadedModel.bufferViews[jointAccessor.bufferView];

					bufferJoints = &(m_LoadedModel.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
					jointComponentType = jointAccessor.componentType;
					jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				// Weights
				const float* bufferWeights = nullptr;
				int weightByteStride;

				if (const auto weightsAttribute = primitive.attributes.find("WEIGHTS_0"); weightsAttribute != primitive.attributes.end())
				{
					const tinygltf::Accessor& weightAccessor = m_LoadedModel.accessors[weightsAttribute->second];
					const tinygltf::BufferView& weightView = m_LoadedModel.bufferViews[weightAccessor.bufferView];

					bufferWeights = reinterpret_cast<const float*>(&(m_LoadedModel.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
					weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				const bool isSkinned = bufferWeights && bufferJoints;

				for (auto j = 0; j < posAccessor.count; j++)
				{
					VertexType& v = m_Vertices[m_LastVertex];

					v.Position = glm::make_vec3(&bufferPos[j * posByteStride]);
					v.Normal = glm::normalize(bufferNormals ? glm::make_vec3(&bufferNormals[j * normByteStride]) : glm::vec3(0.f));
					v.Tangent = glm::vec4(bufferTangents ? glm::make_vec4(&bufferTangents[j * tangentByteStride]) : glm::vec4(0.f));
					v.UV = glm::make_vec2(bufferUV ? glm::make_vec2(&bufferUV[j * uvByteStride]) : glm::vec2(0.f));

					v.Joints = glm::vec4(0.f);
					v.Weights = glm::vec4(0.f);

					if (isSkinned)
					{
						if (jointComponentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
						{
							const uint16_t* buf = static_cast<const uint16_t*>(bufferJoints);
							v.Joints = glm::uvec4(glm::make_vec4(&buf[j * jointByteStride]));
						}
						else
						{
							const uint8_t* buf = static_cast<const uint8_t*>(bufferJoints);
							v.Joints = glm::ivec4(glm::make_vec4(&buf[j * jointByteStride]));
						}

						v.Weights = glm::make_vec4(&bufferWeights[j * weightByteStride]);
						if (glm::length(v.Weights) == 0.f)
							v.Weights = glm::vec4(1.f, 0.f, 0.f, 0.f);
					}

					m_LastVertex++;
				}

				if (primitive.indices > -1)
				{
					const auto& accessor = m_LoadedModel.accessors[primitive.indices > -1 ? primitive.indices : 0];
					const auto& bufferView = m_LoadedModel.bufferViews[accessor.bufferView];
					const auto& buffer = m_LoadedModel.buffers[bufferView.buffer];

					indexCount = static_cast<uint32_t>(accessor.count);
					const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT)
					{
						const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							m_Indices[m_LastIndex++] = buf[index] + vertexStart;
						}
					}

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT)
					{
						const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							m_Indices[m_LastIndex++] = buf[index] + vertexStart;
						}
					}

					if (accessor.componentType == TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE)
					{
						const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
						for (size_t index = 0; index < accessor.count; index++)
						{
							m_Indices[m_LastIndex++] = buf[index] + vertexStart;
						}
					}
				}

				Primitive& newPrimitive = node->m_Mesh->m_Primitives[i];
				newPrimitive.m_MaterialIndex = primitive.material > -1 ? primitive.material : 0; // Default to first material...
				newPrimitive.m_IndexOffset = indexStart;
				newPrimitive.m_IndexCount = indexCount;
				newPrimitive.m_VertexCount = vertexCount;

				newPrimitive.m_Bounds.m_Mins = glm::min(posMin, newPrimitive.m_Bounds.m_Mins);
				newPrimitive.m_Bounds.m_Maxs = glm::max(posMax, newPrimitive.m_Bounds.m_Maxs);
			}
		}

		if (parent)
		{
			parent->m_Children.push_back(node);
		}
		else
		{
			m_Nodes.push_back(node);
		}
	}

	void SkinnedGLTFAsset::UpdateAnimation(int animIndex, float dt)
	{
		if (m_Animations.empty())
			return;

		Animation& animation = m_Animations[animIndex];

		bool updated = animation.Update(dt);

		if (updated)
		{
			for (auto& Node : m_Nodes)
				UpdateJoints(Node);
		}
	}

	void SkinnedGLTFAsset::UpdateJoints(Node* node)
	{
		node->m_UseCachedMatrix = false;

		if (node->m_SkinIndex > -1)
		{
			auto& skin = m_Skins[node->m_SkinIndex];

			std::vector<glm::mat4> jointMatrices(skin.m_Joints.size());
			for (auto i = 0; i < jointMatrices.size(); i++)
				jointMatrices[i] = skin.m_Joints[i]->GetMatrix() * skin.m_InverseBindMatrices[i];

			skin.m_Buffer->Patch(jointMatrices.data(), jointMatrices.size() * sizeof(glm::mat4));
		}

		for (auto& child : node->m_Children)
			UpdateJoints(child);
	}

	void SkinnedGLTFAsset::LoadAnimations()
	{
		m_Animations.resize(m_LoadedModel.animations.size());

		for (auto i = 0; i < m_LoadedModel.animations.size(); i++)
		{
			auto& animation = m_LoadedModel.animations[i];

			Animation& animData = m_Animations[i];

			animData.m_Name = animation.name;
			animData.m_Samplers.resize(animation.samplers.size());
			for (size_t j = 0; j < animation.samplers.size(); j++)
			{
				auto& sampler = animation.samplers[j];

				AnimationSampler& samplerData = animData.m_Samplers[j];
				samplerData.m_Interpolation = sampler.interpolation;

				const auto& inputAccessor = m_LoadedModel.accessors[sampler.input];
				const auto& inputBufferView = m_LoadedModel.bufferViews[inputAccessor.bufferView];
				const auto& inputBuffer = m_LoadedModel.buffers[inputBufferView.buffer];
				const void* inputData = &inputBuffer.data[inputAccessor.byteOffset + inputBufferView.byteOffset];

				const float* buf = static_cast<const float*>(inputData);

				for (auto inputIdx = 0; inputIdx < inputAccessor.count; inputIdx++)
					samplerData.m_Inputs.push_back(buf[inputIdx]);

				for (auto input : m_Animations[i].m_Samplers[j].m_Inputs)
				{
					if (input < m_Animations[i].m_Start)
						m_Animations[i].m_Start = input;
					if (input > m_Animations[i].m_End)
						m_Animations[i].m_End = input;
				}

				const auto& outputAccessor = m_LoadedModel.accessors[sampler.output];
				const auto& outputBufferView = m_LoadedModel.bufferViews[outputAccessor.bufferView];
				const auto& buffer = m_LoadedModel.buffers[outputBufferView.buffer];
				const void* outputData = &buffer.data[outputAccessor.byteOffset + outputBufferView.byteOffset];

				if (outputAccessor.type == TINYGLTF_TYPE_VEC3)
				{
					const glm::vec3* buf = static_cast<const glm::vec3*>(outputData);
					for (size_t index = 0; index < outputAccessor.count; index++)
						samplerData.m_Outputs.push_back(glm::vec4(buf[index].x, buf[index].y, buf[index].z, 0.0f));
				}

				if (outputAccessor.type == TINYGLTF_TYPE_VEC4)
				{
					const glm::vec4* buf = static_cast<const glm::vec4*>(outputData);
					for (size_t index = 0; index < outputAccessor.count; index++)
						samplerData.m_Outputs.push_back(buf[index]);
				}
			}

			m_Animations[i].m_Channels.resize(animation.channels.size());
			for (size_t j = 0; j < animation.channels.size(); j++)
			{
				auto& channel = animation.channels[j];
				auto& channelData = m_Animations[i].m_Channels[j];
				channelData.m_Sampler = channel.sampler;
				channelData.m_Type = GetChannelType(channel.target_path);
				channelData.m_Node = GetNodeByIndex(channel.target_node);
			}
		}
	}

	void SkinnedGLTFAsset::BuildIndirectBatches(std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool, const Renderer::DescriptorLayout& primitiveLayout)
	{
		m_IndirectCommands.clear();

		// Build the indirect commands.
		uint32_t m = 0;
		for (auto i = 0; i < m_AllNodes.size(); i++)
		{
			auto* node = m_AllNodes[i];

			if (node->m_Mesh)
			{
				for (const auto& primitive : node->m_Mesh->m_Primitives)
				{
					VkDrawIndexedIndirectCommand cmd{};
					IndirectPrimitiveData data{};

					const auto& bounds = primitive.m_Bounds;
					const auto origin = (bounds.m_Maxs + bounds.m_Mins) * 0.5f;
					const auto extents = (bounds.m_Maxs - bounds.m_Mins) * 0.5f;
					const auto radius = glm::length(extents);

					// TODO: Replace with model matrix.
					glm::mat4 inv = glm::mat4(1.f);
					inv[2][2] *= -1.f;

					data.NodeMatrix = m_WorldMatrix;
					data.MaterialIndex.x = primitive.m_MaterialIndex;
					data.MaterialIndex.y = node->m_Index;
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

		m_IndirectCommandsBuffer = new Renderer::Buffer(device, cmdBufferSize,
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
		);

		m_PrimitiveStorageBuffer = new Renderer::Buffer(device, storageBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			VK_SHARING_MODE_EXCLUSIVE,
			VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT
		);

		m_PrimitiveStorageBuffer->Patch(m_PerPrimitiveData.data(), m_PerPrimitiveData.size() * sizeof(IndirectPrimitiveData));

		commandBuffer->Begin();
		commandBuffer->CopyBuffer(*cmdStagingBuffer, *m_IndirectCommandsBuffer, static_cast<VkDeviceSize>(cmdBufferSize));
		// commandBuffer->CopyBuffer( *storageStagingBuffer, *m_PrimitiveStorageBuffer, static_cast< VkDeviceSize >( storageBufferSize ) );
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

		// m_IndirectBufferDescriptor = new Renderer::Descriptor( device,
		// 													   indirectLayout,
		// 													   VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		// );
		// 
		// m_IndirectBufferDescriptor->Bind(
		// 	{
		// 		Renderer::Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, {.m_Buffer = m_IndirectCommandsBuffer }}
		// 	}
		// );
	}

	void SkinnedGLTFAsset::LoadSkins()
	{
		if (m_LoadedModel.skins.empty())
			return;

		m_Skins.resize(m_LoadedModel.skins.size());

		for (auto i = 0; i < m_LoadedModel.skins.size(); i++)
		{
			const auto& gltfSkin = m_LoadedModel.skins[i];

			if (gltfSkin.skeleton > -1)
				m_Skins[i].m_SkeletonRootJoint = GetNodeByIndex(gltfSkin.skeleton);

			for (const int jointIndex : gltfSkin.joints)
			{
				Node* node = GetNodeByIndex(jointIndex);
				if (!node)
					continue;

				m_Skins[i].m_Joints.push_back(node);
			}

			if (gltfSkin.inverseBindMatrices > -1)
			{
				const auto& inverseBindMatricesAccessor = m_LoadedModel.accessors[gltfSkin.inverseBindMatrices];
				const auto& inverseBindMatricesBufferView = m_LoadedModel.bufferViews[inverseBindMatricesAccessor.bufferView];

				const auto& buffer = m_LoadedModel.buffers[inverseBindMatricesBufferView.buffer];
				m_Skins[i].m_InverseBindMatrices.resize(inverseBindMatricesAccessor.count);

				memcpy(m_Skins[i].m_InverseBindMatrices.data(), &buffer.data[inverseBindMatricesAccessor.byteOffset + inverseBindMatricesBufferView.byteOffset], inverseBindMatricesAccessor.count * sizeof(glm::mat4));

				m_Skins[i].m_Buffer = new Renderer::Buffer(m_Device, sizeof(glm::mat4) * m_Skins[i].m_InverseBindMatrices.size(),
					VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
					VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);
			}
		}
	}

	void SkinnedGLTFAsset::Render(Renderer::CommandBuffer& commandBuffer, Renderer::Pipeline* pipeline, const std::vector<Renderer::Descriptor*>& sceneDescriptors, int bufferIndex)
	{
		for (auto& primData : m_PerPrimitiveData)
			primData.NodeMatrix = m_WorldMatrix;

		m_PrimitiveStorageBuffer->Patch(m_PerPrimitiveData.data(), m_PerPrimitiveData.size() * sizeof(IndirectPrimitiveData));

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *pipeline);

		std::vector<Renderer::Descriptor*> descriptors = sceneDescriptors;

		// TODO: Add Per Node UBO.
		std::vector<Renderer::Descriptor*> assetDescriptors = { m_MaterialBufferDescriptor, m_TextureBufferDescriptor, m_PrimitiveBufferDescriptor, m_JointDescriptor };

		descriptors.insert(descriptors.end(), assetDescriptors.cbegin(), assetDescriptors.cend());

		commandBuffer.BindDescriptors(descriptors);
		commandBuffer.SetDescriptorOffsets(descriptors, *pipeline);

		const VkDeviceSize offsets[1] = { 0 };
		const VkBuffer vertexBuffer = *m_VertexBuffer;
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);
		vkCmdBindIndexBuffer(commandBuffer, *m_IndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// TODO: Check if device supports MultiDrawIndirect.
		vkCmdDrawIndexedIndirect(commandBuffer, *m_IndirectCommandsBuffer, 0, static_cast<uint32_t>(m_IndirectCommands.size()), sizeof(VkDrawIndexedIndirectCommand));
	}

	void SkinnedGLTFAsset::SetupDevice(const std::vector<Renderer::DescriptorLayout>& descriptorLayouts)
	{
		LoadSkins();
		LoadAnimations();

		CreateGeometryBuffers(m_Device, m_CommandPool);
		LoadMaterials(m_Device, m_CommandPool, descriptorLayouts[0]);
		LoadTextures(m_Device, m_CommandPool, descriptorLayouts[1]);
		LoadBoneData(m_Device);
		BuildIndirectBatches(m_Device, m_CommandPool, descriptorLayouts[2]);
	}

	void SkinnedGLTFAsset::LoadBoneData(std::shared_ptr<Renderer::Device> device)
	{
		m_JointDescriptor = new Renderer::Descriptor(
			device,
			{
				{ 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, nullptr }
			},
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		m_JointDescriptor->Bind(
			{
				Renderer::Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, {.m_Buffer = m_Skins[0].m_Buffer } }
			}
		);
	}

	bool SkinnedGLTFAsset::Animation::Update( float dt )
	{
		if ( !m_IsPaused )
			m_CurTime += dt;

		while ( m_CurTime > m_End )
		{
			m_CurTime -= ( m_End - m_Start );
		}

		bool updated = false;
		for ( auto& channel : m_Channels )
		{
			const auto& sampler = m_Samplers[ channel.m_Sampler ];
			const auto& inputs = sampler.m_Inputs;
			const auto& outputs = sampler.m_Outputs;

			for ( auto i = 0; i < inputs.size( ) - 1; i++ )
			{
				if ( ( m_CurTime >= inputs[ i ] ) && ( m_CurTime <= inputs[ i + 1 ] ) )
				{
					float a = ( m_CurTime - inputs[ i ] ) / ( inputs[ i + 1 ] - inputs[ i ] );

					if ( channel.m_Type == ChannelType::Translation )
					{
						glm::vec3 t1 = outputs[ i ];
						glm::vec3 t2 = outputs[ i + 1 ];

						channel.m_Node->m_Translation = glm::mix(channel.m_Node->m_Translation, glm::mix(t1, t2, a), a * 0.5f);
					}

					if (channel.m_Type == ChannelType::Rotation)
					{
						glm::quat q1(outputs[i].w, outputs[i].x, outputs[i].y, outputs[i].z);
						glm::quat q2(outputs[i + 1].w, outputs[i + 1].x, outputs[i + 1].y, outputs[i + 1].z);

						channel.m_Node->m_Rotation = glm::slerp(channel.m_Node->m_Rotation, glm::slerp(q1, q2, a), a * 0.5f);
						channel.m_Node->m_Rotation = glm::normalize(channel.m_Node->m_Rotation);
					}

					if (channel.m_Type == ChannelType::Scale)
					{
						auto lerped = glm::vec3(glm::mix(outputs[i], outputs[i + 1], a));
						channel.m_Node->m_Scale = glm::mix(channel.m_Node->m_Scale, lerped, a * 0.5f);
					}

					updated = true;
					break;
				}
			}
		}

		return updated;
	}
}