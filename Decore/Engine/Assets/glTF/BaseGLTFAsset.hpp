#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include <tiny_gltf.h>

#include "../BaseAsset.hpp"
#include "../../Core/Collision/CollisionBox.hpp"
#include "../../Core/Collision/CollisionCapsule.hpp"

namespace Engine::Assets
{
	enum class AlphaMode {
		AM_NONE = -1,
		AM_OPAQUE,
		AM_BLEND,
		AM_MASK
	};

	class BaseGLTFAsset : public BaseAsset {
	protected:
		using RGBColor_t = glm::vec3;
		using RGBAColor_t = glm::vec4;

		struct TextureData {
			Renderer::Image* m_Image{};
			Renderer::ImageView* m_ImageView{};
		};

		struct alignas( 16 ) Material {
			int m_AlbedoMapIndex{};
			int m_NormalMapIndex{};
			int m_MetalRoughnessMapIndex{};
			int m_EmissiveMapIndex{};
			glm::ivec4 m_OcclusionMapIndex{};

			RGBAColor_t m_BaseColor{};
			RGBAColor_t m_EmissiveFactor{};
			AlphaMode m_AlphaMode{};
			float m_AlphaCutoff{};

			float m_MetallicFactor{};
			float m_RoughnessFactor{};
		};

		struct Bounds {
			glm::vec3 m_Mins{ std::numeric_limits<float>::max( ) }, m_Maxs{ std::numeric_limits<float>::min( ) };
		};

		struct Primitive {
			uint32_t m_IndexCount{}, m_VertexCount{};
			size_t m_IndexOffset{};

			int m_MaterialIndex{};

			// TODO: GetBounds function.
			Bounds m_Bounds{};
		};

		struct Mesh {
			std::vector<Primitive> m_Primitives{};

			// Bounds m_Bounds{};
		};

		struct Node {
			Node( const std::string& name, Node* parent, int index, int skinIndex ) :
				m_Name( name ), m_Parent( parent ), m_Index( index ), m_SkinIndex( skinIndex )
			{
			}

			glm::mat4 m_Matrix{};

			std::string m_Name{};
			Mesh* m_Mesh{};
			Node* m_Parent{};

			int m_Index{}, m_SkinIndex{ -1 };

			// TRS transform info.
			glm::vec3 m_Translation{}, m_Scale{};
			glm::quat m_Rotation{};

			std::vector<Node*> m_Children{};
			std::map<std::string, std::string> m_Properties{};

			glm::mat4 m_CachedMatrix{};
			bool m_UseCachedMatrix{};

			inline void ParseOrientation( const tinygltf::Node& node )
			{
				m_Matrix = glm::identity<glm::mat4>( );
				m_Translation = glm::vec3( 0.f, 0.f, 0.f );
				m_Scale = glm::vec3( 1.f, 1.f, 1.f );
				m_Rotation = glm::quat( );

				if ( node.matrix.size( ) == 16 )
				{
					m_Matrix = glm::make_mat4( node.matrix.data( ) );
				}

				if ( node.translation.size( ) == 3 )
				{
					m_Translation = glm::make_vec3( node.translation.data( ) );
				}

				if ( node.scale.size( ) == 3 )
				{
					m_Scale = glm::make_vec3( node.scale.data( ) );
				}

				if ( node.rotation.size( ) == 4 )
				{
					m_Rotation = glm::make_quat( node.rotation.data( ) );
				}
			}

			inline glm::mat4 GetLocalMatrix( ) const
			{
				glm::mat4 scaleMat{};

				scaleMat[ 0 ][ 0 ] = m_Scale.x;
				scaleMat[ 1 ][ 1 ] = m_Scale.y;
				scaleMat[ 2 ][ 2 ] = m_Scale.z;
				scaleMat[ 3 ][ 3 ] = 1.f;

				glm::mat4 rotTrans = glm::translate( glm::identity<glm::mat4>( ), m_Translation ) * glm::mat4( m_Rotation );
				glm::mat4 scaleRotTrans = rotTrans * scaleMat;
				return scaleRotTrans * m_Matrix;
			}

