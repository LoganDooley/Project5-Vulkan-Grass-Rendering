#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vulkan/vulkan_raii.hpp>

#include "buffer.h"

class VulkanContext;
struct RenderSettings;

enum class TerrainType : uint32_t;

class Grass {
public:
    Grass(const VulkanContext& context, float density, float radius, vk::Format colorFormat, vk::Format depthFormat);

    Grass(const Grass&) = delete;
    Grass& operator=(const Grass&) = delete;
    Grass(Grass&&) = delete;
    Grass& operator=(Grass&&) = delete;

    void resize(float density, float radius);

    void update(const vk::raii::CommandBuffer& cmd, glm::vec3 cameraPos, bool bFollowCamera);
    void simulate(const vk::raii::CommandBuffer& cmd, float dt, float totalTime, const RenderSettings& renderSettings) const;
    void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress, vk::DeviceAddress cullCameraAddress, vk::DeviceAddress visibleCounterAddress, const RenderSettings& renderSettings) const;

    float getDensity() const { return m_density; }
    float getRadius() const { return m_radius; }
    uint32_t getBladeCount() const { return m_bladeCount; }

    void setTerrainType(TerrainType terrainType);
    TerrainType getTerrainType() const { return m_terrainType; }

private:
    void createUpdatePipelineLayout();
    void createDrawPipelineLayout();
    void createSimPipelineLayout();

    void createUpdatePipeline();
    void createDrawPipeline(vk::Format colorFormat, vk::Format depthFormat);
    void createSimPipeline();

    const VulkanContext& m_context;
    float m_density = 25.0f;
    float m_radius = 50.0f;
    uint32_t m_gridWidth = 0;
    uint32_t m_bladeCount = 0;
    Buffer m_blades;
    vk::raii::PipelineLayout m_updatePipelineLayout{nullptr};
    vk::raii::Pipeline m_updatePipeline{nullptr};
    vk::raii::PipelineLayout m_drawPipelineLayout{nullptr};
    vk::raii::Pipeline m_drawPipeline{nullptr};
    vk::raii::PipelineLayout m_simPipelineLayout{nullptr};
    vk::raii::Pipeline m_simPipeline{nullptr};
    TerrainType m_terrainType{};
    glm::ivec2 m_base = glm::ivec2(0);
    bool m_needsFullInit = true;
};