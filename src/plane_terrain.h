#pragma once

#include "terrain_mode.h"

class VulkanContext;

class PlaneTerrain : public TerrainMode {
public:
    PlaneTerrain(const VulkanContext& context, vk::Format colorFormat, vk::Format depthFormat);

    PlaneTerrain(const PlaneTerrain&) = delete;
    PlaneTerrain& operator=(const PlaneTerrain&) = delete;
    PlaneTerrain(PlaneTerrain&&) = delete;
    PlaneTerrain& operator=(PlaneTerrain&&) = delete;

    void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress) const override;

private:
    void createPipelineLayout();
    void createPipeline(vk::Format colorFormat, vk::Format depthFormat);

    const VulkanContext& m_context;
    vk::raii::PipelineLayout m_pipelineLayout{nullptr};
    vk::raii::Pipeline m_pipeline{nullptr};
};