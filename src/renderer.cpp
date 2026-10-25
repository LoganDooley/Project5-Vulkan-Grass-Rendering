#include "renderer.h"

#include "vulkan_context.h"
#include "window.h"
#include "shader_utils.h"
#include "camera.h"
#include "pipeline_utils.h"
#include "render_settings.h"

#include <array>
#include <stdexcept>
#include <limits>

Renderer::Renderer(const VulkanContext &context, Window &window) :
    m_context(context),
    m_window(window),
    m_swapchain(context, window, m_presentMode),
    m_depthImage(m_context.getAllocator(),
        m_context.getDevice(),
        m_swapchain.getExtent(),
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth),
    m_imGuiLayer(m_context, 
        m_window.getHandle(), 
        m_swapchain.getImageCount(), 
        m_swapchain.getFormat(), 
        m_depthImage.getFormat()),
    m_grass(m_context, 1u << 20, 100.0f, m_swapchain.getFormat(), m_depthImage.getFormat())
{
    // Create command pool
    vk::CommandPoolCreateInfo commandPoolCreateInfo = {
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = context.getQueueFamilyIndex()
    };
    m_commandPool = vk::raii::CommandPool(context.getDevice(), commandPoolCreateInfo);

    // Create command buffers
    vk::CommandBufferAllocateInfo commandBufferAllocInfo = {
        .commandPool = *m_commandPool,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = static_cast<uint32_t>(m_frames.size())
    };
    vk::raii::CommandBuffers buffers(context.getDevice(), commandBufferAllocInfo);
    
    // Populate per frame data
    for(uint32_t i = 0; i < m_frames.size(); i++) {
        m_frames[i].commandBuffer = std::move(buffers[i]);
        vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
        m_frames[i].imageAvailableSemaphore = vk::raii::Semaphore(context.getDevice(), semaphoreCreateInfo);
        vk::FenceCreateInfo fenceCreateInfo = {
            .flags = vk::FenceCreateFlagBits::eSignaled
        };
        m_frames[i].inFlightFence = vk::raii::Fence(context.getDevice(), fenceCreateInfo);
        m_frames[i].cameraBuffer = Buffer(m_context.getAllocator(), 
            m_context.getDevice(), 
            sizeof(CameraData),
            vk::BufferUsageFlagBits::eStorageBuffer,
            true);
    }

    // Create render finished semaphores
    m_renderFinishedSemaphores.reserve(m_swapchain.getImageCount());
    for (uint32_t i = 0; i < m_swapchain.getImageCount(); i++){
        vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
        m_renderFinishedSemaphores.emplace_back(context.getDevice(), semaphoreCreateInfo);
    }

    m_positionsBuffer = Buffer(m_context.getAllocator(), m_context.getDevice(), 3 * sizeof(glm::vec4), vk::BufferUsageFlagBits::eStorageBuffer, false);

    // Create pipeline layouts
    createMeshPipelineLayout();
    createAnimPipelineLayout();

    // Create pipelines
    createTrianglePipeline();
    createGroundPipeline();
    createAnimPipeline();

    // Perform grass init immediately
    m_context.submitImmediate([&](const vk::raii::CommandBuffer& cmd) {
        m_grass.recordInit(cmd);
    });
}

