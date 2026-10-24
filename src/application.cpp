#include "application.h"

#include <GLFW/glfw3.h>

#include <algorithm>

Application::Application() : 
    m_window(1280, 720, "Vulkan Grass"),
    m_context(m_window),
    m_renderer(m_context, m_window),
    m_camera(glm::vec3(0.0f, 1.0f, 3.0f))
{
    
}

void Application::run() {
    m_firstFrameTime = std::chrono::steady_clock::now();
    m_lastFrameTime = m_firstFrameTime;
    while (!m_window.shouldClose()) {
        m_window.pollEvents();
        std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
        float dt = std::chrono::duration<float>(now - m_lastFrameTime).count();
        dt = std::min(dt, 0.1f);
        // With eFifo at 240 fps, when the simulation is fast, the frame times alternate between fast and slow.
        // Adding a smoothed Dt to try evening it out for simulation puposes
        m_smoothDt = m_smoothDt * 0.9 + dt * 0.1f;
        float totalTime = std::chrono::duration<float>(now - m_firstFrameTime).count();
        m_lastFrameTime = now;

        if(m_window.isKeyDown(GLFW_KEY_ESCAPE)) {
            m_window.requestClose();
        }

        // Do camera movement
        CameraInput input{};
        if (m_window.isKeyDown(GLFW_KEY_W)) {
            input.move.z += 1.0f;
        }
        if (m_window.isKeyDown(GLFW_KEY_S)){
            input.move.z -= 1.0f;
        } 
        if (m_window.isKeyDown(GLFW_KEY_D)) {
            input.move.x += 1.0f;
        }
        if (m_window.isKeyDown(GLFW_KEY_A)) {
            input.move.x -= 1.0f;
        }
        if (m_window.isKeyDown(GLFW_KEY_SPACE)) {
            input.move.y += 1.0f;
        }
        if (m_window.isKeyDown(GLFW_KEY_LEFT_SHIFT)) {
            input.move.y -= 1.0f;
        }
        if (m_window.isMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
            input.look = glm::vec2(m_window.getMouseDelta());
        }
        m_camera.update(dt, input);

        // Do rendering
        m_renderer.drawFrame(m_camera, m_smoothDt, totalTime);

        m_window.endFrame();
    }
    m_context.getDevice().waitIdle();
}
