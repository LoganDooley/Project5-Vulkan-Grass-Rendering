#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vma/vk_mem_alloc.h>

#include <utility>

class Image {
public:
    Image() = default;
    Image(VmaAllocator allocator, const vk::raii::Device& device, 
        vk::Extent2D extent, vk::Format format, 
        vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect);
    ~Image();

    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    Image(Image&& other) noexcept;
    Image& operator=(Image&& other) noexcept;

    vk::Image getImage() const { return m_image; }
    vk::ImageView getImageView() const { return *m_imageView; }
    vk::Format getFormat() const { return m_format; }
    vk::Extent2D getExtent() const { return m_extent; }

private:
    void destroy();

    VmaAllocator m_allocator = nullptr;
    VkImage m_image = VK_NULL_HANDLE; // Require raw VkImage to work with VMA
    VmaAllocation m_allocation = nullptr;
    vk::raii::ImageView m_imageView{nullptr};
    vk::Format m_format = vk::Format::eUndefined;
    vk::Extent2D m_extent{};
};