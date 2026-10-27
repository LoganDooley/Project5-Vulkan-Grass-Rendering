#include "renderer.h"

#include "vulkan_context.h"
#include "window.h"
#include "camera.h"
#include "render_settings.h"

#include <array>
#include <stdexcept>
#include <limits>

Renderer::Renderer(const VulkanContext &context, Window &window, const RenderSettings& renderSettings) :
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
    m_terrain(m_context, m_swapchain.getFormat(), m_depthImage.getFormat(), renderSettings.terrainType),
    m_grass(m_context, renderSettings.grassDensity, renderSettings.grassRadius, m_swapchain.getFormat(), m_depthImage.getFormat()),
    m_profiler(context, static_cast<uint32_t>(m_frames.size()))
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
            HostAccess::Write);
        m_frames[i].cullCameraBuffer = Buffer(m_context.getAllocator(), 
            m_context.getDevice(), 
            sizeof(CameraData),
            vk::BufferUsageFlagBits::eStorageBuffer,
            HostAccess::Write);
        m_frames[i].visibleCounter = Buffer(m_context.getAllocator(), 
            m_context.getDevice(), 
            sizeof(uint32_t),
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
            HostAccess::None);
        m_frames[i].visibleReadback = Buffer(m_context.getAllocator(), 
            m_context.getDevice(), 
            sizeof(uint32_t),
            vk::BufferUsageFlagBits::eTransferDst,
            HostAccess::Readback);
    }

    // Create render finished semaphores
    m_renderFinishedSemaphores.reserve(m_swapchain.getImageCount());
    for (uint32_t i = 0; i < m_swapchain.getImageCount(); i++){
        vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
        m_renderFinishedSemaphores.emplace_back(context.getDevice(), semaphoreCreateInfo);
    }

    // Set terrain type
    m_grass.setTerrainType(renderSettings.terrainType);
}

void Renderer::drawFrame(const Camera& camera, float dt, float totalTime, const RenderSettings& renderSettings)
{
    FrameData& frameData = m_frames[m_frameIndex];

    // Wait for in flight fence
    if(m_context.getDevice().waitForFences(*frameData.inFlightFence, vk::True, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess){
        throw std::runtime_error("Wait for in flight fence failed!");
    }

    m_profiler.collect(m_frameIndex);
    frameData.visibleReadback.read(&m_visibleBlades, sizeof(uint32_t), 0);

    // Check for present mode changed
    vk::PresentModeKHR desiredPresentMode = renderSettings.bUseVsync ? vk::PresentModeKHR::eFifo : vk::PresentModeKHR::eImmediate;
    if(desiredPresentMode != m_presentMode){
        m_presentMode = desiredPresentMode;
        recreateSwapchain();
    }

    // Check for the grass density changing
    bool resize = renderSettings.grassDensity != m_grass.getDensity() || 
        renderSettings.grassRadius != m_grass.getRadius();
    bool reinit = resize || renderSettings.terrainType != m_grass.getTerrainType();
    if(reinit){
        m_context.getDevice().waitIdle();
        if(resize){
            m_grass.resize(renderSettings.grassDensity, renderSettings.grassRadius);
        }
        m_grass.setTerrainType(renderSettings.terrainType);
    }

    // Check for the terrain type changing
    m_terrain.setType(renderSettings.terrainType);

    // Update camera data
    vk::Extent2D extent = m_swapchain.getExtent();
    float aspect = static_cast<float>(extent.width) / static_cast<float>(extent.height);

    CameraData cameraData = camera.getCameraData(aspect);
    frameData.cameraBuffer.write(&cameraData, sizeof(cameraData));
    
    // Update cull camera data
    if(renderSettings.bFreezeCulling && !m_cullCameraFrozen){
        m_frozenCullCamera = cameraData;
        m_cullCameraFrozen = true;
    }
    else if(!renderSettings.bFreezeCulling){
        m_cullCameraFrozen = false;
    }
    const CameraData& cullCamera = renderSettings.bFreezeCulling ? m_frozenCullCamera : cameraData;
    frameData.cullCameraBuffer.write(&cullCamera, sizeof(CameraData));

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

    m_profiler.beginFrame(commandBuffer, m_frameIndex);

    // Zero out the visible counter
    commandBuffer.fillBuffer(frameData.visibleCounter.getBuffer(), 0, sizeof(uint32_t), 0);
    memoryBarrier(commandBuffer, vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eTaskShaderEXT,
        vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite);

    // Update grass
    m_grass.update(commandBuffer, camera.getPosition(), renderSettings.bGrassFollowsCamera);

    // - Run grass sim
    m_profiler.mark(commandBuffer, GpuTimestamp::SimStart);
    if(renderSettings.bSimulate){
        m_grass.simulate(commandBuffer, dt, totalTime, renderSettings);
    }
    m_profiler.mark(commandBuffer, GpuTimestamp::SimEnd);

    // Update the terrain if needed
    m_terrain.update(commandBuffer, camera.getPosition());

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

    // Run terrain pipeline
    m_terrain.draw(commandBuffer, frameData.cameraBuffer.getDeviceAddress());

    // Run grass pipeline
    m_profiler.mark(commandBuffer, GpuTimestamp::GrassStart);
    m_grass.draw(commandBuffer, 
        frameData.cameraBuffer.getDeviceAddress(), 
        frameData.cullCameraBuffer.getDeviceAddress(),
        frameData.visibleCounter.getDeviceAddress(), 
        renderSettings);
    m_profiler.mark(commandBuffer, GpuTimestamp::GrassEnd);

    // Draw imgui
    m_imGuiLayer.record(commandBuffer);

    commandBuffer.endRendering();

    // Copy the visible blade count
    memoryBarrier(commandBuffer, vk::PipelineStageFlagBits2::eTaskShaderEXT,
        vk::AccessFlagBits2::eShaderStorageWrite,
        vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferRead);

    commandBuffer.copyBuffer(frameData.visibleCounter.getBuffer(), frameData.visibleReadback.getBuffer(),
        vk::BufferCopy{
            .size = sizeof(uint32_t)
        }
    );

    memoryBarrier(commandBuffer, vk::PipelineStageFlagBits2::eTransfer,
        vk::AccessFlagBits2::eTransferWrite,
        vk::PipelineStageFlagBits2::eHost,
        vk::AccessFlagBits2::eHostRead);

    // - Convert to present mode
    transitionImage(commandBuffer, image,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe, vk::AccessFlagBits2::eNone,
        vk::ImageAspectFlagBits::eColor);

    m_profiler.mark(commandBuffer, GpuTimestamp::FrameEnd);
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
    m_profiler.endFrame();

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

void Renderer::memoryBarrier(const vk::raii::CommandBuffer &cmd, vk::PipelineStageFlags2 srcStage, vk::AccessFlags2 srcAccess, vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess)
{
    vk::MemoryBarrier2 barrier = {
        .srcStageMask  = srcStage,
        .srcAccessMask = srcAccess,
        .dstStageMask  = dstStage,
        .dstAccessMask = dstAccess,
    };

    cmd.pipelineBarrier2(vk::DependencyInfo{
        .memoryBarrierCount = 1,
        .pMemoryBarriers = &barrier
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

void Renderer::createDepthImage()
{
    m_depthImage = Image(m_context.getAllocator(),
        m_context.getDevice(),
        m_swapchain.getExtent(),
        vk::Format::eD32Sfloat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::ImageAspectFlagBits::eDepth);
}
