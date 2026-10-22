#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <vma/vk_mem_alloc.h>

#include <stdexcept>
#include <utility>

class Window;

struct VmaAllocatorHandle {
    VmaAllocatorHandle() = default;
    explicit VmaAllocatorHandle(const VmaAllocatorCreateInfo& createInfo){
        if(vmaCreateAllocator(&createInfo, &handle) != VK_SUCCESS){
            throw std::runtime_error("Failed to create vma allocator!");
        }
    }

    ~VmaAllocatorHandle() {
        if(handle != VK_NULL_HANDLE){
            vmaDestroyAllocator(handle);
        }
    }

    VmaAllocatorHandle(const VmaAllocatorHandle&) = delete;
    VmaAllocatorHandle& operator=(const VmaAllocatorHandle&) = delete;

    VmaAllocatorHandle(VmaAllocatorHandle&& other) noexcept :
        handle(std::exchange(other.handle, VK_NULL_HANDLE))
    {

    }

    VmaAllocatorHandle& operator=(VmaAllocatorHandle&& other) noexcept {
        if (this != &other) {
            if (handle) {
                vmaDestroyAllocator(handle);
            }
            handle = std::exchange(other.handle, VK_NULL_HANDLE);
        }
        return *this;
    }

    VmaAllocator handle = VK_NULL_HANDLE;
};

class VulkanContext {
public:
    explicit VulkanContext(const Window& window);
    
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) = delete;
    VulkanContext& operator=(VulkanContext&&) = delete;

    const vk::raii::Instance& getInstance() const { return m_instance; }
    const vk::raii::PhysicalDevice& getPhysicalDevice() const { return m_physicalDevice; }
    const vk::raii::Device& getDevice() const { return m_device; }
    const vk::raii::SurfaceKHR& getSurface() const { return m_surface; }
    const vk::raii::Queue& getQueue() const { return m_queue; }
    uint32_t getQueueFamilyIndex() const { return m_queueFamilyIndex; }

private:
    void createInstance(const Window& window);
    void createDebugMessenger();
    void createSurface(const Window& window);
    void pickPhysicalDevice();
    void createDevice();
    void createAllocator();
    VmaAllocator getAllocator() const { return m_allocator.handle; }

    vk::raii::Context m_context;
    vk::raii::Instance m_instance{nullptr};
    vk::raii::DebugUtilsMessengerEXT m_debugMessenger{nullptr};
    vk::raii::SurfaceKHR m_surface{nullptr};
    vk::raii::PhysicalDevice m_physicalDevice{nullptr};
    vk::raii::Device m_device{nullptr};
    VmaAllocatorHandle m_allocator;
    vk::raii::Queue m_queue{nullptr};
    uint32_t m_queueFamilyIndex = 0;
};