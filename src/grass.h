#pragma once

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

    void recordInit(const vk::raii::CommandBuffer& cmd) const;
    void simulate(const vk::raii::CommandBuffer& cmd, float dt, float totalTime, const RenderSettings& renderSettings) const;
    void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress, vk::DeviceAddress cullCameraAddress, vk::DeviceAddress visibleCounterAddress, const RenderSettings& renderSettings) const;

    float getDensity() const { return m_density; }
    float getRadius() const { return m_radius; }
    uint32_t getBladeCount() const { return m_bladeCount; }

    void setTerrainType(TerrainType terrainType) { m_terrainType = terrainType; }
    TerrainType getTerrainType() const { return m_terrainType; }

private:
    void createInitPipelineLayout();
    void createDrawPipelineLayout();
    void createSimPipelineLayout();

    void createInitPipeline();
    void createDrawPipeline(vk::Format colorFormat, vk::Format depthFormat);
    void createSimPipeline();

    const VulkanContext& m_context;
    float m_density = 25.0f;
    float m_radius = 50.0f;
    uint32_t m_gridWidth = 0;
    uint32_t m_bladeCount = 0;
    Buffer m_blades;
    vk::raii::PipelineLayout m_initPipelineLayout{nullptr};
    vk::raii::Pipeline m_initPipeline{nullptr};
    vk::raii::PipelineLayout m_drawPipelineLayout{nullptr};
    vk::raii::Pipeline m_drawPipeline{nullptr};
    vk::raii::PipelineLayout m_simPipelineLayout{nullptr};
    vk::raii::Pipeline m_simPipeline{nullptr};
    TerrainType m_terrainType{};
};