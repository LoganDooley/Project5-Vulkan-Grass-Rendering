#pragma once

#include <glm/vec2.hpp>
#include <vulkan/vulkan_raii.hpp>

#include <vector>

struct GLFWwindow;

class Window {
public:
    Window(int width, int height, const char* title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    Window(Window&& other) noexcept;
    Window& operator=(Window&& other) noexcept;

    GLFWwindow* getHandle() const { return m_glfwWindow; }

    bool shouldClose() const;
    void pollEvents();

    bool wasResized() const { return m_resized; }
    void clearResized() { m_resized = false; }

    glm::dvec2 getMouseDelta() const { return m_mouseDelta; }
    double getScrollDelta() const { return m_scrollDelta; }

    void endFrame();

    bool isKeyDown(int key) const;
    bool isMouseDown(int button) const;

    void requestClose();

    void setTitle(const char* title);

    std::vector<const char*> getRequiredInstanceExtensions() const;

    vk::raii::SurfaceKHR createSurface(const vk::raii::Instance& instance) const;

    vk::Extent2D getFramebufferExtent();

    void waitWhileMinimized();

private:
    GLFWwindow* m_glfwWindow = nullptr;

    bool m_resized = false;
    glm::dvec2 m_previousMousePos = glm::dvec2(0.0);
    glm::dvec2 m_mouseDelta = glm::dvec2(0.0);
    double m_scrollDelta = 0.0;
    bool m_mouseInitialized = false;
};
