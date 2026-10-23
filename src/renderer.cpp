#include "renderer.h"

#include "vulkan_context.h"
#include "window.h"

#include <array>
#include <stdexcept>
#include <limits>

Renderer::Renderer(const VulkanContext &context, Window &window) :
    m_context(context),
    m_window(window),
    m_swapchain(context, window, vk::PresentModeKHR::eFifo)
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
    }

    // Create render finished semaphores
    m_renderFinishedSemaphores.reserve(m_swapchain.getImageCount());
    for (uint32_t i = 0; i < m_swapchain.getImageCount(); i++){
        vk::SemaphoreCreateInfo semaphoreCreateInfo = {};
        m_renderFinishedSemaphores.emplace_back(context.getDevice(), semaphoreCreateInfo);
    }
}

void Renderer::drawFrame()
{
    FrameData& frameData = m_frames[m_frameIndex];

    // Wait for in flight fence
    if(m_context.getDevice().waitForFences(*frameData.inFlightFence, vk::True, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess){
        throw std::runtime_error("Wait for in flight fence failed!");
    }

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

    // - Prepare image to be cleared
    transitionImage(commandBuffer, image, 
        vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eNone,
    vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite);

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

    vk::RenderingInfo renderingInfo = {
        .renderArea = {
            .extent = m_swapchain.getExtent()
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment
    };

    commandBuffer.beginRendering(renderingInfo);
    commandBuffer.endRendering();

    // - Convert to present mode
    transitionImage(commandBuffer, image,
        vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
        vk::PipelineStageFlagBits2::eColorAttachmentOutput, vk::AccessFlagBits2::eColorAttachmentWrite,
        vk::PipelineStageFlagBits2::eBottomOfPipe, vk::AccessFlagBits2::eNone);

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
    vk::PipelineStageFlags2 dstStage, vk::AccessFlags2 dstAccess)
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
            .aspectMask = vk::ImageAspectFlagBits::eColor, 
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

    m_swapchain = Swapchain(m_context, m_window, vk::PresentModeKHR::eFifo, *m_swapchain.getSwapchain());

    // Rebuild per-image semaphores
    m_renderFinishedSemaphores.clear();
    for (uint32_t i = 0; i < m_swapchain.getImageCount(); i++) {
        m_renderFinishedSemaphores.emplace_back(m_context.getDevice(), vk::SemaphoreCreateInfo{});
    }

    m_window.clearResized();
}