void Renderer::drawFrame(const Camera& camera, float dt, float totalTime, const RenderSettings& renderSettings)
{
    FrameData& frameData = m_frames[m_frameIndex];

    // Wait for in flight fence
    if(m_context.getDevice().waitForFences(*frameData.inFlightFence, vk::True, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess){
        throw std::runtime_error("Wait for in flight fence failed!");
    }

    // Check for present mode changed
    vk::PresentModeKHR desiredPresentMode = renderSettings.bUseVsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate;
    if(desiredPresentMode != m_presentMode){
        m_presentMode = desiredPresentMode;
        recreateSwapchain();
    }

    // Update camera data
    vk::Extent2D extent = m_swapchain.getExtent();
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    CameraData cameraData = camera.getCameraData(aspect);
    frameData.cameraBuffer.write(&cameraData, sizeof(cameraData));

    // Acquire swapchain image
    uint32_t imageIndex;
    try
    {
        auto acquired = m_swapchain.getSwapchain().acquireNextImage(std::numeric_limits<uint64_t>::max(), *frameData.imageAvailableSemaphore);
        imageIndex = acquired.value;
    }
    catch(const vk::OutOfDateKHRError&)
    {
        recreateSwapchain();
        return;
    }
    
    // Reset in flight fence
    m_context.getDevice().resetFences(*frameData.inFlightFence);

    // Record command buffer
    const vk::raii::CommandBuffer& commandBuffer = frameData.commandBuffer;
    vk::Image image = m_swapchain.getImageUnchecked(imageIndex);

    commandBuffer.reset();
    commandBuffer.begin(vk::CommandBufferBeginInfo{
        .flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit
    });

    // - Wait for mesh shader before we overwrite
    vk::MemoryBarrier2 readBeforeWrite = {
        .srcStageMask  = vk::PipelineStageFlagBits2::eTaskShaderEXT | vk::PipelineStageFlagBits2::eMeshShaderEXT,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
        .dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
    };

    commandBuffer.pipelineBarrier2(vk::DependencyInfo{ 
        .memoryBarrierCount = 1, 
        .pMemoryBarriers = &readBeforeWrite 
    });

    // - Run compute shader
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *m_animPipeline);
    AnimPushConstants animPc = { 
        .positions = m_positionsBuffer.getDeviceAddress(), 
        .time = totalTime 
    };
    commandBuffer.pushConstants<AnimPushConstants>(*m_animPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, animPc);
    commandBuffer.dispatch(1, 1, 1);

    // - Finish writes before read
    vk::MemoryBarrier2 writeBeforeRead = {
        .srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
        .dstStageMask  = vk::PipelineStageFlagBits2::eTaskShaderEXT | vk::PipelineStageFlagBits2::eMeshShaderEXT,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
    };
    commandBuffer.pipelineBarrier2(vk::DependencyInfo{ 
        .memoryBarrierCount = 1, 
        .pMemoryBarriers = &writeBeforeRead 
    });

    // - Run grass sim
    if(renderSettings.bSimulate){
        m_grass.simulate(commandBuffer, dt, totalTime, renderSettings);
    }

    // - Prepare image to be cleared
    transitionImage(commandBuffer, image, 
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::ImageAspectFlagBits::eColor);

    transitionImage(commandBuffer, m_depthImage.getImage(),
        vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::PipelineStageFlagBits2::eEarlyFragmentTests | vk::PipelineStageFlagBits2::eLateFragmentTests,
        vk::AccessFlagBits2::eDepthStencilAttachmentRead | vk::AccessFlagBits2::eDepthStencilAttachmentWrite,
        vk::ImageAspectFlagBits::eDepth);

    // - Clear image
    vk::RenderingAttachmentInfo colorAttachment = {
        .imageView = m_swapchain.getImageViewUnchecked(imageIndex),
        .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .clearValue = {
            .color = {
                .float32 = std::array<float, 4>{
                    0.1f, 0.2f, 0.3f, 1.0f
                }
            }
        }
    };

    vk::RenderingAttachmentInfo depthAttachment = {
        .imageView = m_depthImage.getImageView(),
        .imageLayout = vk::ImageLayout::eDepthAttachmentOptimal,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eDontCare,
        .clearValue = { 
            .depthStencil = { 
                .depth = 0.0f, 
                .stencil = 0 
            } 
        },
    };

    vk::RenderingInfo renderingInfo = {
        .renderArea = {
            .extent = extent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment,
        .pDepthAttachment = &depthAttachment,
    };

    commandBuffer.beginRendering(renderingInfo);

    // Set viewport and scissor
    commandBuffer.setViewport(0, 
        vk::Viewport {
            .x = 0.0f,
            .y = 0.0f,
            .width = static_cast<float>(extent.width),
            .height = static_cast<float>(extent.height),
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        }
    );
    commandBuffer.setScissor(0, 
        vk::Rect2D {
            .offset = { 0, 0 },
            .extent = extent
        }
    );

    // Create push constants
    PushConstants pc = {
        .camera = frameData.cameraBuffer.getDeviceAddress(),
        .positions = m_positionsBuffer.getDeviceAddress()
    };

    // Run ground pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_groundPipeline);
    commandBuffer.pushConstants<PushConstants>(*m_meshPipelineLayout, m_pcStages, 0, pc);
    commandBuffer.drawMeshTasksEXT(1, 1, 1);

    // Run grass pipeline
    m_grass.draw(commandBuffer, frameData.cameraBuffer.getDeviceAddress(), renderSettings);

    // Run triangle pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_trianglePipeline);
    commandBuffer.pushConstants<PushConstants>(*m_meshPipelineLayout, m_pcStages, 0, pc);
    commandBuffer.drawMeshTasksEXT(2, 1, 1);

    // Draw imgui
    m_imGuiLayer.record(commandBuffer);

    commandBuffer.endRendering();

    // - Convert to present mode
    transitionImage(commandBuffer, image,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe, vk::AccessFlagBits2::eNone,
        vk::ImageAspectFlagBits::eColor);

    commandBuffer.end();

    // Submit command buffer
    vk::SemaphoreSubmitInfo waitInfo = {
        .semaphore = *frameData.imageAvailableSemaphore,
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput,
    };

    vk::SemaphoreSubmitInfo signalInfo = {
        .semaphore = *m_renderFinishedSemaphores[imageIndex],
        .stageMask = vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };

    vk::CommandBufferSubmitInfo commandBufferInfo = {
        .commandBuffer = *commandBuffer
    };

    vk::SubmitInfo2 submitInfo = {
        .waitSemaphoreInfoCount = 1,
        .pWaitSemaphoreInfos = &waitInfo,
        .commandBufferInfoCount = 1,
        .pCommandBufferInfos = &commandBufferInfo,
        .signalSemaphoreInfoCount = 1,
        .pSignalSemaphoreInfos = &signalInfo
    };

    m_context.getQueue().submit2(submitInfo, *frameData.inFlightFence);

    // Present to swapchain
    vk::SwapchainKHR swapchain = *m_swapchain.getSwapchain();
    vk::Semaphore renderFinished = *m_renderFinishedSemaphores[imageIndex];

    vk::PresentInfoKHR presentInfo = {
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &renderFinished,
        .swapchainCount = 1,
        .pSwapchains = &swapchain,
        .pImageIndices = &imageIndex
    };

    bool requiresRecreate = false;
    try
    {
        vk::Result result = m_context.getQueue().presentKHR(presentInfo);
        requiresRecreate = (result == vk::Result::eSuboptimalKHR);
    }
    catch(const vk::OutOfDateKHRError&)
    {
        requiresRecreate = true;
    }

    // Update frame index
    m_frameIndex = (m_frameIndex + 1) % m_frames.size();

    if(requiresRecreate || m_window.wasResized()) {
        recreateSwapchain();
    }
}

void Renderer::transitionImage(const vk::raii::CommandBuffer &cmd, vk::Image image, 
    vk::ImageLayout oldLayout, vk::ImageLayout newLayout, 
    vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess, 
    vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess,
    vk::ImageAspectFlags aspect)
{
    vk::ImageMemoryBarrier2 barrier = {
        .srcStageMask = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask = dstStage,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange = {
            .aspectMask = aspect, 
            .levelCount = 1, 
            .layerCount = 1 
        }
    };

    cmd.pipelineBarrier2(vk::DependencyInfo{
        .imageMemoryBarrierCount = 1,
        .pImageMemoryBarriers = &barrier
    });
}

void Renderer::recreateSwapchain()
{
    m_window.waitWhileMinimized();
    m_context.getDevice().waitIdle();

    m_swapchain = Swapchain(m_context, m_window, m_presentMode, *m_swapchain.getSwapchain());

    // Rebuild per-image semaphores
    m_renderFinishedSemaphores.clear();
    for (uint32_t i = 0; i < m_swapchain.getImageCount(); i++) {
        m_renderFinishedSemaphores.emplace_back(m_context.getDevice(), vk::SemaphoreCreateInfo{});
    }

    createDepthImage();

    m_window.clearResized();
}

void Renderer::createMeshPipelineLayout()
{
    m_pcStages = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment;
    vk::PushConstantRange pcRange = {
        .stageFlags = m_pcStages,
        .offset = 0,
        .size = sizeof(PushConstants)
    };
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pcRange
    };
    m_meshPipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);
}

