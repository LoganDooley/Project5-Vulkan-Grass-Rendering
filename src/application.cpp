#include "application.h"

#include <GLFW/glfw3.h>

#include <algorithm>

Application::Application() : 
    m_window(1280, 720, "Vulkan Grass"),
    m_context(m_window),
    m_renderer(m_context, m_window)
{
    
}

void Application::run() {
    m_lastFrameTime = std::chrono::steady_clock::now();
    while (!m_window.shouldClose()) {
        m_window.pollEvents();
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        dt = std::min(dt, 0.1f);
        m_lastFrameTime = now;

        if(m_window.isKeyDown(GLFW_KEY_ESCAPE)) {
            m_window.requestClose();
        }

        // Do camera movement

        // Do rendering
        m_renderer.drawFrame();

        m_window.endFrame();
    }
    m_context.getDevice().waitIdle();
}
