#include "terrain.h"

#include "vulkan_context.h"
#include "plane_terrain.h"
#include "heightmap_terrain.h"

Terrain::Terrain(const VulkanContext& context, vk::Format colorFormat, vk::Format depthFormat, TerrainType type) :
    m_context(context),
    m_colorFormat(colorFormat),
    m_depthFormat(depthFormat),
    m_type(type)
{
    setType(type);
}

void Terrain::setType(TerrainType type)
{
    if(type == m_type && m_mode != nullptr) {
        return;
    }

    m_context.getDevice().waitIdle();

    switch(type){
        case TerrainType::Plane:
            m_mode = std::make_unique<PlaneTerrain>(m_context, m_colorFormat, m_depthFormat);
            break;
        case TerrainType::Heightmap:
            m_mode = std::make_unique<HeightmapTerrain>(m_context, m_colorFormat, m_depthFormat);
            break;
        case TerrainType::Voxel:
        default:
            m_mode.reset();
            break;
    }

    m_type = type;
}

void Terrain::update(const vk::raii::CommandBuffer &cmd, const glm::vec3 &cameraPos)
{
    if(m_mode == nullptr){
        return;
    }

    m_mode->update(cmd, cameraPos);
}

void Terrain::draw(const vk::raii::CommandBuffer &cmd, vk::DeviceAddress cameraAddress) const
{
    if(m_mode == nullptr){
        return;
    }

    m_mode->draw(cmd, cameraAddress);
}