void Renderer::createAnimPipelineLayout()
{
    vk::PushConstantRange range = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(AnimPushConstants)
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range
    };
    m_animPipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);
}

void Renderer::createTrianglePipeline()
{
    const vk::raii::Device& device = m_context.getDevice();
    
    // Create shader modules
    vk::raii::ShaderModule taskModule = ShaderUtils::createShaderModule(device, "shaders/triangle.task.spv");
    vk::raii::ShaderModule meshModule = ShaderUtils::createShaderModule(device, "shaders/triangle.mesh.spv");
    vk::raii::ShaderModule fragModule = ShaderUtils::createShaderModule(device, "shaders/triangle.frag.spv");

    std::vector<vk::PipelineShaderStageCreateInfo> stages = {
        {
            .stage = vk::ShaderStageFlagBits::eTaskEXT,
            .module = *taskModule,
            .pName = "taskMain"
        },
        {
            .stage = vk::ShaderStageFlagBits::eMeshEXT,
            .module = *meshModule,
            .pName = "meshMain"
        },
        {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *fragModule,
            .pName = "fragMain"
        }
    };

    m_trianglePipeline = PipelineUtils::createMeshPipeline(device, stages, m_meshPipelineLayout, m_swapchain.getFormat(), m_depthImage.getFormat());
}

void Renderer::createGroundPipeline()
{
    const vk::raii::Device& device = m_context.getDevice();
    
    // Create shader modules
    vk::raii::ShaderModule meshModule = ShaderUtils::createShaderModule(device, "shaders/ground.mesh.spv");
    vk::raii::ShaderModule fragModule = ShaderUtils::createShaderModule(device, "shaders/ground.frag.spv");
    
    std::vector<vk::PipelineShaderStageCreateInfo> stages = {
        {
            .stage = vk::ShaderStageFlagBits::eMeshEXT,
            .module = *meshModule,
            .pName = "meshMain"
        },
        {
            .stage = vk::ShaderStageFlagBits::eFragment,
            .module = *fragModule,
            .pName = "fragMain"
        }
    };

    m_groundPipeline = PipelineUtils::createMeshPipeline(device, stages, m_meshPipelineLayout, m_swapchain.getFormat(), m_depthImage.getFormat());
}

void Renderer::createAnimPipeline()
{
    vk::raii::ShaderModule module = ShaderUtils::createShaderModule(m_context.getDevice(), "shaders/triangle_anim.comp.spv");

    m_animPipeline = PipelineUtils::createComputePipeline(m_context.getDevice(), module, m_animPipelineLayout);
}

void Renderer::createDepthImage()
{
    m_depthImage = Image(m_context.getAllocator(),
        m_context.getDevice(),
        m_swapchain.getExtent(),
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth);
}