			inline glm::mat4 GetMatrix( )
			{
				if ( !m_UseCachedMatrix )
				{
					glm::mat4 mat = GetLocalMatrix( );

					Node* currentParent = m_Parent;
					while ( currentParent )
					{
						mat = currentParent->GetLocalMatrix( ) * mat;
						currentParent = currentParent->m_Parent;
					}

					m_UseCachedMatrix = true;
					m_CachedMatrix = mat;

					return mat;
				}
				else
				{
					return m_CachedMatrix;
				}
			}
		};

		tinygltf::Model m_LoadedModel{};

		Renderer::Buffer* m_VertexBuffer{};
		Renderer::Buffer* m_IndexBuffer{};

		std::vector<Node*> m_Nodes;

		std::shared_ptr<Renderer::Device> m_Device{};
		std::shared_ptr<Renderer::CommandPool> m_CommandPool{};
	public: // TODO: Remove.
		std::vector<Node*> m_AllNodes;

		std::vector<TextureData> m_Textures{};
		std::vector<Material> m_Materials{};

		std::vector<Core::CollisionBox> m_AABBs{};

		size_t m_VertexCount{}, m_IndexCount{};
		size_t m_LastVertex{}, m_LastIndex{};

		Renderer::Sampler* m_DefaultTextureSampler = nullptr;
		Renderer::Buffer* m_MaterialsBuffer = nullptr;

		Renderer::Descriptor* m_MaterialBufferDescriptor = nullptr;
		Renderer::Descriptor* m_TextureBufferDescriptor = nullptr;

		__forceinline Node* FindNode( Node* source, int index )
		{
			if ( source->m_Index == index )
				return source;

			for ( auto& child : source->m_Children )
			{
				Node* res = FindNode( child, index );
				if ( res )
					return res;
			}

			return nullptr;
		}

		__forceinline Node* GetNodeByIndex( int index )
		{
			for ( auto& node : m_Nodes )
			{
				Node* res = FindNode( node, index );
				if ( res )
					return res;
			}

			return nullptr;
		}

		__forceinline void GetNodeInfo( const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount )
		{
			for ( auto i = 0; i < node.children.size( ); i++ )
			{
				GetNodeInfo( model.nodes[ node.children[ i ] ], model, vertexCount, indexCount );
			}

			if ( node.mesh > -1 )
			{
				const auto& mesh = model.meshes[ node.mesh ];
				for ( const auto& primitive : mesh.primitives )
				{
					if ( auto positionAttr = primitive.attributes.find( "POSITION" ); positionAttr != primitive.attributes.cend( ) )
					{
						vertexCount += model.accessors[ positionAttr->second ].count;
					}

					if ( primitive.indices > -1 )
						indexCount += model.accessors[ primitive.indices ].count;
				}
			}
		}

		bool OpenFile( const std::string& gltfPath )
		{
			tinygltf::TinyGLTF loader;
			std::string err{}, warn{};

			bool res{};

			if ( gltfPath.find( ".glb" ) != std::string::npos )
			{
				res = loader.LoadBinaryFromFile( &m_LoadedModel, &err, &warn, gltfPath );
			}
			else
			{
				res = loader.LoadASCIIFromFile( &m_LoadedModel, &err, &warn, gltfPath );
			}

			if ( !res )
			{
				if ( !err.empty( ) )
					printf( "Err: %s\n", err.data( ) );

				if ( !warn.empty( ) )
					printf( "Warn: %s\n", warn.data( ) );
			}

			return res;
		}

		virtual bool LoadAsset( const std::string& gltfPath, int sceneIndex ) = 0;
		virtual void LoadNode( const tinygltf::Node& inputNode, Node* parent, uint32_t nodeIndex ) = 0;
		virtual void CreateGeometryBuffers( std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool ) = 0;
		virtual void UnloadAsset( ) = 0;

		virtual std::string GetName( ) const override { return m_LoadedModel.nodes[ 0 ].name; }
		virtual void LoadMaterials( std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool, const Renderer::DescriptorLayout& materialLayout );
		virtual void LoadTextures( std::shared_ptr<Renderer::Device> device, std::shared_ptr<Renderer::CommandPool> commandPool, const Renderer::DescriptorLayout& textureLayout );
	public:
		virtual void SetupDevice( const std::vector<Renderer::DescriptorLayout>& descriptorLayouts ) = 0;
	};
}