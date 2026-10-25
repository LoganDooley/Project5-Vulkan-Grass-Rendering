#include "application.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "settings_ui.h"

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
        m_smoothDt = m_smoothDt * 0.9f + dt * 0.1f;
        float totalTime = std::chrono::duration<float>(now - m_firstFrameTime).count();
        m_lastFrameTime = now;
        m_cpuAccumulatedTime += dt;
        m_cpuAccumulatedFrames++;

        // Record cpu time
        m_frameStats.frameHistory[m_frameStats.historyIndex] = dt * 1000.0f;
        m_frameStats.historyIndex = (m_frameStats.historyIndex + 1) % m_frameStats.frameHistory.size();
        if(m_cpuAccumulatedTime >= 0.5f){
            // Update at most once every 0.5 seconds
            m_frameStats.cpuFrameMs = 1000.0f * m_cpuAccumulatedTime / m_cpuAccumulatedFrames;
            m_cpuAccumulatedTime = 0.0f;
            m_cpuAccumulatedFrames = 0;
        }


        if(m_window.isKeyDown(GLFW_KEY_ESCAPE)) {
            m_window.requestClose();
        }

        // Do camera movement
        CameraInput input{};

        ImGuiIO& io = ImGui::GetIO();
        if(!io.WantCaptureMouse) {
            if (m_window.isMouseDown(GLFW_MOUSE_BUTTON_RIGHT)) {
                input.look = glm::vec2(m_window.getMouseDelta());
            }
        }
        if(!io.WantCaptureKeyboard) {
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
        }
        m_camera.update(dt, input);

        // Begin imgui
        ImGuiLayer& imGuiLayer = m_renderer.getImGuiLayer();
        imGuiLayer.beginFrame();
        SettingsUI::drawSettingsPanel(m_renderSettings, m_frameStats);
        imGuiLayer.endFrame();

        // Do rendering
        m_renderer.drawFrame(m_camera, m_smoothDt, totalTime, m_renderSettings);

        m_window.endFrame();
    }
    m_context.getDevice().waitIdle();
}
