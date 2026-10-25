#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "buffer.h"

class VulkanContext;
struct RenderSettings;

class Grass {
public:
    Grass(const VulkanContext& context, uint32_t bladeCount, float areaSize, vk::Format colorFormat, vk::Format depthFormat);

    Grass(const Grass&) = delete;
    Grass& operator=(const Grass&) = delete;
    Grass(Grass&&) = delete;
    Grass& operator=(Grass&&) = delete;

    void recordInit(const vk::raii::CommandBuffer& cmd) const;
    void simulate(const vk::raii::CommandBuffer& cmd, float dt, float totalTime) const;
    void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress, const RenderSettings& renderSettings) const;

private:
    void createInitPipelineLayout();
    void createDrawPipelineLayout();
    void createSimPipelineLayout();

    void createInitPipeline();
    void createDrawPipeline(vk::Format colorFormat, vk::Format depthFormat);
    void createSimPipeline();

    const VulkanContext& m_context;
    uint32_t m_bladeCount = 0;
    float m_areaSize = 1.f;
    Buffer m_blades;
    vk::raii::PipelineLayout m_initPipelineLayout{nullptr};
    vk::raii::Pipeline m_initPipeline{nullptr};
    vk::raii::PipelineLayout m_drawPipelineLayout{nullptr};
    vk::raii::Pipeline m_drawPipeline{nullptr};
    vk::raii::PipelineLayout m_simPipelineLayout{nullptr};
    vk::raii::Pipeline m_simPipeline{nullptr};
};