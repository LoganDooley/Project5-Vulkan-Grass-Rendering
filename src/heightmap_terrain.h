#pragma once

#include <glm/vec2.hpp>

#include "terrain_mode.h"

class VulkanContext;

class HeightmapTerrain : public TerrainMode {
public:
    HeightmapTerrain(const VulkanContext& context, vk::Format colorFormat, vk::Format depthFormat);

    HeightmapTerrain(const HeightmapTerrain&) = delete;
    HeightmapTerrain& operator=(const HeightmapTerrain&) = delete;
    HeightmapTerrain(HeightmapTerrain&&) = delete;
    HeightmapTerrain& operator=(HeightmapTerrain&&) = delete;

    void update(const vk::raii::CommandBuffer& cmd, const glm::vec3& cameraPos) override;
    void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress) const override;

private:
    void createPipelineLayout();
    void createPipeline(vk::Format colorFormat, vk::Format depthFormat);

    const VulkanContext& m_context;
    vk::raii::PipelineLayout m_pipelineLayout{nullptr};
    vk::raii::Pipeline m_pipeline{nullptr};
    glm::vec2 m_center = glm::vec2(0.0f);
};