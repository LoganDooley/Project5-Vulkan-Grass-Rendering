#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vma/vk_mem_alloc.h>

#include <utility>

enum class HostAccess {
    None, Write, Readback
};

class Buffer {
public:
    Buffer() = default;
    Buffer(VmaAllocator allocator, const vk::raii::Device& device,
        vk::DeviceSize size, vk::BufferUsageFlags usage, HostAccess hostAccess);
    ~Buffer();

    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    vk::Buffer getBuffer() const { return m_buffer; }
    vk::DeviceAddress getDeviceAddress() const { return m_address; }
    vk::DeviceSize getSize() const { return m_size; }
    void* getMappedData() const { return m_mapped; }
    
    void write(const void* data, vk::DeviceSize bytes, vk::DeviceSize offset = 0);
    void read(void* dst, vk::DeviceSize bytes, vk::DeviceSize offset) const;

private:
    void destroy();

    VmaAllocator m_allocator = nullptr;
    VkBuffer m_buffer = VK_NULL_HANDLE; // Require raw VkBuffer to work with VMA
    VmaAllocation m_allocation = nullptr;
    vk::DeviceSize m_size = 0;
    vk::DeviceAddress m_address = 0; // GPU pointer
    void* m_mapped = nullptr; // CPU pointer
};