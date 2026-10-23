#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>

namespace PipelineUtils {
    vk::raii::Pipeline createMeshPipeline(const vk::raii::Device& device,
        const std::vector<vk::PipelineShaderStageCreateInfo>& stages,
        vk::PipelineLayout layout,
        vk::Format colorFormat,
        vk::Format depthFormat);

    vk::raii::Pipeline createComputePipeline(const vk::raii::Device& device,
        const vk::raii::ShaderModule& module,
        vk::PipelineLayout layout);
};