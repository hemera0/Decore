#pragma once
#include <fstream>
#include <sstream>

#include "shaderc/shaderc.h"

namespace Engine::Renderer {
	class Pipeline;

	class Shader {
	public:
		Shader();
		Shader(const std::string& path, VkShaderStageFlagBits flags); // From path...

		const std::vector<uint8_t>& GetSPIRV() const { return m_SpirvData; }
		const VkShaderStageFlagBits GetStageFlags() const { return m_StageFlags; }

		const std::string& GetPath() const { return m_Path; }

		bool GetDirty( ) const { return m_Dirty; }
		void SetDirty( bool dirty ) { m_Dirty = dirty; }
		
		std::vector<Pipeline*> GetPipelines( ) { return m_Pipelines; }
		void AddPipelineRef( Pipeline* pipeline ) { m_Pipelines.push_back( pipeline ); }

		bool Compile();
		bool Recompile( );
	private:

		VkShaderStageFlagBits m_StageFlags{};

		__forceinline shaderc_shader_kind GetShaderKindFromStage(VkShaderStageFlagBits flags) {
			if (flags & VK_SHADER_STAGE_VERTEX_BIT)
				return shaderc_vertex_shader;

			if (flags & VK_SHADER_STAGE_COMPUTE_BIT)
				return shaderc_compute_shader;

			// assume its a fragment shader by default.
			return shaderc_fragment_shader;
		}

		shaderc_shader_kind m_ShaderKind{};

		std::string m_Path{};
		std::string m_SourceData{};
		std::vector<uint8_t> m_SpirvData{};
		
		// All pipelines referencing this shader.
		std::vector<Pipeline*> m_Pipelines = {};

		bool m_Dirty{};
	};

	class ShaderCompiler {
		static inline ShaderCompiler* s_Inst = nullptr;
	public:
		static ShaderCompiler* Get( )
		{
			if ( !s_Inst )
				s_Inst = new ShaderCompiler( );

			return s_Inst;
		}

		bool Compile( Shader* shader );
	};

}