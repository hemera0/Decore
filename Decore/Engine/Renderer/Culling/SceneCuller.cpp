#include "SceneCuller.hpp"

namespace Engine::Renderer
{
	SceneCuller::SceneCuller( std::shared_ptr<Device> device, Swapchain* swapchain, const DescriptorLayout& primitiveLayout )
	{
		auto indirectLayout = Renderer::DescriptorLayout(device, { { 0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr} });

		const auto cullComputeShader = Shader( "../Shaders/Compute/CullFrustumCS.hlsl", VK_SHADER_STAGE_COMPUTE_BIT );

		for ( auto i = 0; i < m_CullDatas.size( ); i++ )
		{
			m_CullDatas[ i ] = new Buffer( device, sizeof( CullingUniforms ),
										   VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
										   VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
										   VK_SHARING_MODE_EXCLUSIVE, VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT );
		}

		m_Descriptor = new Descriptor(
			device,
			{
				{ 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT, nullptr }
			},
			VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT
		);


		m_Pipeline = new ComputePipeline( cullComputeShader, { indirectLayout, primitiveLayout, m_Descriptor->GetLayout( ) }, {}, *swapchain );
	}

	void SceneCuller::Cull( const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, const float nearClip, const float farClip, CommandBuffer& commandBuffer, std::vector<Assets::BaseAsset*> staticGeometry, int cascadeIndex )
	{
		CullingUniforms cullUniforms{};

		// from niagara - zeux 
		glm::mat4 projectionT = glm::transpose( projectionMatrix );

		const auto normalizePlane =
			[ & ]( const glm::vec4 p )
		{
			return p / glm::length( glm::vec3( p ) );
		};

		glm::vec4 frustumX = normalizePlane( projectionT[ 3 ] + projectionT[ 0 ] ); // x + w < 0
		glm::vec4 frustumY = normalizePlane( projectionT[ 3 ] + projectionT[ 1 ] ); // y + w < 0

		cullUniforms.Frustum[ 0 ] = frustumX.x;
		cullUniforms.Frustum[ 1 ] = frustumX.z;
		cullUniforms.Frustum[ 2 ] = frustumY.y;
		cullUniforms.Frustum[ 3 ] = frustumY.z;
		cullUniforms.CameraView = viewMatrix;
		cullUniforms.NearFar = glm::vec2( nearClip, farClip );

		auto index = cascadeIndex == -1 ? 0 : cascadeIndex + 1;

		m_CullDatas[ index ]->Patch( &cullUniforms, sizeof( cullUniforms ) );

		m_Descriptor->Bind(
			{
				Descriptor::BindingInfo{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, {.m_Buffer = m_CullDatas[ index ] } }
			}
		);

		for ( auto& object : staticGeometry )
		{
			if ( Assets::StaticGLTFAsset* staticGLTF = dynamic_cast< Assets::StaticGLTFAsset* >( object ) )
			{
				std::vector<Descriptor*> descriptors = { staticGLTF->GetIndirectDescriptor( index ), staticGLTF->GetPrimitiveDescriptor( ), m_Descriptor };

				vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *( m_Pipeline ) );

				commandBuffer.BindDescriptors( descriptors );
				commandBuffer.SetDescriptorOffsets( descriptors, *( m_Pipeline ) );

				vkCmdDispatch( commandBuffer, static_cast< uint32_t >( staticGLTF->GetIndirectCommandsCount( ) ) / 16, 1, 1 );
			}
		}
	}
}