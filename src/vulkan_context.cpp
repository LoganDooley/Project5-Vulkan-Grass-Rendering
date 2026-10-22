#include "vulkan_context.h"

#include "window.h"

#include <iostream>
#include <optional>
#include <cstring>

#ifdef NDEBUG
constexpr bool kEnableValidation = false;
#else
constexpr bool kEnableValidation = true;
#endif

static vk::Bool32 debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity,
                                [[maybe_unused]] vk::DebugUtilsMessageTypeFlagsEXT type,
                                const vk::DebugUtilsMessengerCallbackDataEXT* data,
                                [[maybe_unused]] void* userData) 
{
    const char* severityPrefix = (severity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) ? "[ERROR] " : "[WARN] ";
    std::cerr << severityPrefix << data->pMessage << "\n";
    return vk::False;
}

VulkanContext::VulkanContext(const Window &window)
{
    createInstance(window);
    createDebugMessenger();
    createSurface(window);
    pickPhysicalDevice();
    createDevice();
    createAllocator();
}

void VulkanContext::createInstance(const Window &window)
{
    vk::ApplicationInfo appInfo = {
        .pApplicationName = "Vulkan Grass",
        .applicationVersion = vk::makeApiVersion(0, 1, 0, 0),
        .pEngineName = "Blade",
        .engineVersion = vk::makeApiVersion(0, 1, 0, 0),
        .apiVersion = vk::ApiVersion14
    };

    std::vector<const char*> extensions = window.getRequiredInstanceExtensions();
    if(kEnableValidation){
        extensions.push_back(vk::EXTDebugUtilsExtensionName);
    }

    std::vector<const char*> layers;
    if(kEnableValidation){
        layers.emplace_back("VK_LAYER_KHRONOS_validation");
    }

    vk::InstanceCreateInfo createInfo = {
        .pApplicationInfo = &appInfo,
        .enabledLayerCount = static_cast<uint32_t>(layers.size()),
        .ppEnabledLayerNames = layers.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
    };

    m_instance = vk::raii::Instance(m_context, createInfo);
}

void VulkanContext::createDebugMessenger()
{
    if(!kEnableValidation){
        return;
    }

    vk::DebugUtilsMessengerCreateInfoEXT createInfo = {
        .messageSeverity = vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | 
            vk::DebugUtilsMessageSeverityFlagBitsEXT::eError,
        .messageType = vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | 
            vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation |
            vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance,
        .pfnUserCallback = &debugCallback
    };

    m_debugMessenger = vk::raii::DebugUtilsMessengerEXT(m_instance, createInfo);
}

void VulkanContext::createSurface(const Window &window)
{
    m_surface = window.createSurface(m_instance);
}

void VulkanContext::pickPhysicalDevice()
{
    for(const auto& option : m_instance.enumeratePhysicalDevices()){
        if(option.getProperties().apiVersion < vk::ApiVersion14){
            continue;
        }

        std::vector<vk::ExtensionProperties> extensionProperties = option.enumerateDeviceExtensionProperties();
        bool bFoundSwapchainExtension = false;
        bool bFoundMeshShaderExtension = false;
        for(const auto& extension : extensionProperties){
            if(std::strcmp(extension.extensionName, vk::KHRSwapchainExtensionName) == 0){
                bFoundSwapchainExtension = true;
            }
            if(std::strcmp(extension.extensionName, vk::EXTMeshShaderExtensionName) == 0){
                bFoundMeshShaderExtension = true;
            }
        }

        if(!bFoundSwapchainExtension || !bFoundMeshShaderExtension){
            continue;
        }

        const auto features = option.getFeatures2<vk::PhysicalDeviceFeatures2, 
            vk::PhysicalDeviceVulkan12Features, 
            vk::PhysicalDeviceVulkan13Features, 
            vk::PhysicalDeviceMeshShaderFeaturesEXT>();

        if(!features.get<vk::PhysicalDeviceVulkan12Features>().bufferDeviceAddress ||
            !features.get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering ||
            !features.get<vk::PhysicalDeviceVulkan13Features>().synchronization2 ||
            !features.get<vk::PhysicalDeviceMeshShaderFeaturesEXT>().taskShader ||
            !features.get<vk::PhysicalDeviceMeshShaderFeaturesEXT>().meshShader)
        {
            continue;
        }
        
        const auto queueFamilyProperties = option.getQueueFamilyProperties();
        std::optional<uint32_t> sufficientQueueFamily = std::nullopt;
        for(uint32_t i = 0; i < queueFamilyProperties.size(); i++){
            const auto& queueFamily = queueFamilyProperties[i];
            
            if(!option.getSurfaceSupportKHR(i, *m_surface)){
                continue;
            }

            if((queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) && (queueFamily.queueFlags & vk::QueueFlagBits::eCompute))
            {
                sufficientQueueFamily = i;
                break;  
            }
        }

        if(!sufficientQueueFamily.has_value()){
            continue;
        }

        m_physicalDevice = option;
        m_queueFamilyIndex = sufficientQueueFamily.value();

        if(option.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu){
            break;
        }
    }

    if (!*m_physicalDevice) {
        throw std::runtime_error("No GPU supports Vulkan 1.4 + mesh shaders");
    }

    std::cout<< "GPU: " << m_physicalDevice.getProperties().deviceName.data() << "\n";
}

void VulkanContext::createDevice()
{
    float priority = 1.0f;
    vk::DeviceQueueCreateInfo queueInfo = {
        .queueFamilyIndex = m_queueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &priority
    };

    const vk::StructureChain<vk::PhysicalDeviceFeatures2, 
        vk::PhysicalDeviceVulkan12Features, 
        vk::PhysicalDeviceVulkan13Features, 
        vk::PhysicalDeviceMeshShaderFeaturesEXT> features
    {
        {},
        { .bufferDeviceAddress = vk::True },
        { .synchronization2 = vk::True, .dynamicRendering = vk::True },
        { .taskShader = vk::True, .meshShader = vk::True },
    };

    std::vector<const char*> deviceExtensions = {
        vk::KHRSwapchainExtensionName,
        vk::EXTMeshShaderExtensionName
    };

    vk::DeviceCreateInfo createInfo = {
        .pNext = &features.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data()
    };

    m_device = vk::raii::Device(m_physicalDevice, createInfo);
    m_queue = vk::raii::Queue(m_device, m_queueFamilyIndex, 0);
}

void VulkanContext::createAllocator()
{
    VmaVulkanFunctions functions = {
        .vkGetInstanceProcAddr = m_context.getDispatcher()->vkGetInstanceProcAddr,
        .vkGetDeviceProcAddr = m_instance.getDispatcher()->vkGetDeviceProcAddr
    };

    VmaAllocatorCreateInfo createInfo = {
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = *m_physicalDevice,
        .device = *m_device,
        .pVulkanFunctions = &functions,
        .instance = *m_instance,
        .vulkanApiVersion = vk::ApiVersion14
    };

    m_allocator = VmaAllocatorHandle(createInfo);
}
