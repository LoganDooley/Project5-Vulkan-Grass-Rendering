#include "image.h"

#include <stdexcept>

Image::Image(VmaAllocator allocator, const vk::raii::Device& device, 
    vk::Extent2D extent, vk::Format format, 
    vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect) :
    m_allocator(allocator),
    m_format(format),
    m_extent(extent)
{
    vk::ImageCreateInfo imageInfo = {
        .imageType = vk::ImageType::e2D,
        .format = format,
        .extent = {
            .width = extent.width,
            .height = extent.height,
            .depth = 1
        },
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = usage,
        .initialLayout = vk::ImageLayout::eUndefined
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO
    };

    if(vmaCreateImage(allocator, 
        &static_cast<const VkImageCreateInfo&>(imageInfo),
        &allocInfo, &m_image, &m_allocation, 
        nullptr) != VK_SUCCESS) 
    {
        throw std::runtime_error("vmaCreateImage failed!");
    }

    vk::ImageViewCreateInfo imageViewInfo = {
        .image = m_image,
        .viewType = vk::ImageViewType::e2D,
        .format = format,
        .subresourceRange = {
            .aspectMask = aspect,
            .levelCount = 1,
            .layerCount = 1
        }
    };

    m_imageView = vk::raii::ImageView(device, imageViewInfo);
}

Image::~Image()
{
    destroy();
}

Image::Image(Image &&other) noexcept :
    m_allocator(std::exchange(other.m_allocator, nullptr)),
    m_image(std::exchange(other.m_image, VK_NULL_HANDLE)),
    m_allocation(std::exchange(other.m_allocation, nullptr)),
    m_imageView(std::exchange(other.m_imageView, nullptr)),
    m_format(std::exchange(other.m_format, vk::Format::eUndefined)),
    m_extent(std::exchange(other.m_extent, vk::Extent2D{}))
{
}

Image &Image::operator=(Image &&other) noexcept
{
    if(this != &other) {
        destroy();
        m_allocator = std::exchange(other.m_allocator, nullptr);
        m_image = std::exchange(other.m_image, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, nullptr);
        m_imageView = std::exchange(other.m_imageView, nullptr);
        m_format = std::exchange(other.m_format, vk::Format::eUndefined);
        m_extent = std::exchange(other.m_extent, vk::Extent2D{});
    }

    return *this;
}

void Image::destroy()
{
    m_imageView.clear();

    if(m_image != VK_NULL_HANDLE){
        vmaDestroyImage(m_allocator, m_image, m_allocation);
        m_image = VK_NULL_HANDLE;
    }
}
