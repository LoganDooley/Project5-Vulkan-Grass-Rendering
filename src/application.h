#pragma once

#include "window.h"
#include "vulkan_context.h"

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

    std::chrono::steady_clock::time_point m_lastFrameTime;
};
