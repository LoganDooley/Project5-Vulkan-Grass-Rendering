#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <vector>

class VulkanContext;
class Window;

class Swapchain {
public:
    Swapchain(const VulkanContext& context, Window& window, vk::PresentModeKHR preferredMode, vk::SwapchainKHR oldSwapchain = nullptr);

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    Swapchain(Swapchain&& other) noexcept = default;
    Swapchain& operator=(Swapchain&& other) noexcept = default;

    const vk::raii::SwapchainKHR& getSwapchain() const { return m_swapchain; }
    uint32_t getImageCount() const { return static_cast<uint32_t>(m_images.size()); }
    vk::Image getImageUnchecked(uint32_t index) const { return m_images[index]; }
    vk::ImageView getImageViewUnchecked(uint32_t index) const { return *m_imageViews[index]; }
    vk::Format getFormat() const { return m_format; }
    vk::Extent2D getExtent() const { return m_extent; }

private:
    static vk::SurfaceFormatKHR chooseFormat(const std::vector<vk::SurfaceFormatKHR>& formats);
    static vk::PresentModeKHR choosePresentMode(const std::vector<vk::PresentModeKHR>& presentModes, vk::PresentModeKHR preferredMode);
    static vk::Extent2D chooseExtent(const vk::SurfaceCapabilitiesKHR& capabilities, Window& window);

    vk::raii::SwapchainKHR m_swapchain{nullptr};
    std::vector<vk::Image> m_images;
    std::vector<vk::raii::ImageView> m_imageViews;
    vk::Format m_format = vk::Format::eUndefined;
    vk::Extent2D m_extent{};
};