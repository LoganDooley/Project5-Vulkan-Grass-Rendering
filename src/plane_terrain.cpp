#include "plane_terrain.h"

#include "vulkan_context.h"
#include "gpu_types.h"
#include "shader_utils.h"
#include "pipeline_utils.h"
#include "render_settings.h"

static constexpr vk::ShaderStageFlags stageFlags = vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment;

PlaneTerrain::PlaneTerrain(const VulkanContext &context, vk::Format colorFormat, vk::Format depthFormat) :
    m_context(context)
{
    createPipelineLayout();
    createPipeline(colorFormat, depthFormat);
}

void PlaneTerrain::draw(const vk::raii::CommandBuffer &cmd, vk::DeviceAddress cameraAddress) const
{
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
    
    GroundPushConstants pc = {
        .camera = cameraAddress
    };

    cmd.pushConstants<GroundPushConstants>(*m_pipelineLayout, stageFlags, 0, pc);
    
    cmd.drawMeshTasksEXT(1, 1, 1);
}

void PlaneTerrain::createPipelineLayout()
{
    vk::PushConstantRange pcRange = {
        .stageFlags = stageFlags,
        .offset = 0,
        .size = sizeof(GroundPushConstants)
    };
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pcRange
    };
    m_pipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);
}

void PlaneTerrain::createPipeline(vk::Format colorFormat, vk::Format depthFormat)
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

    m_pipeline = PipelineUtils::createMeshPipeline(device, stages, m_pipelineLayout, colorFormat, depthFormat);
}
