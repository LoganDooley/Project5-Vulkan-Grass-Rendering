#pragma once

#include <vulkan/vulkan_raii.hpp>

class VulkanContext;
struct GLFWwindow;

class ImGuiLayer {
public:
    ImGuiLayer(const VulkanContext& context, GLFWwindow* window, 
        uint32_t imageCount, vk::Format colorFormat, vk::Format depthFormat);
    
    ~ImGuiLayer();

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;
    ImGuiLayer(ImGuiLayer&&) = delete;
    ImGuiLayer& operator=(ImGuiLayer&&) = delete;

    void beginFrame();
    void endFrame();
    void record(const vk::raii::CommandBuffer& cmd) const;

private:
    VkFormat m_colorFormat = VK_FORMAT_UNDEFINED;
};