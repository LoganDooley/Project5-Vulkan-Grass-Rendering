#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace ShaderUtils {
    std::vector<uint32_t> readSpirv(const std::string& path);

    vk::raii::ShaderModule createShaderModule(const vk::raii::Device& device, const std::string& path);
};