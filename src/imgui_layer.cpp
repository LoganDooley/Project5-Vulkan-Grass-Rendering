#include "imgui_layer.h"

#include "vulkan_context.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

ImGuiLayer::ImGuiLayer(const VulkanContext &context, 
    GLFWwindow *window, 
    uint32_t imageCount, 
    vk::Format colorFormat, vk::Format depthFormat) :
    m_colorFormat(static_cast<VkFormat>(colorFormat))
{
    // Create imgui context

    // Load vulkan functions

    // Init glfw backend

    // Init imgui vulkan
}

ImGuiLayer::~ImGuiLayer()
{
}

void ImGuiLayer::beginFrame()
{
}

void ImGuiLayer::endFrame()
{
}

void ImGuiLayer::record(const vk::raii::CommandBuffer &cmd) const
{
}
