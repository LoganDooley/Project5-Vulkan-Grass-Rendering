#include "shader_utils.h"

#include <fstream>
#include <stdexcept>

std::vector<uint32_t> ShaderUtils::readSpirv(const std::string &path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if(!file.is_open()){
        throw std::runtime_error("Failed to open shader: " + path);
    }

    const std::streamsize size = file.tellg();
    if(size <= 0 || size % 4 != 0){
        throw std::runtime_error("Invalid SPIRV file size: " + path);
    }

    std::vector<uint32_t> words(static_cast<size_t>(size) / 4);

    file.seekg(0);
    file.read(reinterpret_cast<char*>(words.data()), size);

    return words;
}

vk::raii::ShaderModule ShaderUtils::createShaderModule(const vk::raii::Device &device, const std::string &path)
{
    std::vector<uint32_t> code = readSpirv(path);
    vk::ShaderModuleCreateInfo moduleInfo = {
        .codeSize = code.size() * sizeof(uint32_t),
        .pCode = code.data()
    };
    return vk::raii::ShaderModule(device, moduleInfo);
}
