#pragma once

#include <vulkan/vulkan_raii.hpp>

#include "swapchain.h"
#include "buffer.h"
#include "gpu_types.h"
#include "image.h"
#include "grass.h"
#include "imgui_layer.h"
#include "gpu_profiler.h"
#include "terrain.h"

#include <array>
#include <vector>

class VulkanContext;
class Window;
class Camera;
struct RenderSettings;

struct FrameData {
    vk::raii::CommandBuffer commandBuffer{nullptr};
    vk::raii::Semaphore imageAvailableSemaphore{nullptr};
    vk::raii::Fence inFlightFence{nullptr};
    Buffer cameraBuffer;
};

class Renderer {
public:
    Renderer(const VulkanContext& context, Window& window, const RenderSettings& renderSettings);

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    void drawFrame(const Camera& camera, float dt, float totalTime, const RenderSettings& renderSettings);

    ImGuiLayer& getImGuiLayer() { return m_imGuiLayer; }
    const GpuTimings& getGpuTimings() const { return m_profiler.getTimings(); }

    uint32_t getBladeCount() const { return m_grass.getBladeCount(); }

private:
    static void transitionImage(const vk::raii::CommandBuffer& cmd,
        vk::Image image,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess,
        vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess,
        vk::ImageAspectFlags aspect);

    void recreateSwapchain();

    void createDepthImage();

    const VulkanContext& m_context;
    Window& m_window;
    vk::PresentModeKHR m_presentMode = vk::PresentModeKHR::eFifo;
    Swapchain m_swapchain;
    Image m_depthImage;
    ImGuiLayer m_imGuiLayer;
    vk::raii::CommandPool m_commandPool{nullptr};
    std::array<FrameData, 2> m_frames;
    std::vector<vk::raii::Semaphore> m_renderFinishedSemaphores;
    uint32_t m_frameIndex = 0;
    vk::ShaderStageFlags m_pcStages;
    Terrain m_terrain;
    Grass m_grass;
    GpuProfiler m_profiler;
};