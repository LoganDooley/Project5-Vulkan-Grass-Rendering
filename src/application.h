#pragma once

#include "window.h"
#include "vulkan_context.h"
#include "renderer.h"
#include "camera.h"
#include "render_settings.h"
#include "frame_stats.h"

#include <chrono>

class Application {
public:
    Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) = delete;
    Application& operator=(Application&&) = delete;

    void run();

private:
    Window m_window;
    VulkanContext m_context;
    Renderer m_renderer;
    Camera m_camera;
    RenderSettings m_renderSettings{};
    FrameStats m_frameStats{};

    std::chrono::steady_clock::time_point m_firstFrameTime;
    std::chrono::steady_clock::time_point m_lastFrameTime;
    float m_smoothDt = 1.0f / 60.0f;
    float m_cpuAccumulatedTime = 0.0f;
    uint32_t m_cpuAccumulatedFrames = 0;
};
