#include <fstream>
#include <iostream>
#include <shaderc/shaderc.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_msl.hpp>

#include "hl-forge-shaders.h"


// Compiles a shader to SPIR-V assembly. Returns the assembly text
// as a string.
std::string compile_file_to_assembly(const char *source_name,
                                            EShaderKind kind,
                                            const std::string &source,
                                            bool optimize) {
    shaderc::Compiler compiler;
    shaderc::CompileOptions options;
//    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_opengl, 0);
      options.SetTargetEnvironment(shaderc_target_env_default, 0);
      options.SetAutoBindUniforms(true);
      options.SetTargetSpirv(shaderc_spirv_version_1_0);
    options.SetForcedVersionProfile(330, shaderc_profile_none);
    options.SetAutoMapLocations(true);
    //	options.SetAutoBindUniforms(true);

    auto spirvkind = shaderc_vertex_shader;
    if (kind == HLFG_SHADER_FRAGMENT)spirvkind = shaderc_fragment_shader;
    // Like -DMY_DEFINE=1
    //  options.AddMacroDefinition("MY_DEFINE", "1");
    if (optimize) options.SetOptimizationLevel(shaderc_optimization_level_size);

    //	std::cout << "source:" << std::endl << source << std::endl;
    shaderc::AssemblyCompilationResult result = compiler.CompileGlslToSpvAssembly(
        source, spirvkind, source_name, "main", options);

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        std::cerr << result.GetErrorMessage();
        std::cout << "Done compiling" << std::endl;
        return "";
    }

    return {result.cbegin(), result.cend()};
}

std::vector<uint32_t> assemble_to_spv(const std::string &source) {
    shaderc::Compiler compiler;

/*
    shaderc::CompileOptions options;
    options.SetSourceLanguage(shaderc_source_language_glsl);
    options.SetTargetEnvironment(shaderc_target_env_opengl, 0);
    options.SetForcedVersionProfile(330, shaderc_profile_none);
    options.SetAutoMapLocations(true);
	*/
    //	options.SetAutoBindUniforms(true);
	auto assembling = compiler.AssembleToSpv(source);

    return {assembling.cbegin(), assembling.cend()};
}

void writeShaderSource(const std::string &path, const std::string &source) {
    std::ofstream out(path);
    out << source;
    out.close();
}
void writeShaderSPV(const std::string &path, const std::vector<uint32_t> &code) {
    std::ofstream fout(path, std::ios::out | std::ios::binary);
    fout.write((char*)&code[0], code.size() * sizeof(uint32_t));
    fout.close();
}
std::string getShaderSource(const char *path) {
    std::ifstream ifs(path);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                        (std::istreambuf_iterator<char>()));
    return content;
}

std::string getMSLFromSPV( const std::vector<uint32_t> &spirv_binary ) {

	spirv_cross::CompilerMSL mslCompiler(std::move(spirv_binary));

    // Set some options.
	spirv_cross::CompilerGLSL::Options common_options;
//    common_options.force_temporary = true;
    common_options.vulkan_semantics = true;
	mslCompiler.set_common_options(common_options);

    // Set some options.
	spirv_cross::CompilerMSL::Options mtl_options;
//    mtl_options.argument_buffers = true;
 //   mtl_options.force_native_arrays = true;
    mtl_options.set_msl_version(2,4);
	mslCompiler.set_msl_options(mtl_options);

	// Compile to MSL
	std::string source = mslCompiler.compile();
    return source;

}