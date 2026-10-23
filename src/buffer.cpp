#include "buffer.h"

#include <cstring>
#include <stdexcept>

Buffer::Buffer(VmaAllocator allocator, 
    const vk::raii::Device &device, 
    vk::DeviceSize size, 
    vk::BufferUsageFlags usage, 
    bool hostVisible) :
    m_allocator(allocator),
    m_size(size)
{
    vk::BufferCreateInfo bufferInfo = {
        .size = size,
        .usage = usage | vk::BufferUsageFlagBits::eShaderDeviceAddress
    };

    VmaAllocationCreateInfo allocInfo = {
        .usage = VMA_MEMORY_USAGE_AUTO
    };
    if(hostVisible) {
        allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VmaAllocationInfo resultInfo{};
    if(vmaCreateBuffer(allocator, 
        &static_cast<const VkBufferCreateInfo&>(bufferInfo),
        &allocInfo, &m_buffer, &m_allocation, 
        &resultInfo) != VK_SUCCESS) 
    {
        throw std::runtime_error("vmaCreateBuffer failed!");
    }
    m_mapped = resultInfo.pMappedData;
    m_address = device.getBufferAddress(vk::BufferDeviceAddressInfo {
        .buffer = m_buffer
    });
}

Buffer::~Buffer()
{
    destroy();
}

Buffer::Buffer(Buffer &&other) noexcept :
    m_allocator(std::exchange(other.m_allocator, nullptr)),
    m_buffer(std::exchange(other.m_buffer, VK_NULL_HANDLE)),
    m_allocation(std::exchange(other.m_allocation, nullptr)),
    m_size(std::exchange(other.m_size, 0)),
    m_address(std::exchange(other.m_address, 0)),
    m_mapped(std::exchange(other.m_mapped, nullptr))
{
}

Buffer &Buffer::operator=(Buffer &&other) noexcept
{
    if(this != &other) {
        destroy();
        m_allocator = std::exchange(other.m_allocator, nullptr);
        m_buffer = std::exchange(other.m_buffer, VK_NULL_HANDLE);
        m_allocation = std::exchange(other.m_allocation, nullptr);
        m_size = std::exchange(other.m_size, 0);
        m_address = std::exchange(other.m_address, 0);
        m_mapped = std::exchange(other.m_mapped, nullptr);
    }

    return *this;
}

void Buffer::write(const void *data, vk::DeviceSize bytes, vk::DeviceSize offset)
{
    if(!m_mapped) {
        throw std::runtime_error("Buffer:write can only be called on host-visible buffers!");
    }
    std::memcpy(static_cast<char*>(m_mapped) + offset, data, bytes);
    vmaFlushAllocation(m_allocator, m_allocation, offset, bytes);
}

void Buffer::destroy()
{
    if(m_buffer != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_buffer, m_allocation);
        m_buffer = VK_NULL_HANDLE;
    }
}
