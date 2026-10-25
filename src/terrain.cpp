#include "terrain.h"

#include "vulkan_context.h"
#include "render_settings.h"
#include "gpu_types.h"
#include "shader_utils.h"
#include "pipeline_utils.h"

static constexpr vk::ShaderStageFlags stageFlags = vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment;
    

Terrain::Terrain(const VulkanContext &context, vk::Format colorFormat, vk::Format depthFormat) :
    m_context(context)
{
    createPlanePipelineLayout();
    createPlanePipline(colorFormat, depthFormat);
}

void Terrain::draw(const vk::raii::CommandBuffer &cmd, vk::DeviceAddress cameraAddress, const RenderSettings &settings) const
{
    switch(settings.terrainMode){
        case TerrainMode::Plane:
            drawPlane(cmd, cameraAddress);
            break;
        case TerrainMode::Heightmap:
            break;
        case TerrainMode::Voxel:
            break;
        default:
            break;
    }
}

void Terrain::createPlanePipelineLayout()
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
    m_planePipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);
}

void Terrain::createPlanePipline(vk::Format colorFormat, vk::Format depthFormat)
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

    m_planePipeline = PipelineUtils::createMeshPipeline(device, stages, m_planePipelineLayout, colorFormat, depthFormat);
}

void Terrain::drawPlane(const vk::raii::CommandBuffer &cmd, vk::DeviceAddress cameraAddress) const
{
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_planePipeline);
    
    GroundPushConstants pc = {
        .camera = cameraAddress
    };

    cmd.pushConstants<GroundPushConstants>(*m_planePipelineLayout, stageFlags, 0, pc);
    
    cmd.drawMeshTasksEXT(1, 1, 1);
}
