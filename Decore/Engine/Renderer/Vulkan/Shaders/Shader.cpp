#include "../VulkanRenderer.hpp"

#include <filesystem>

namespace Engine::Renderer {
	struct IncludesList {
		std::set<std::string> m_Paths;
	};

	struct FileData {
		std::filesystem::path m_Path;

		std::string m_PathName{};
		std::vector<char> m_Contents; 
	};
	
	std::vector<char> ReadFileToVector(const std::filesystem::path& path) {
		std::vector<char> contents{};

		size_t fileSize = 0;
		std::ifstream file(path, std::ios::in | std::ios::binary | std::ios::ate);

		if (file.is_open()) {
			auto endOfFilePos = file.tellg();
			if (endOfFilePos >= 0) {
				fileSize = size_t(endOfFilePos);
			}

			contents.resize(fileSize);

			file.seekg(0, std::ios::beg);
			file.read(contents.data(), endOfFilePos);
			file.close();
		}

		return contents;
	}

	Shader::Shader() {

	}

	Shader::Shader( const std::string& path, VkShaderStageFlagBits flags )
		: m_Path( path ), m_StageFlags( flags )
	{
		m_ShaderKind = GetShaderKindFromStage( flags );

		std::ifstream inputStream( m_Path, std::ios::in | std::ios::binary );
		if ( inputStream.is_open( ) )
		{
			std::stringstream buffer{};
			buffer << inputStream.rdbuf( );

			m_SourceData = buffer.str( );

			if ( !Compile( ) )
			{
				throw std::runtime_error( "Shader compilation error" );
			}
		}
	}
	
	shaderc_include_result* ProcessShaderIncludes(void* user_data, const char* requested_source, int type,
		const char* requesting_source, size_t include_depth) {

		auto self = new shaderc_include_result();
		auto includesList = reinterpret_cast<IncludesList*>(user_data);

		std::filesystem::path wantedSourcePath;

		if (type == shaderc_include_type_relative) {
			auto basename = std::filesystem::path(requesting_source).remove_filename();
			wantedSourcePath = basename.append(requested_source);
		}

		bool wantedFileExists = std::filesystem::exists(wantedSourcePath);

		auto fileData = new FileData();

		if (wantedFileExists) {
			wantedSourcePath = std::filesystem::canonical(wantedSourcePath);
			includesList->m_Paths.insert(wantedSourcePath.string());

			std::ifstream inputStream(wantedSourcePath, std::ios::in | std::ios::binary);
			if (inputStream.is_open()) {

				std::stringstream buffer{};
				buffer << inputStream.rdbuf();

				const auto& bufferStr = buffer.str();

				fileData->m_Path = wantedSourcePath;

				const auto& pathStr = wantedSourcePath.string();

				fileData->m_PathName.resize(pathStr.size());
				std::copy(pathStr.begin(), pathStr.end(), fileData->m_PathName.begin());

				fileData->m_Contents = ReadFileToVector(wantedSourcePath);
			}
		}

		self->user_data = fileData;
		self->content = fileData->m_Contents.data();
		self->content_length = fileData->m_Contents.size();

		self->source_name = fileData->m_PathName.c_str();
		self->source_name_length = fileData->m_PathName.size();

		return self;
	}

	void ReleaseShaderInclude(void* user_data, shaderc_include_result* include_result) {
		if (FileData* fileData = reinterpret_cast<FileData*>(include_result->user_data)) {
			delete fileData;
		}

		delete include_result;
	}

	bool Shader::Recompile( )
	{
		std::ifstream inputStream( m_Path, std::ios::in | std::ios::binary );
		if ( inputStream.is_open( ) )
		{
			std::stringstream buffer{};
			buffer << inputStream.rdbuf( );

			m_SourceData = buffer.str( );
		}

		return Compile( );
	}

	bool Shader::Compile() {
		static shaderc_compiler_t compiler = shaderc_compiler_initialize();
		static shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false);
		shaderc_compile_options_set_target_env(options, shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_4);
		shaderc_compile_options_set_target_spirv(options, shaderc_spirv_version_1_6);
		static IncludesList includes{};
		shaderc_compile_options_set_include_callbacks(options, &ProcessShaderIncludes, &ReleaseShaderInclude, &includes);
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		shaderc_compilation_result_t result = shaderc_compile_into_spv(
			compiler, const_cast<const char*>(m_SourceData.data()), m_SourceData.length(), m_ShaderKind, m_Path.c_str(), "main", options);

		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success)
		{
			printf("Shader compilation error: %s\n", shaderc_result_get_error_message(result));
			return false;
		}

		m_SpirvData.resize(shaderc_result_get_length(result));
		memcpy(m_SpirvData.data(), shaderc_result_get_bytes(result), m_SpirvData.size());

		shaderc_result_release(result);
		// shaderc_compile_options_release(options);
		// shaderc_compiler_release(compiler);

		return true;
	}
}