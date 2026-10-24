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
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    // Load vulkan functions
    struct LoaderData {
        PFN_vkGetInstanceProcAddr getProcAddr;
        VkInstance instance;
    };
    LoaderData loader {
        context.getInstanceProcAddr(),
        *context.getInstance()
    };

    ImGui_ImplVulkan_LoadFunctions(VK_API_VERSION_1_4, 
        [](const char* name, void* userData) -> PFN_vkVoidFunction {
            auto* d = static_cast<LoaderData*>(userData);
            return d->getProcAddr(d->instance, name);
        },
        &loader);

    // Init glfw backend
    ImGui_ImplGlfw_InitForVulkan(window, true);

    // Init imgui vulkan
    ImGui_ImplVulkan_InitInfo info = {
        .ApiVersion = VK_API_VERSION_1_4,
        .Instance = *context.getInstance(),
        .PhysicalDevice = *context.getPhysicalDevice(),
        .Device = *context.getDevice(),
        .QueueFamily = context.getQueueFamilyIndex(),
        .Queue = *context.getQueue(),
        .DescriptorPoolSize = 16,
        .MinImageCount = 2,
        .ImageCount = imageCount,
        .UseDynamicRendering = true
    };

    VkPipelineRenderingCreateInfo& renderingInfo = info.PipelineInfoMain.PipelineRenderingCreateInfo;
    renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    renderingInfo.colorAttachmentCount = 1;
    renderingInfo.pColorAttachmentFormats = &m_colorFormat;
    renderingInfo.depthAttachmentFormat = static_cast<VkFormat>(depthFormat);

    ImGui_ImplVulkan_Init(&info);
}

ImGuiLayer::~ImGuiLayer()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::beginFrame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::endFrame()
{
    ImGui::Render();
}

void ImGuiLayer::record(const vk::raii::CommandBuffer &cmd) const
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *cmd);
}
