#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "swapchain.h"
#include "buffer.h"
#include "gpu_types.h"
#include "image.h"

#include <array>
#include <vector>

class VulkanContext;
class Window;
class Camera;

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

    void drawFrame(const Camera& camera, float totalTime);

private:
    static void transitionImage(const vk::raii::CommandBuffer& cmd,
        vk::Image image,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
        vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess,
        vk::ImageAspectFlags aspect);

    void recreateSwapchain();

    void createMeshPipelineLayout();
    void createAnimPipelineLayout();
    void createTrianglePipeline();
    void createGroundPipeline();
    void createAnimPipeline();

    // Pipeline helpers
    vk::raii::Pipeline createMeshPipeline(const std::vector<vk::PipelineShaderStageCreateInfo>& stages, vk::PipelineLayout layout);

    void createDepthImage();

    const VulkanContext& m_context;
    Window& m_window;
    Swapchain m_swapchain;
    Image m_depthImage;
    vk::raii::PipelineLayout m_meshPipelineLayout{nullptr};
    vk::raii::Pipeline m_trianglePipeline{nullptr}; 
    vk::raii::PipelineLayout m_animPipelineLayout{nullptr};
    vk::raii::Pipeline m_animPipeline{nullptr};
    vk::raii::Pipeline m_groundPipeline{nullptr};
    vk::raii::CommandPool m_commandPool{nullptr};
    std::array<FrameData, 2> m_frames;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    uint32_t m_frameIndex = 0;
    vk::ShaderStageFlags m_pcStages;
    Buffer m_positionsBuffer;
};