#include "pipeline_utils.h"

vk::raii::Pipeline PipelineUtils::createMeshPipeline(const vk::raii::Device &device, 
    const std::vector<vk::PipelineShaderStageCreateInfo> &stages, 
    vk::PipelineLayout layout, 
    vk::Format colorFormat, 
    vk::Format depthFormat)
{
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

    // Depth state
    vk::PipelineDepthStencilStateCreateInfo depthStencilState = {
        .depthTestEnable = vk::True,
        .depthWriteEnable = vk::True,
        .depthCompareOp = vk::CompareOp::eGreater
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
    vk::PipelineRenderingCreateInfo renderingCreateInfo = {
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
        .depthAttachmentFormat = depthFormat
    };

    // Create the pipeline
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo = {
        .pNext = &renderingCreateInfo,
        .stageCount = static_cast<uint32_t>(stages.size()),
        .pStages = stages.data(),
        .pViewportState = &viewportState,
        .pRasterizationState = &rasterizationState,
        .pMultisampleState = &multisampleState,
        .pDepthStencilState = &depthStencilState,
        .pColorBlendState = &colorBlendState,
        .pDynamicState = &dynamicState,
        .layout = layout
    };

    return vk::raii::Pipeline(device, nullptr, pipelineCreateInfo);
}

vk::raii::Pipeline PipelineUtils::createComputePipeline(const vk::raii::Device &device, 
    const vk::raii::ShaderModule &module, 
    vk::PipelineLayout layout)
{
    vk::ComputePipelineCreateInfo pipelineCreateInfo = {
        .stage = {
            .stage = vk::ShaderStageFlagBits::eCompute,
            .module = *module,
            .pName = "computeMain"
        },
        .layout = layout
    };

    return vk::raii::Pipeline(device, nullptr, pipelineCreateInfo);
}
