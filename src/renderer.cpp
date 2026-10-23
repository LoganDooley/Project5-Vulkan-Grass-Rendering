#include "renderer.h"

#include "vulkan_context.h"
#include "window.h"
#include "shader_utils.h"
#include "camera.h"

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

    // Create triangle pipeline
    createTrianglePipeline();
}

void Renderer::drawFrame(const Camera& camera, float totalTime)
{
    FrameData& frameData = m_frames[m_frameIndex];

    // Wait for in flight fence
    if(m_context.getDevice().waitForFences(*frameData.inFlightFence, vk::True, std::numeric_limits<uint64_t>::max()) != vk::Result::eSuccess){
        throw std::runtime_error("Wait for in flight fence failed!");
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
            .extent = extent
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachment
    };

    commandBuffer.beginRendering(renderingInfo);

    // Run triangle pipeline
    commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_trianglePipeline);
    // - Add push constants
    PushConstants pc = {
        .camera = frameData.cameraBuffer.getDeviceAddress()
    };
    commandBuffer.pushConstants<PushConstants>(*m_trianglePipelineLayout, m_pcStages, 0, pc);
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
    commandBuffer.drawMeshTasksEXT(1, 1, 1);

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

void Renderer::createTrianglePipeline()
{
    std::vector<uint32_t> code = ShaderLoader::readSpirv("shaders/triangle.spv");
    vk::ShaderModuleCreateInfo moduleInfo = {
        .codeSize = code.size() * sizeof(uint32_t),
        .pCode = code.data()
    };
    vk::raii::ShaderModule shaderModule(m_context.getDevice(), moduleInfo);

    // Create layout
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
    m_trianglePipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);

    std::array<vk::PipelineShaderStageCreateInfo, 3> stages = {
        {
            {
                .stage = vk::ShaderStageFlagBits::eTaskEXT,
                .module = *shaderModule,
                .pName = "taskMain"
            },
            {
                .stage = vk::ShaderStageFlagBits::eMeshEXT,
                .module = *shaderModule,
                .pName = "meshMain"
            },
            {
                .stage = vk::ShaderStageFlagBits::eFragment,
                .module = *shaderModule,
                .pName = "fragMain"
            }
        },
    };

    // Viewport
    vk::PipelineViewportStateCreateInfo viewportState = {
        .viewportCount = 1,
        .scissorCount = 1
    };

    // Rasterization
    vk::PipelineRasterizationStateCreateInfo rasterizationState = {
        .polygonMode = vk::PolygonMode::eFill,
        .cullMode = vk::CullModeFlagBits::eNone,
        .frontFace = vk::FrontFace::eCounterClockwise,
        .lineWidth = 1.0f
    };

    // Multisampling
    vk::PipelineMultisampleStateCreateInfo multisampleState = {
        .rasterizationSamples = vk::SampleCountFlagBits::e1
    };

    // Color blending
    vk::PipelineColorBlendAttachmentState blendAttachment = {
        .blendEnable = vk::False,
        .colorWriteMask = vk::ColorComponentFlagBits::eR | 
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA,
    };

    vk::PipelineColorBlendStateCreateInfo colorBlendState = { 
        .attachmentCount = 1, 
        .pAttachments = &blendAttachment 
    };

    // Dynamic state
    std::array<vk::DynamicState, 2> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    vk::PipelineDynamicStateCreateInfo dynamicState = {
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    // Dynamic rendering
    vk::Format colorFormat = m_swapchain.getFormat();
    vk::PipelineRenderingCreateInfo renderingCreateInfo = {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat
    };

    // Create the pipeline
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo = {
        .pNext = &renderingCreateInfo,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = *m_trianglePipelineLayout
    };

    m_trianglePipeline = vk::raii::Pipeline(m_context.getDevice(), nullptr, pipelineCreateInfo);
}
