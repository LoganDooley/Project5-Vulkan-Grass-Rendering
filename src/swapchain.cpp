#include "swapchain.h"

#include "vulkan_context.h"
#include "window.h"

#include <algorithm>
#include <stdexcept>
#include <limits>
#include <iostream>

Swapchain::Swapchain(const VulkanContext &context, Window &window, vk::PresentModeKHR preferredMode, vk::SwapchainKHR oldSwapchain)
{
    const vk::raii::PhysicalDevice& physicalDevice = context.getPhysicalDevice();
    vk::SurfaceKHR surface = *context.getSurface();

    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    // Pick settings
    vk::SurfaceFormatKHR surfaceFormat = chooseFormat(formats);
    m_format = surfaceFormat.format;

    vk::PresentModeKHR presentMode = choosePresentMode(presentModes, preferredMode);

    m_extent = chooseExtent(capabilities, window);

    uint32_t imageCount = capabilities.minImageCount + 1;
    if(capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    // Create the swapchain
    vk::SwapchainCreateInfoKHR createInfo = {
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = surfaceFormat.format,
        .imageColorSpace = surfaceFormat.colorSpace,
        .imageExtent = m_extent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .imageSharingMode = vk::SharingMode::eExclusive,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = presentMode,
        .clipped = vk::True,
        .oldSwapchain = oldSwapchain
    };

    m_swapchain = vk::raii::SwapchainKHR(context.getDevice(), createInfo);

    // Get images and create image views
    m_images = m_swapchain.getImages();
    m_imageViews.reserve(m_images.size());
    for (vk::Image image : m_images) {
        vk::ImageViewCreateInfo viewCreateInfo = {
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = m_format,
            .subresourceRange = {
                .aspectMask = vk::ImageAspectFlagBits::eColor,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
        };

        m_imageViews.emplace_back(context.getDevice(), viewCreateInfo);
    }
}

vk::SurfaceFormatKHR Swapchain::chooseFormat(const std::vector<vk::SurfaceFormatKHR> &formats)
{
    if(formats.empty()){
        throw std::runtime_error("No available formats to choose from!");
    }

    for (const auto& format : formats) {
        if(format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear){
            return format;
        }
    }

    return formats[0];
}

vk::PresentModeKHR Swapchain::choosePresentMode(const std::vector<vk::PresentModeKHR> &presentModes, vk::PresentModeKHR preferredMode)
{
    for (const auto& presentMode : presentModes) {
        if(presentMode == preferredMode){
            return presentMode;
        }
    }

    return vk::PresentModeKHR::eFifo;
}

vk::Extent2D Swapchain::chooseExtent(const vk::SurfaceCapabilitiesKHR &capabilities, Window &window)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    }

    vk::Extent2D size = window.getFramebufferExtent();
    size.width = std::clamp(size.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    size.height = std::clamp(size.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return size;
}
