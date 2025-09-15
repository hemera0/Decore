#include "Scene.hpp"

#include "../../Core/Core.hpp"
#include "../../Assets/Assets.hpp"

#include "../../../../Dependencies/imgui/backends/imgui_impl_vulkan.h"

namespace Engine::Renderer
{
	VkPipelineCache pipelineCache = 0;

	Scene::Scene(
		std::shared_ptr<Device> device, 
		std::shared_ptr<CommandPool> commandPool, 
		const std::unique_ptr<Swapchain>& swapchain, 
		const std::vector<DescriptorLayout>& objectLayouts
	) :
		m_Device(device),
		m_CommandPool(commandPool),
		m_Swapchain(swapchain.get())
	{
		Setup(device, commandPool, swapchain, objectLayouts);
	}

	size_t debugVertexBufferSize = 0;

	void Scene::Setup(
		std::shared_ptr<Device> device,
		std::shared_ptr<CommandPool> commandPool,
		const std::unique_ptr<Swapchain>& swapchain,
		const std::vector<DescriptorLayout>& objectLayouts
	)
	{
		VkFormat depthFormat = swapchain->GetDepthFormat();
		std::vector<VkFormat> colorFormats = { swapchain->GetHDRFormat() };

		m_MainCamera = Core::Camera(Core::CameraType::Attachment);
		m_ActiveEnvironment = new EnvironmentInfo(device, commandPool, "C:\\TestAssets\\IBL\\Pure");

		m_ShadowMapPass = new ShadowMapPass(device, swapchain, objectLayouts);

		m_SceneUniforms = new Buffer(device, sizeof(SceneUniforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT);

		m_SceneDescriptor = new Descriptor(
			device,
			{
				{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_VERTEX_BIT, nullptr }
			},
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		m_SceneDescriptor->Bind(
			{
				Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {.m_Buffer = m_SceneUniforms } }
			}
		);

		m_PrePassDepthSampler = new Sampler(m_Device, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_FILTER_LINEAR, 1.f);

		SetupSSAOPass();
		SetupBloomPasses();

		std::vector<VkDescriptorSetLayout> setLayouts = {
			m_SceneDescriptor->GetLayout(),
			m_ActiveEnvironment->GetDescriptor()->GetLayout(),
			m_ShadowMapPass->GetDescriptor()->GetLayout(),
			m_ShadowMapPass->GetCascadeDescriptor()->GetLayout(),
			m_SSAOImageDescriptor->GetLayout()
		};

		for (auto& layout : objectLayouts)
			setLayouts.push_back(layout.GetLayout());

		SetupDetphPrepass(colorFormats, depthFormat, objectLayouts, setLayouts);
		// Setup skybox.
		SetupSkybox(colorFormats, depthFormat, objectLayouts, setLayouts);

		auto msaaMultisampleState = Pipeline::SetupMultiSampleState();
		msaaMultisampleState.rasterizationSamples = m_Swapchain->GetMSAASamples();

		m_OpaqueGLTFPipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\GLTF_StaticVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\DecorePBR_Standard.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats(colorFormats)
			.SetDepthAttachmentFormat(depthFormat)
			.SetInputAttributeDescriptions(Engine::Assets::StaticGLTFAsset::VertexType::GetInputAttributeDescriptions())
			.SetInputBindingDescriptions({ Engine::Assets::StaticGLTFAsset::VertexType::GetBindingDescription() })
			.SetDescriptorSetLayouts(setLayouts)
			.SetPushConstants({ VkPushConstantRange{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) } })
			.SetMultisampleState(msaaMultisampleState)
			.Build(*m_Swapchain);
		
		m_PrePassOpaqueGLTFPipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\GLTF_StaticVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\PrePass\\StaticPrepassPS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats({ m_Swapchain->GetImageFormat() })
			.SetDepthAttachmentFormat(depthFormat)
			.SetInputAttributeDescriptions(Engine::Assets::StaticGLTFAsset::VertexType::GetInputAttributeDescriptions())
			.SetInputBindingDescriptions({ Engine::Assets::StaticGLTFAsset::VertexType::GetBindingDescription() })
			.SetDescriptorSetLayouts(setLayouts)
			.Build(*m_Swapchain);

		auto skinnedJointDescriptor = Renderer::DescriptorLayout(device, { { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_ALL, nullptr} });

		setLayouts.push_back(skinnedJointDescriptor);

		m_SkinnedGLTFPipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\GLTF_SkinnedVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\DecorePBR_Standard.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats(colorFormats)
			.SetDepthAttachmentFormat(depthFormat)
			.SetInputAttributeDescriptions(Engine::Assets::SkinnedGLTFAsset::VertexType::GetInputAttributeDescriptions())
			.SetInputBindingDescriptions({ Engine::Assets::SkinnedGLTFAsset::VertexType::GetBindingDescription() })
			.SetDescriptorSetLayouts(setLayouts)
			.SetPushConstants({ VkPushConstantRange{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) } })
			.SetMultisampleState(msaaMultisampleState)
			.Build(*m_Swapchain);

		m_PrePassSkinnedGLTFPipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\GLTF_SkinnedVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\PrePass\\SkinnedPrepassPS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats({ m_Swapchain->GetImageFormat() })
			.SetDepthAttachmentFormat(depthFormat)
			.SetInputAttributeDescriptions(Engine::Assets::SkinnedGLTFAsset::VertexType::GetInputAttributeDescriptions())
			.SetInputBindingDescriptions({ Engine::Assets::SkinnedGLTFAsset::VertexType::GetBindingDescription() })
			.SetDescriptorSetLayouts(setLayouts)
			.SetPushConstants({ VkPushConstantRange{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t) } })
			.Build(*m_Swapchain);

		auto collisionRasterizationState = Pipeline::SetupRasterizationState();
		collisionRasterizationState.cullMode = VK_CULL_MODE_NONE;

		auto collisionInputAssembly = Pipeline::SetupInputAssemblyState();
		collisionInputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

		m_CollisionDebugPipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\CollisionDebugVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\CollisionDebugPS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats(colorFormats)
			.SetDepthAttachmentFormat(depthFormat)
			.SetInputAttributeDescriptions({ VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 } })
			.SetInputBindingDescriptions({ VkVertexInputBindingDescription{ 0, sizeof(glm::vec3), VK_VERTEX_INPUT_RATE_VERTEX } })
			.SetPushConstants({ VkPushConstantRange{ VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4) * 2 } })
			.SetMultisampleState(msaaMultisampleState)
			.SetRasterizationState(collisionRasterizationState)
			.SetInputAssemblyState(collisionInputAssembly)
			.Build(*m_Swapchain);

		m_RTSampler = new Sampler(m_Device);

		m_FullscreenDescriptor = new Descriptor(
			device,
			{ { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr } },
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		m_ApplyBloomFullscreenDescriptor = new Descriptor(
			device,
			{ { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr } },
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		std::vector<VkDescriptorSetLayout> fullscreenLayouts = {
			m_FullscreenDescriptor->GetLayout()
		};

		std::vector<VkDescriptorSetLayout> finalFullscreenLayouts = {
			m_FullscreenDescriptor->GetLayout(),
			m_ApplyBloomFullscreenDescriptor->GetLayout()
		};

		auto fullscreenRasterizationState = Pipeline::SetupRasterizationState();
		fullscreenRasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		fullscreenRasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		std::vector<VkFormat> bloomFormat = { swapchain->GetHDRFormat() };

		m_HDRBloomPipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\QuadVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\Bloom\\BloomCollectPS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats(bloomFormat)
			.SetDescriptorSetLayouts(fullscreenLayouts)
			.SetRasterizationState(fullscreenRasterizationState)
			.Build(*m_Swapchain);

		m_BloomDownscalePipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\QuadVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\Bloom\\BloomDownsamplePS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats(bloomFormat)
			.SetDescriptorSetLayouts(fullscreenLayouts)
			.SetPushConstants({ VkPushConstantRange{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec2) } })
			.SetRasterizationState(fullscreenRasterizationState)
			.Build(*m_Swapchain);

		VkPipelineColorBlendAttachmentState upscaleBlendAttachmentState = {};
		upscaleBlendAttachmentState.blendEnable = VK_TRUE;
		upscaleBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
		upscaleBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		upscaleBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
		upscaleBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		upscaleBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		upscaleBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;
		upscaleBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		m_BloomUpscalePipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\QuadVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\Bloom\\BloomUpsamplePS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats(bloomFormat)
			.SetDescriptorSetLayouts(fullscreenLayouts)
			.SetPushConstants({ VkPushConstantRange{ VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec2) } })
			.SetRasterizationState(fullscreenRasterizationState)
			.SetColorBlendAttachmentStates({ upscaleBlendAttachmentState })
			.Build(*m_Swapchain);

		m_FullscreenPipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\QuadVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\QuadPS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats({ m_Swapchain->GetImageFormat() })
			.SetDescriptorSetLayouts(finalFullscreenLayouts)
			.SetRasterizationState(fullscreenRasterizationState)
			.Build(*m_Swapchain);

		m_SceneDescriptors = {
			m_SceneDescriptor,
			m_ActiveEnvironment->GetDescriptor(),
			m_ShadowMapPass->GetDescriptor().get(),
			m_ShadowMapPass->GetCascadeDescriptor().get(),
			m_SSAOImageDescriptor
		};
	}

	Scene::~Scene()
	{
		delete m_ShadowMapPass;
		delete m_SkyboxPipeline;
		delete m_SceneDescriptor;
		delete m_SceneUniforms;
		delete m_SkyboxPipeline;
		delete m_OpaqueGLTFPipeline;
		delete m_SkinnedGLTFPipeline;
		delete m_ActiveEnvironment;
		delete m_SkyCube;
	}

	void Scene::SetupSkybox( const std::vector<VkFormat>& colorFormats, VkFormat depthFormat, const std::vector<DescriptorLayout>& objectLayouts, const std::vector<VkDescriptorSetLayout>& setLayouts )
	{
		auto depthState = Pipeline::SetupDepthStencilState( );
		auto skyRasterizationState = Pipeline::SetupRasterizationState( );
		auto msaaMultisampleState = Pipeline::SetupMultiSampleState();

		depthState.depthTestEnable = VK_FALSE;
		depthState.depthWriteEnable = VK_FALSE;

		skyRasterizationState.cullMode = VK_CULL_MODE_FRONT_BIT;
		skyRasterizationState.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		msaaMultisampleState.rasterizationSamples = m_Swapchain->GetMSAASamples();

		m_SkyboxPipeline = PipelineBuilder()
			.SetShaders(
				{
					ShaderRegistry::Get()->Register("..\\Shaders\\SkyVS.hlsl", VK_SHADER_STAGE_VERTEX_BIT),
					ShaderRegistry::Get()->Register("..\\Shaders\\SkyPS.hlsl", VK_SHADER_STAGE_FRAGMENT_BIT)
				})
			.SetColorAttachmentFormats(colorFormats)
			.SetDepthAttachmentFormat(depthFormat)
			.SetInputAttributeDescriptions(Engine::Assets::StaticGLTFAsset::VertexType::GetInputAttributeDescriptions())
			.SetInputBindingDescriptions({ Engine::Assets::StaticGLTFAsset::VertexType::GetBindingDescription() })
			.SetDescriptorSetLayouts(setLayouts)
			.SetDepthStencilState(depthState)
			.SetRasterizationState(skyRasterizationState)
			.SetMultisampleState(msaaMultisampleState)
			.SetColorBlendAttachmentStates({ Pipeline::SetupDefaultColorBlendAttachmentState() })
			.Build(*m_Swapchain);

		auto skyCube = new Assets::StaticGLTFAsset( "C:\\TestAssets\\IBL\\SkyCube.gltf", m_Device, m_CommandPool );
		skyCube->SetupDevice( objectLayouts );

		m_SkyCube = skyCube;
	}

	void Scene::SetupDetphPrepass(const std::vector<VkFormat>& colorFormats, VkFormat depthFormat, const std::vector<DescriptorLayout>& objectLayouts, const std::vector<VkDescriptorSetLayout>& setLayouts)
	{
		m_ImguiDepthPrePass = ImGui_ImplVulkan_AddTexture( *m_PrePassDepthSampler, *( m_Swapchain->m_PrePassDepthImageView ), VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL );
	}

	void Scene::SetupSSAOPass()
	{
		const auto& extents = m_Swapchain->GetExtents();
		const auto ssaoImageFormat = VK_FORMAT_R32G32_SFLOAT;

		m_SSAOImageWidth = extents.width;
		m_SSAOImageHeight = extents.height;

		m_SSAOImage = new Image(m_Device, m_SSAOImageWidth, m_SSAOImageHeight, 1, ssaoImageFormat, VK_IMAGE_TILING_OPTIMAL, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		m_SSAOImageView = new ImageView(m_Device, *m_SSAOImage, 1, ssaoImageFormat);


		m_SSAOImageCreateInfo = Image::GetDefault2DCreateInfo(m_SSAOImageWidth, m_SSAOImageHeight, 1, VK_FORMAT_R32_SFLOAT, VK_IMAGE_TILING_OPTIMAL, VK_SAMPLE_COUNT_1_BIT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT);
		m_SSAOImageDescriptor = new Descriptor(m_Device,
			{
				{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr }
			},
			VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);

		const auto& physicalDevice = m_Device->GetPhysicalDevice();

		size_t ffxCacaoContextSize = FFX_CACAO_VkGetContextSize();
		m_CacaoContext = (FFX_CACAO_VkContext*)malloc(ffxCacaoContextSize);

		FFX_CACAO_VkCreateInfo info = {};
		info.physicalDevice = *physicalDevice;
		info.device = *m_Device;
		info.flags = FFX_CACAO_VK_CREATE_USE_16_BIT | FFX_CACAO_VK_CREATE_USE_DEBUG_MARKERS | FFX_CACAO_VK_CREATE_NAME_OBJECTS;
		FFX_CACAO_Status status = FFX_CACAO_VkInitContext(m_CacaoContext, &info);

		FFX_CACAO_VkScreenSizeInfo screenSizeInfo = {};
		screenSizeInfo.width = extents.width/* width of the input/output buffers */;
		screenSizeInfo.height = extents.height/* height of the input/output buffers */;
		screenSizeInfo.depthView = *(m_Swapchain->m_PrePassDepthImageView) /* a VkImageView for the depth buffer, should be in layout VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL */;
		screenSizeInfo.normalsView = VK_NULL_HANDLE/* an optional VkImageView for the normal buffer (VK_NULL_HANDLE if not provided), should be in layout VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL */;
		screenSizeInfo.output = *m_SSAOImage/* a VkImage for writing the output of FFX CACAO */;
		screenSizeInfo.outputView = *m_SSAOImageView/* a VkImageView corresponding to the VkImage for writing the output of FFX CACAO */;
		status = FFX_CACAO_VkInitScreenSizeDependentResources(m_CacaoContext, &screenSizeInfo);
	}

	void Scene::SetupBloomPasses( )
	{
		m_BloomSampler = new Sampler( m_Device );

		CreateBloomImages( true );
	}

	void Scene::DestroyBloomImages()
	{
		m_DownsampleImageViews.clear();
		m_UpsampleImageViews.clear();

		for (auto i = 0; i < m_EmissionImageViews.size(); i++) {
			delete m_EmissionImageViews[i];
			m_EmissionImageViews[i] = nullptr;
		}
	}

	void Scene::CreateBloomImages(bool createDescriptors)
	{
		constexpr const uint32_t mipChainLength = 5;

		const auto bloomFormat = m_Swapchain->GetHDRFormat();
		const auto& extents = m_Swapchain->GetExtents();

		m_BloomImageWidth = extents.width;
		m_BloomImageHeight = extents.height;

		m_BloomImage = new Image(
			m_Device,
			m_BloomImageWidth,
			m_BloomImageHeight,
			mipChainLength,
			bloomFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_SAMPLE_COUNT_1_BIT,
			VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		);

		m_BloomImageView = new ImageView(m_Device, *m_BloomImage, mipChainLength, bloomFormat);

		uint32_t mipWidth = m_BloomImageWidth;
		uint32_t mipHeight = m_BloomImageHeight;

		m_EmissionImageViews.resize(mipChainLength);
		
		if (createDescriptors)
			m_BloomMipDescriptors.resize(mipChainLength);

		for (auto i = 0; i < mipChainLength; i++)
		{
			VkImageViewCreateInfo createInfo = ImageView::GetDefault2DCreateInfo(*m_BloomImage, 1, bloomFormat);
			createInfo.subresourceRange.baseMipLevel = i;

			m_EmissionImageViews[i] = new ImageView(m_Device, *m_BloomImage, 1, bloomFormat, &createInfo);
		}

		for (auto i = 0; i < m_BloomMipDescriptors.size(); i++)
		{
			if (createDescriptors) {
				m_BloomMipDescriptors[i] = new Descriptor(
					m_Device,
					{ { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT, nullptr } },
					VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
				);
			}

			m_BloomMipDescriptors[i]->Bind(
				{
					Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {.m_ImageView = m_EmissionImageViews[i] }, m_BloomSampler }
				}
			);
		}

		uint32_t mipLevel = 1;
		for (uint32_t i = 0; i < mipChainLength - 1; ++i)
		{
			m_DownsampleImageViews.push_back(m_EmissionImageViews[mipLevel]);
			++mipLevel;
		}

		mipLevel = mipChainLength - 2;
		for (uint32_t i = 0; i < mipChainLength - 1; ++i)
		{
			m_UpsampleImageViews.push_back(m_EmissionImageViews[mipLevel]);
			--mipLevel;
		}
	}

	bool Scene::RenderCollisions()
	{
		auto test1 = (Assets::StaticGLTFAsset*)(m_SceneModels[0]);

		std::vector<glm::vec3> vertices{};

		static bool debuggingColliders;
		if (Core::InputSystem::GetKeyPressed('X'))
			debuggingColliders = !debuggingColliders;

		for (auto node : test1->m_AllNodes)
		{
			if (node->m_Mesh)
			{
				for (auto& primitive : node->m_Mesh->m_Primitives)
				{
					glm::vec3 testmins = primitive.m_Bounds.m_Mins;
					glm::vec3 testmaxs = primitive.m_Bounds.m_Maxs;

					glm::mat4 m = glm::mat4(1.f);
					m[2][2] *= -1.f;

					testmins = glm::vec3(node->GetMatrix() * glm::vec4(testmins, 1.f));
					testmins = glm::vec3(m * glm::vec4(testmins, 1.f));

					testmaxs = glm::vec3(node->GetMatrix() * glm::vec4(testmaxs, 1.f));
					testmaxs = glm::vec3(m * glm::vec4(testmaxs, 1.f));

					Core::CollisionBox bbox(testmins, testmaxs);

					const auto& mins = bbox.GetMins();
					const auto& maxs = bbox.GetMaxs();

					std::array<glm::vec3, 24> boxLines = {
						mins, { mins.x, mins.y, maxs.z },
						mins, { maxs.x, mins.y, mins.z },
						{ maxs.x, mins.y, mins.z }, { maxs.x, mins.y, maxs.z },
						{ mins.x, mins.y, maxs.z }, { maxs.x, mins.y, maxs.z },
						{ maxs.x, mins.y, mins.z }, { maxs.x, maxs.y, mins.z },
						{ maxs.x, mins.y, maxs.z }, maxs,
						{ mins.x, mins.y, maxs.z }, { mins.x, maxs.y, maxs.z },
						{ mins.x, maxs.y, mins.z }, mins,
						{ mins.x, maxs.y, mins.z }, { mins.x, maxs.y, maxs.z },
						{ mins.x, maxs.y, mins.z }, { maxs.x, maxs.y, mins.z },
						{ maxs.x, maxs.y, maxs.z }, { mins.x, maxs.y, maxs.z },
						{ maxs.x, maxs.y, maxs.z }, { maxs.x, maxs.y, mins.z },
					};

					vertices.insert(vertices.end(), boxLines.begin(), boxLines.end());
				}
			}
		}

		{
			auto test2 = (Assets::SkinnedGLTFAsset*)(m_SkinnedSceneModels[0]);
			glm::vec3 playerPos = test2->m_WorldMatrix[3];
			glm::vec3 boxSize = glm::vec3(0.25f, 0.5f, 0.25f);
			glm::vec3 boxOffset = glm::vec3(0.f, 0.75f, 0.f);

			// Core::CollisionBox playerAABB( playerPos - boxSize, playerPos + boxSize );

			const auto& mins = playerPos + boxOffset - boxSize; //  playerAABB.GetMins( );
			const auto& maxs = playerPos + boxOffset + boxSize;

			std::array<glm::vec3, 24> playerboxLines = {
				mins, { mins.x, mins.y, maxs.z },
				mins, { maxs.x, mins.y, mins.z },
				{ maxs.x, mins.y, mins.z }, { maxs.x, mins.y, maxs.z },
				{ mins.x, mins.y, maxs.z }, { maxs.x, mins.y, maxs.z },
				{ maxs.x, mins.y, mins.z }, { maxs.x, maxs.y, mins.z },
				{ maxs.x, mins.y, maxs.z }, maxs,
				{ mins.x, mins.y, maxs.z }, { mins.x, maxs.y, maxs.z },
				{ mins.x, maxs.y, mins.z }, mins,
				{ mins.x, maxs.y, mins.z }, { mins.x, maxs.y, maxs.z },
				{ mins.x, maxs.y, mins.z }, { maxs.x, maxs.y, mins.z },
				{ maxs.x, maxs.y, maxs.z }, { mins.x, maxs.y, maxs.z },
				{ maxs.x, maxs.y, maxs.z }, { maxs.x, maxs.y, mins.z },
			};

			vertices.insert(vertices.end(), playerboxLines.begin(), playerboxLines.end());
		}

		if (vertices.size() * sizeof(glm::vec3) > debugVertexBufferSize)
		{
			if (m_CollisionDebugVertexBuffer)
			{
				delete m_CollisionDebugVertexBuffer;
			}

			debugVertexBufferSize = sizeof(glm::vec3) * vertices.size();

			m_CollisionDebugVertexBuffer = new Buffer(m_Device, sizeof(glm::vec3) * vertices.size(), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT, VK_SHARING_MODE_EXCLUSIVE);
		}

		m_CollisionDebugVertexBuffer->Patch(vertices.data(), sizeof(glm::vec3) * vertices.size());

		return debuggingColliders;
	}

	void Scene::Render(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer, std::function<void()> callback)
	{
		PreRender(frameId, commandBuffer, computeCommandBuffer, callback);

		RenderDepthPrepass(frameId, commandBuffer, computeCommandBuffer);
		RenderSSAOPass(frameId, commandBuffer, computeCommandBuffer);

		// Main Forward Color Pass
		{
			ImageView* depthTarget = m_Swapchain->m_DepthImageView;

			Image* msaaImage = m_Swapchain->m_MSAAImage;
			ImageView* msaaTarget = m_Swapchain->m_MSAAImageView;

			Image* hdrImage = m_Swapchain->m_HDRImage;
			ImageView* hdrTarget = m_Swapchain->m_HDRImageView;

			VkClearValue clearValue = { { 0.1f, 0.1f, 0.1f, 1.f } };
			VkRenderingAttachmentInfo colorAttachment = RenderPassSpecification::GetColorAttachmentInfo(*msaaTarget, &clearValue, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL, VK_RESOLVE_MODE_AVERAGE_BIT, *hdrTarget, VK_IMAGE_LAYOUT_GENERAL);
			VkRenderingAttachmentInfo depthAttachment = RenderPassSpecification::GetDepthAttachmentInfo(*depthTarget, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

			const auto& extents = m_Swapchain->GetExtents();

			const auto objectDraw =
				[&](const std::vector<Descriptor*>& _sceneDescriptors, Renderer::Pipeline* _pipeline, bool isStatic, int bufferIndex) -> void
				{
					if (isStatic)
					{
						for (auto& asset : m_SceneModels)
							asset->Render(commandBuffer, _pipeline, _sceneDescriptors, bufferIndex);
					}
					else
					{
						for (auto& asset : m_SkinnedSceneModels)
							asset->Render(commandBuffer, _pipeline, _sceneDescriptors, bufferIndex);
					}
				};

			m_ShadowMapPass->Render(commandBuffer, computeCommandBuffer, m_MainCamera, LightDirection, m_Culler, m_SceneModels, objectDraw);

			{
				VkRenderingInfo renderingInfo = RenderPassSpecification::CreateRenderingInfo(m_Swapchain->GetExtents(), &colorAttachment, &depthAttachment);

				commandBuffer.BeginRendering(&renderingInfo);
				commandBuffer.SetViewport(static_cast<float>(extents.width), static_cast<float>(extents.height));
				commandBuffer.SetScissor(extents.width, extents.height);

				{
					bool debuggingColliders = RenderCollisions();
					if (!debuggingColliders)
						m_SkyCube->Render(commandBuffer, m_SkyboxPipeline, m_SceneDescriptors);

					uint32_t ssaoEnabled = m_SSAOEnabled;
					vkCmdPushConstants(commandBuffer, m_OpaqueGLTFPipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(ssaoEnabled), &ssaoEnabled);

					// Draw Scene Objects to standard pass.
					if (!debuggingColliders)
						objectDraw(m_SceneDescriptors, m_OpaqueGLTFPipeline, true, 0);

					// Draw Skinned Scene Objects to standard pass.
					objectDraw(m_SceneDescriptors, m_SkinnedGLTFPipeline, false, 0);

					if (debuggingColliders)
					{
						vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_CollisionDebugPipeline);

						const VkDeviceSize offsets[1] = { 0 };
						const VkBuffer vertexBuffer = *m_CollisionDebugVertexBuffer;
						vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, offsets);

						struct {
							glm::mat4 m1;
							glm::mat4 m2;
						} data = {};

						data.m1 = m_MainCamera.GetViewMatrix();
						data.m2 = m_MainCamera.GetProjectionMatrix();

						vkCmdPushConstants(commandBuffer, m_CollisionDebugPipeline->GetPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(data), &data);
						vkCmdDraw(commandBuffer, static_cast<uint32_t>(debugVertexBufferSize), 1, 0, 0);
					}
				}

				commandBuffer.EndRendering();
			}

		}

		RenderBloomPass(frameId, commandBuffer, computeCommandBuffer);
		RenderFinalPass(frameId, commandBuffer, computeCommandBuffer);
		EndRender(frameId, commandBuffer, computeCommandBuffer, callback);

		// Transition image layout to be presentable.
		VkImage currentImage = m_Swapchain->GetImages()[frameId];

		commandBuffer.ImageBarrier(
			currentImage,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			0,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);
	}

	void Scene::RecreateSwapchainResources()
	{
		const auto& extents = m_Swapchain->GetExtents();

		DestroyBloomImages();
		CreateBloomImages(false);

		FFX_CACAO_VkScreenSizeInfo screenSizeInfo = {};
		screenSizeInfo.width = extents.width/* width of the input/output buffers */;
		screenSizeInfo.height = extents.height/* height of the input/output buffers */;
		screenSizeInfo.depthView = *(m_Swapchain->m_PrePassDepthImageView) /* a VkImageView for the depth buffer, should be in layout VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL */;
		screenSizeInfo.normalsView = VK_NULL_HANDLE/* an optional VkImageView for the normal buffer (VK_NULL_HANDLE if not provided), should be in layout VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL */;
		screenSizeInfo.output = *m_SSAOImage/* a VkImage for writing the output of FFX CACAO */;
		screenSizeInfo.outputView = *m_SSAOImageView/* a VkImageView corresponding to the VkImage for writing the output of FFX CACAO */;
		auto status = FFX_CACAO_VkInitScreenSizeDependentResources(m_CacaoContext, &screenSizeInfo);
	}

	void Scene::PreRender(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer, std::function<void()> callback)
	{
		// Update skinned mesh animations.
		for (auto& skinnedModel : m_SkinnedSceneModels)
		{
			if (!skinnedModel)
				continue;

			if (Assets::SkinnedGLTFAsset* asset = dynamic_cast<Assets::SkinnedGLTFAsset*>(skinnedModel))
				asset->UpdateAnimation(asset->m_ActiveAnimIndex, Core::TimeSystem::GetDeltaTime());
		}

		// Update Scene Uniforms.
		m_Uniforms.ModelMatrix = glm::mat4(1.f);
		m_Uniforms.ModelMatrix[2][2] *= -1.f;

		m_Uniforms.ViewMatrix = m_MainCamera.GetViewMatrix();
		m_Uniforms.ProjectionMatrix = m_MainCamera.GetProjectionMatrix();

		m_Uniforms.CamPos = glm::inverse(m_Uniforms.ViewMatrix)[3];
		m_Uniforms.LightDirection = LightDirection;
		m_Uniforms.LightColor = LightColor;

		m_SceneUniforms->Patch(&m_Uniforms, sizeof(m_Uniforms));
	
		if (Core::InputSystem::GetKeyPressed('C'))
			m_FreezeFrustum = !m_FreezeFrustum;

		if (!m_FreezeFrustum)
			m_Culler->Cull(m_MainCamera.GetViewMatrix(), m_MainCamera.GetProjectionMatrix(), m_MainCamera.GetNearClip(), m_MainCamera.GetFarClip(), computeCommandBuffer, m_SceneModels);
	}

	void Scene::EndRender(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer, std::function<void()> callback)
	{
		VkImage currentImage = m_Swapchain->GetImages()[frameId];
		ImageView* currentFrame = m_Swapchain->GetImageViews()[frameId];

		VkRenderingAttachmentInfo colorAttachment = RenderPassSpecification::GetColorAttachmentInfo(*currentFrame, nullptr, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

		const auto& extents = m_Swapchain->GetExtents();

		VkRenderingInfo renderingInfo = RenderPassSpecification::CreateRenderingInfo(m_Swapchain->GetExtents(), &colorAttachment, nullptr);

		commandBuffer.BeginRendering(&renderingInfo);
		commandBuffer.SetViewport(static_cast<float>(extents.width), static_cast<float>(extents.height));
		commandBuffer.SetScissor(extents.width, extents.height);

		callback();

		commandBuffer.EndRendering();
	}

	void Scene::RenderDepthPrepass( uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer )
	{
		Image* depthImage = m_Swapchain->m_PrePassDepthImage;
		ImageView* depthTarget = m_Swapchain->m_PrePassDepthImageView;

		Image* normalsImage = m_Swapchain->m_PrePassNormalsImage;
		ImageView* normalsTarget = m_Swapchain->m_PrePassNormalsImageView;

		VkClearValue clear = { { 0.f, 0.f, 0.f, 1.f } };

		VkRenderingAttachmentInfo depthAttachment = RenderPassSpecification::GetDepthAttachmentInfo( *depthTarget, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL );
		VkRenderingAttachmentInfo colorAttachment = RenderPassSpecification::GetColorAttachmentInfo( *normalsTarget, &clear, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL );

		commandBuffer.ImageBarrier(
			*depthImage,
			0,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
		);

		commandBuffer.ImageBarrier(
			*normalsImage,
			0,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			VK_IMAGE_LAYOUT_UNDEFINED,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);

		const auto& extents = m_Swapchain->GetExtents( );

		VkRenderingInfo renderingInfo = RenderPassSpecification::CreateRenderingInfo( extents, &colorAttachment, &depthAttachment );

		commandBuffer.BeginRendering( &renderingInfo );
		commandBuffer.SetViewport( static_cast< float >( extents.width ), static_cast< float >( extents.height ) );
		commandBuffer.SetScissor( extents.width, extents.height );

		const auto objectDraw =
			[ & ]( const std::vector<Descriptor*>& _sceneDescriptors, Renderer::Pipeline* _pipeline, bool isStatic, int bufferIndex ) -> void
		{
			if ( isStatic )
			{
				for ( auto& asset : m_SceneModels )
				{
					asset->Render( commandBuffer, _pipeline, _sceneDescriptors, bufferIndex );
				}
			}
			else
			{
				for ( auto& asset : m_SkinnedSceneModels )
				{
					asset->Render( commandBuffer, _pipeline, _sceneDescriptors, bufferIndex );
				}
			}
		};

		{
			// Draw Scene Objects to depth pre-pass.
			objectDraw( m_SceneDescriptors, m_PrePassOpaqueGLTFPipeline, true, 0 );

			// Draw Skinned Scene Objects to depth pre-pass.
			objectDraw( m_SceneDescriptors, m_PrePassSkinnedGLTFPipeline, false, 0 );
		}

		commandBuffer.EndRendering( );

		commandBuffer.ImageBarrier(
			*depthImage,
			VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
			0,
			VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_DEPTH_BIT, 0, 1, 0, 1 }
		);

		commandBuffer.ImageBarrier(
			*normalsImage,
			VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			0,
			VK_IMAGE_LAYOUT_GENERAL,
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		);
	}

	void Scene::RenderSSAOPass(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer)
	{
		FFX_CACAO_Settings settings = FFX_CACAO_DEFAULT_SETTINGS;
		settings.generateNormals = true;

		FFX_CACAO_Status status = FFX_CACAO_VkUpdateSettings( m_CacaoContext, &settings );

		FFX_CACAO_Matrix4x4 proj = {}, normalsToView = {};

		glm::mat4 projectionMatrix = m_MainCamera.GetProjectionMatrix( );
		projectionMatrix[ 1 ][ 1 ] *= -1.f;
		memcpy( &proj, glm::value_ptr( projectionMatrix ), sizeof( FFX_CACAO_Matrix4x4 ) );

		glm::mat4 identity = glm::mat4( 1.f );
		memcpy( &normalsToView, glm::value_ptr( identity ), sizeof( FFX_CACAO_Matrix4x4 ) );

		status = FFX_CACAO_VkDraw( m_CacaoContext, commandBuffer, &proj, &normalsToView );

		m_SSAOImageDescriptor->Bind(
			{
				Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {.m_ImageView = m_SSAOImageView }, m_PrePassDepthSampler }
			}
		);
	}

	void Scene::RenderBloomPass(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer)
	{
		VkImage currentImage = *m_Swapchain->m_HDRImage;
		ImageView* currentFrame = m_Swapchain->m_HDRImageView;

		Image* bloomImage = m_BloomImage;
		ImageView* bloomTarget = m_BloomImageView;

		const auto& extents = m_Swapchain->GetExtents();

		// First pass. Collect all HDR colors and store them in bloom texture.
		{
			VkClearValue clearValue = { { 0.1f, 0.1f, 0.1f, 1.f } };
			VkRenderingAttachmentInfo colorAttachment = RenderPassSpecification::GetColorAttachmentInfo(*bloomTarget, &clearValue, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

			commandBuffer.ImageBarrier(
				*bloomImage,
				0,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
			);

			m_FullscreenDescriptor->Bind(
				{
					Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {.m_ImageView = currentFrame }, m_RTSampler }
				}
			);

			VkRenderingInfo renderingInfo = RenderPassSpecification::CreateRenderingInfo(extents, &colorAttachment, nullptr);

			commandBuffer.BeginRendering(&renderingInfo);
			commandBuffer.SetViewport(static_cast<float>(extents.width), static_cast<float>(extents.height));
			commandBuffer.SetScissor(extents.width, extents.height);

			std::vector descs = { m_FullscreenDescriptor };

			commandBuffer.BindDescriptors(descs);
			commandBuffer.SetDescriptorOffsets(descs, *m_HDRBloomPipeline);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_HDRBloomPipeline);
			vkCmdDraw(commandBuffer, 3, 1, 0, 0);

			commandBuffer.EndRendering();

			commandBuffer.ImageBarrier(
				*bloomImage,
				VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				VK_ACCESS_SHADER_READ_BIT,
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
			);
		}

		// Second pass. Downsample.
		{
			uint32_t mipLevel = 0;
			for (auto i = 0; i < m_EmissionImageViews.size() - 1; ++i)
			{
				VkClearValue clearValue = { { 0.0f, 0.0f, 0.0f, 1.f } };
				VkRenderingAttachmentInfo colorAttachment = RenderPassSpecification::GetColorAttachmentInfo(*m_DownsampleImageViews[i], &clearValue, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

				VkExtent2D bloomExtents = { m_BloomImageWidth >> (mipLevel + 1), m_BloomImageHeight >> (mipLevel + 1) };

				VkRenderingInfo renderingInfo = RenderPassSpecification::CreateRenderingInfo(bloomExtents, &colorAttachment, nullptr);

				commandBuffer.ImageBarrier(
					*bloomImage,
					0,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 1, 0, 1 }
				);

				commandBuffer.BeginRendering(&renderingInfo);
				commandBuffer.SetViewport(static_cast<float>(bloomExtents.width), static_cast<float>(bloomExtents.height));
				commandBuffer.SetScissor(bloomExtents.width, bloomExtents.height);

				std::vector descs = { m_BloomMipDescriptors[mipLevel] };

				commandBuffer.BindDescriptors(descs);
				commandBuffer.SetDescriptorOffsets(descs, *m_BloomDownscalePipeline);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_BloomDownscalePipeline);

				glm::vec2 screenExtents = { bloomExtents.width, bloomExtents.height };

				vkCmdPushConstants(commandBuffer, m_BloomDownscalePipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec2), (void*)(&screenExtents));
				vkCmdDraw(commandBuffer, 3, 1, 0, 0);

				commandBuffer.EndRendering();

				commandBuffer.ImageBarrier(
					*bloomImage,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 1, 0, 1 }
				);

				++mipLevel;
			}

			mipLevel = m_EmissionImageViews.size() - 1;
			for (auto i = 0; i < m_EmissionImageViews.size() - 1; ++i)
			{
				VkClearValue clearValue = { { 0.0f, 0.0f, 0.0f, 1.f } };
				VkRenderingAttachmentInfo colorAttachment = RenderPassSpecification::GetColorAttachmentInfo(*m_UpsampleImageViews[i], &clearValue, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);

				VkExtent2D bloomExtents = { m_BloomImageWidth >> (mipLevel - 1), m_BloomImageHeight >> (mipLevel - 1) };

				VkRenderingInfo renderingInfo = RenderPassSpecification::CreateRenderingInfo(bloomExtents, &colorAttachment, nullptr);

				commandBuffer.ImageBarrier(
					*bloomImage,
					0,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 1, 0, 1 }
				);

				commandBuffer.BeginRendering(&renderingInfo);
				commandBuffer.SetViewport(static_cast<float>(bloomExtents.width), static_cast<float>(bloomExtents.height));
				commandBuffer.SetScissor(bloomExtents.width, bloomExtents.height);

				std::vector descs = { m_BloomMipDescriptors[mipLevel] };

				commandBuffer.BindDescriptors(descs);
				commandBuffer.SetDescriptorOffsets(descs, *m_BloomUpscalePipeline);

				vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_BloomUpscalePipeline);

				glm::vec2 screenExtents = { bloomExtents.width, bloomExtents.height };

				vkCmdPushConstants(commandBuffer, m_BloomUpscalePipeline->GetPipelineLayout(), VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(glm::vec2), (void*)(&screenExtents));
				vkCmdDraw(commandBuffer, 3, 1, 0, 0);

				commandBuffer.EndRendering();

				commandBuffer.ImageBarrier(
					*bloomImage,
					VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					VK_ACCESS_SHADER_READ_BIT,
					VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, mipLevel, 1, 0, 1 }
				);

				--mipLevel;
			}
		}

		//commandBuffer.ImageBarrier(
		//	*bloomImage,
		//	VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
		//	0,
		//	VK_IMAGE_LAYOUT_GENERAL,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		//	VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		//);
	}

	void Scene::RenderFinalPass(uint32_t frameId, CommandBuffer& commandBuffer, CommandBuffer& computeCommandBuffer)
	{
		VkImage currentImage = m_Swapchain->GetImages()[frameId];
		ImageView* currentFrame = m_Swapchain->GetImageViews()[frameId];

		Image* hdrImage = m_Swapchain->m_HDRImage;
		ImageView* hdrTarget = m_Swapchain->m_HDRImageView;

		const auto& extents = m_Swapchain->GetExtents();

		m_FullscreenDescriptor->Bind(
			{
				Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {.m_ImageView = hdrTarget }, m_RTSampler }
			}
		);

		m_ApplyBloomFullscreenDescriptor->Bind(
			{
				Descriptor::BindingInfo { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, {.m_ImageView = m_BloomImageView}, m_BloomSampler }
			}
		);

		VkClearValue clearValue = { { 0.1f, 0.1f, 0.1f, 1.f } };
		VkRenderingAttachmentInfo colorAttachment = RenderPassSpecification::GetColorAttachmentInfo(*currentFrame, &clearValue, VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL);
		VkRenderingInfo renderingInfo = RenderPassSpecification::CreateRenderingInfo(m_Swapchain->GetExtents(), &colorAttachment, nullptr);

		//commandBuffer.ImageBarrier(
		//	*hdrImage,
		//	0,
		//	VK_ACCESS_COLOR_ATTACHMENT_READ_BIT,
		//	VK_IMAGE_LAYOUT_GENERAL,
		//	VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
		//	VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
		//	VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		//);

		commandBuffer.BeginRendering(&renderingInfo);
		commandBuffer.SetViewport(static_cast<float>(extents.width), static_cast<float>(extents.height));
		commandBuffer.SetScissor(extents.width, extents.height);

		std::vector descs = { m_FullscreenDescriptor, m_ApplyBloomFullscreenDescriptor };

		commandBuffer.BindDescriptors(descs);
		commandBuffer.SetDescriptorOffsets(descs, *m_FullscreenPipeline);

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, *m_FullscreenPipeline);
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		commandBuffer.EndRendering();

		//commandBuffer.ImageBarrier(
		//	*hdrImage,
		//	0,
		//	0,
		//	VK_IMAGE_LAYOUT_GENERAL,
		//	VK_IMAGE_LAYOUT_GENERAL,
		//	VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
		//	VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
		//	VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 }
		//);
	}

	void Scene::RenderSceneObjects(CommandBuffer& commandBuffer, const std::vector<Descriptor*>& sceneDescriptors, Renderer::Pipeline* pipeline, bool isStatic, int bufferIndex)
	{

	}

	bool Scene::Intersects(const Core::CollisionCapsule& capsule) {
		bool intersectsAny = false;

		for (auto& model : m_SceneModels) {
			if (Assets::StaticGLTFAsset* asset = dynamic_cast<Assets::StaticGLTFAsset*>(model)) {
				intersectsAny = asset->Intersects(capsule);
			}
		}

		return intersectsAny;
	}

	bool Scene::Intersects( const Core::CollisionBox& aabb, glm::vec3& normal )
	{
		int intersectionCount = 0;
		for ( auto& model : m_SceneModels )
		{
			if ( Assets::StaticGLTFAsset* asset = dynamic_cast< Assets::StaticGLTFAsset* >( model ) )
			{
				if ( asset->Intersects( aabb, normal ) )
					intersectionCount++;
			}
		}

		return intersectionCount > 0;
	}
}