#ifndef __HL_FORGE_SHADERS_H_
#define __HL_FORGE_SHADERS_H_
#pragma once
#include <string>
#include <vector>
enum EShaderKind {
    HLFG_SHADER_VERTEX,
    HLFG_SHADER_FRAGMENT
};
std::vector<uint32_t> assemble_to_spv(const std::string &source);
std::string compile_file_to_assembly(const char *source_name,
                                     EShaderKind kind,
                                     const std::string &source,
                                     bool optimize = false);
void writeShaderSource(const std::string &path, const std::string &source);
void writeShaderSPV(const std::string &path, const std::vector<uint32_t> &code);
std::string getShaderSource(const std::string &path);
std::string getMSLFromSPV( const std::vector<uint32_t> &spirv_binary ) ;
#endif