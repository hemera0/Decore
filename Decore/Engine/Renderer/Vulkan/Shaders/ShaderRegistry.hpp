#pragma once

namespace Engine::Renderer
{
	class Shader;

	class ShaderRegistry {
	public:
		static inline ShaderRegistry* s_Inst = nullptr;

		static ShaderRegistry* Get( )
		{
			if ( !s_Inst )
				s_Inst = new ShaderRegistry( );

			return s_Inst;
		}

		static void Dispose( )
		{
			delete s_Inst;
			s_Inst = nullptr;
		}

		Shader* Register( const std::string& path, VkShaderStageFlagBits flags )
		{
			if ( m_RegisteredShaders.find( path ) == m_RegisteredShaders.end( ) )
				m_RegisteredShaders[ path ] = new Shader( path, flags );

			return m_RegisteredShaders[ path ];
		}

		std::unordered_map<std::string, Shader*> GetShaders( )
		{
			return m_RegisteredShaders;
		}

		void OnUpdate();
	private:
		std::unordered_map<std::string, Shader*> m_RegisteredShaders{};
	};
}