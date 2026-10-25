#pragma once

#include "terrain_mode.h"

class VulkanContext;

class HeightmapTerrain : public TerrainMode {
public:
    HeightmapTerrain(const VulkanContext& context, vk::Format colorFormat, vk::Format depthFormat);

    HeightmapTerrain(const HeightmapTerrain&) = delete;
    HeightmapTerrain& operator=(const HeightmapTerrain&) = delete;
    HeightmapTerrain(HeightmapTerrain&&) = delete;
    HeightmapTerrain& operator=(HeightmapTerrain&&) = delete;

    void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress) const override;

private:
    void createPipelineLayout();
    void createPipeline(vk::Format colorFormat, vk::Format depthFormat);

    const VulkanContext& m_context;
    vk::raii::PipelineLayout m_pipelineLayout{nullptr};
    vk::raii::Pipeline m_pipeline{nullptr};
};