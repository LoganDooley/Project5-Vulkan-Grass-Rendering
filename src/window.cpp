#include "window.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <stdexcept>
#include <utility>

Window::Window(int width, int height, const char* title) {
    glfwSetErrorCallback([](int code, const char* description) {
        std::fprintf(stderr, "GLFW error %d: %s\n", code, description);
    });

    if (!glfwInit()) {
        throw std::runtime_error("Failed to initialize GLFW");
    }

    // No API for vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    m_glfwWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_glfwWindow) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(m_glfwWindow, this);

    glfwSetFramebufferSizeCallback(m_glfwWindow, [](GLFWwindow* w, int, int){
        Window* window = static_cast<Window*>(glfwGetWindowUserPointer(w));
        window->m_resized = true;
    });

    glfwSetCursorPosCallback(m_glfwWindow, [](GLFWwindow* w, double x, double y){
        Window* window = static_cast<Window*>(glfwGetWindowUserPointer(w));
        if(!window->m_mouseInitialized){
            window->m_previousMousePos = glm::dvec2(x, y);
            window->m_mouseInitialized = true;
        }
        else{
            window->m_mouseDelta += glm::dvec2(x, y) - window->m_previousMousePos;
            window->m_previousMousePos = glm::dvec2(x, y);
        }
    });
}

Window::~Window() {
    if (m_glfwWindow) {
        glfwDestroyWindow(m_glfwWindow);
        glfwTerminate();
    }
}

Window::Window(Window&& other) noexcept : 
    m_glfwWindow(std::exchange(other.m_glfwWindow, nullptr)),
    m_resized(std::exchange(other.m_resized, false)),
    m_previousMousePos(std::exchange(other.m_previousMousePos, glm::dvec2(0.0))),
    m_mouseDelta(std::exchange(other.m_mouseDelta, glm::dvec2(0.0))),
    m_mouseInitialized(std::exchange(other.m_mouseInitialized, false))
{
    if (m_glfwWindow) {
        glfwSetWindowUserPointer(m_glfwWindow, this);
    }
}

Window& Window::operator=(Window&& other) noexcept {
    if (this != &other) {
        if (m_glfwWindow) {
            glfwDestroyWindow(m_glfwWindow);
        }
        m_glfwWindow = std::exchange(other.m_glfwWindow, nullptr);
        if (m_glfwWindow) {
            glfwSetWindowUserPointer(m_glfwWindow, this);
        }

        m_resized = std::exchange(other.m_resized, false);
        m_previousMousePos = std::exchange(other.m_previousMousePos, glm::dvec2(0.0));
        m_mouseDelta = std::exchange(other.m_mouseDelta, glm::dvec2(0.0));
        m_mouseInitialized = std::exchange(other.m_mouseInitialized, false);
    }
    return *this;
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_glfwWindow);
}

void Window::pollEvents() {
    glfwPollEvents();
}

void Window::endFrame() {
    m_mouseDelta = glm::dvec2(0.0);
}

bool Window::isKeyDown(int key) const {
    return glfwGetKey(m_glfwWindow, key) == GLFW_PRESS;
}

bool Window::isMouseDown(int button) const {
    return glfwGetMouseButton(m_glfwWindow, button) == GLFW_PRESS;
}

void Window::requestClose() {
    glfwSetWindowShouldClose(m_glfwWindow, GLFW_TRUE);
}

void Window::setTitle(const char* title) {
    glfwSetWindowTitle(m_glfwWindow, title);
}

std::vector<const char*> Window::getRequiredInstanceExtensions() const {
    uint32_t count = 0;
    const char** extensions = glfwGetRequiredInstanceExtensions(&count);

    return { extensions, extensions + count };
}

vk::raii::SurfaceKHR Window::createSurface(const vk::raii::Instance& instance) const {
    VkSurfaceKHR raw;
    if(glfwCreateWindowSurface(*instance, m_glfwWindow, nullptr, &raw) != VK_SUCCESS){
        throw std::runtime_error("Failed to create window surface!");
    }

    return vk::raii::SurfaceKHR(instance, raw);
}

vk::Extent2D Window::getFramebufferExtent() {
    int width, height;
    glfwGetFramebufferSize(m_glfwWindow, &width, &height);
    return vk::Extent2D {
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height)
    };
}

void Window::waitWhileMinimized() {
    vk::Extent2D framebufferExtent = getFramebufferExtent();
    while(framebufferExtent.width == 0 || framebufferExtent.height == 0){
        glfwWaitEvents();
        framebufferExtent = getFramebufferExtent();
    }
}