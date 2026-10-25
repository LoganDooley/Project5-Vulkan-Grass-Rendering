#pragma once

#include <vulkan/vulkan_raii.hpp>

class VulkanContext;
struct RenderSettings;

class Terrain {
public:
    Terrain(const VulkanContext& context, vk::Format colorFormat, vk::Format depthFormat);

    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;
    Terrain(Terrain&&) = delete;
    Terrain& operator=(Terrain&&) = delete;

    void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress, const RenderSettings& settings) const;

private:
    void createPlanePipelineLayout();
    void createPlanePipline(vk::Format colorFormat, vk::Format depthFormat);
    void drawPlane(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress) const;

    const VulkanContext& m_context;
    vk::raii::PipelineLayout m_planePipelineLayout{nullptr};
    vk::raii::Pipeline m_planePipeline{nullptr};
};