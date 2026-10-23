#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "swapchain.h"
#include "buffer.h"
#include "gpu_types.h"

#include <array>
#include <vector>

class VulkanContext;
class Window;

struct FrameData {
    vk::raii::CommandBuffer commandBuffer{nullptr};
    vk::raii::Semaphore imageAvailableSemaphore{nullptr};
    vk::raii::Fence inFlightFence{nullptr};
    Buffer cameraBuffer;
};

class Renderer {
public:
    Renderer(const VulkanContext& context, Window& window);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void drawFrame(float dt, float totalTime);

private:
    static void transitionImage(const vk::raii::CommandBuffer& cmd,
        vk::Image image,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
        vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess);

    void recreateSwapchain();

    void createTrianglePipeline();

    const VulkanContext& m_context;
    Window& m_window;
    Swapchain m_swapchain;
    vk::raii::PipelineLayout m_trianglePipelineLayout{nullptr};
    vk::raii::Pipeline m_trianglePipeline{nullptr}; 
    vk::raii::CommandPool m_commandPool{nullptr};
    std::array<FrameData, 2> m_frames;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    uint32_t m_frameIndex = 0;
    vk::ShaderStageFlags m_pcStages;
};