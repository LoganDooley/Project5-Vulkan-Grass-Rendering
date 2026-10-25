#pragma once

#include "terrain_mode.h"
#include "render_settings.h"

#include <memory>

class VulkanContext;

class Terrain {
public:
    Terrain(const VulkanContext& context, vk::Format colorFormat, vk::Format depthFormat, TerrainType type);

    Terrain(const Terrain&) = delete;
    Terrain& operator=(const Terrain&) = delete;
    Terrain(Terrain&&) = delete;
    Terrain& operator=(Terrain&&) = delete;

    void setType(TerrainType type);
    TerrainType getType() const { return m_type; }

    void update(const vk::raii::CommandBuffer& cmd, const glm::vec3& cameraPos);
    void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress) const;

private:
    const VulkanContext& m_context;
    vk::Format m_colorFormat;
    vk::Format m_depthFormat;
    TerrainType m_type;
    std::unique_ptr<TerrainMode> m_mode;
};