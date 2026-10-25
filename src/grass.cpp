#include "grass.h"

#include "vulkan_context.h"
#include "pipeline_utils.h"
#include "shader_utils.h"
#include "gpu_types.h"
#include "render_settings.h"

#include <cmath>

Grass::Grass(const VulkanContext &context, 
    float density, float radius,
    vk::Format colorFormat, vk::Format depthFormat) :
    m_context(context)
{
    resize(density, radius);

    // Create pipeline layouts
    createInitPipelineLayout();
    createDrawPipelineLayout();
    createSimPipelineLayout();

    // Create pipelines
    createInitPipeline();
    createDrawPipeline(colorFormat, depthFormat);
    createSimPipeline();
}

void Grass::resize(float density, float radius)
{
    m_density = density;
    m_radius = radius;
    m_gridWidth = std::max(1u, (uint32_t)std::lround(2.0f * radius * std::sqrt(density)));
    m_bladeCount = m_gridWidth * m_gridWidth;

    m_blades = Buffer(m_context.getAllocator(), m_context.getDevice(), m_bladeCount * sizeof(Blade), vk::BufferUsageFlagBits::eStorageBuffer, false);
}

void Grass::recordInit(const vk::raii::CommandBuffer &cmd) const
{
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *m_initPipeline);

    GrassInitPushConstants pc = { 
        .blades = m_blades.getDeviceAddress(),
        .bladeCount = m_bladeCount,
        .gridWidth = m_gridWidth,
        .areaSize = 2 * m_radius,
        .seed = 1234
    };
    cmd.pushConstants<GrassInitPushConstants>(*m_initPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pc);
    cmd.dispatch((m_bladeCount + 63) / 64, 1, 1);

    // Add barrier
    vk::MemoryBarrier2 writeBeforeRead = {
        .srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
        .dstStageMask  = vk::PipelineStageFlagBits2::eAllCommands,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{ 
        .memoryBarrierCount = 1, 
        .pMemoryBarriers = &writeBeforeRead 
    });
}

void Grass::simulate(const vk::raii::CommandBuffer &cmd, float dt, float totalTime, const RenderSettings& renderSettings) const
{
    // Add WAR barrier
    vk::MemoryBarrier2 readBeforeWrite = {
        .srcStageMask  = vk::PipelineStageFlagBits2::eTaskShaderEXT | vk::PipelineStageFlagBits2::eMeshShaderEXT,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
        .dstStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead | vk::AccessFlagBits2::eShaderStorageWrite,
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{ 
        .memoryBarrierCount = 1, 
        .pMemoryBarriers = &readBeforeWrite 
    });

    // Dispatch
    cmd.bindPipeline(vk::PipelineBindPoint::eCompute, *m_simPipeline);
    GrassSimPushConstants pc = { 
        .blades = m_blades.getDeviceAddress(),
        .bladeCount = m_bladeCount,
        .deltaTime = dt,
        .totalTime = totalTime,
        .windStrength = renderSettings.windStrength,
        .gustStrength = renderSettings.gustStrength,
        .gravity = renderSettings.gravity
    };
    cmd.pushConstants<GrassSimPushConstants>(*m_simPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, pc);
    cmd.dispatch((m_bladeCount + 63) / 64, 1, 1);

    // Add RAW barrier
    vk::MemoryBarrier2 writeBeforeRead = {
        .srcStageMask  = vk::PipelineStageFlagBits2::eComputeShader,
        .srcAccessMask = vk::AccessFlagBits2::eShaderStorageWrite,
        .dstStageMask  = vk::PipelineStageFlagBits2::eTaskShaderEXT | vk::PipelineStageFlagBits2::eMeshShaderEXT,
        .dstAccessMask = vk::AccessFlagBits2::eShaderStorageRead,
    };
    cmd.pipelineBarrier2(vk::DependencyInfo{ 
        .memoryBarrierCount = 1, 
        .pMemoryBarriers = &writeBeforeRead 
    });
}

void Grass::draw(const vk::raii::CommandBuffer &cmd, vk::DeviceAddress cameraAddress, const RenderSettings& renderSettings) const
{
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_drawPipeline);
    GrassDrawPushConstants pc = {
        .camera = cameraAddress,
        .blades = m_blades.getDeviceAddress(),
        .bladeCount = m_bladeCount,
        .cullFlags = renderSettings.getCullFlags(),
        .distanceCullingMaxDistance = renderSettings.distanceCullingMaxDistance
    };
    cmd.pushConstants<GrassDrawPushConstants>(*m_drawPipelineLayout,
        vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
        0, pc);

    cmd.drawMeshTasksEXT((m_bladeCount + 31) / 32, 1, 1);
}

void Grass::createInitPipelineLayout()
{
    vk::PushConstantRange range = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(GrassInitPushConstants)
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range
    };
    m_initPipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);
}

void Grass::createDrawPipelineLayout()
{
    vk::PushConstantRange range = {
        .stageFlags = vk::ShaderStageFlagBits::eTaskEXT | vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment,
        .offset = 0,
        .size = sizeof(GrassDrawPushConstants)
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range
    };
    m_drawPipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);
}

void Grass::createSimPipelineLayout()
{
    vk::PushConstantRange range = {
        .stageFlags = vk::ShaderStageFlagBits::eCompute,
        .offset = 0,
        .size = sizeof(GrassSimPushConstants)
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &range
    };
    m_simPipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);

}

void Grass::createInitPipeline()
{
    vk::raii::ShaderModule module = ShaderUtils::createShaderModule(m_context.getDevice(), "shaders/grass_init.comp.spv");
    m_initPipeline = PipelineUtils::createComputePipeline(m_context.getDevice(), module, m_initPipelineLayout);
}

void Grass::createDrawPipeline(vk::Format colorFormat, vk::Format depthFormat)
{
    vk::raii::ShaderModule taskModule = ShaderUtils::createShaderModule(m_context.getDevice(), "shaders/grass.task.spv");
    vk::raii::ShaderModule meshModule = ShaderUtils::createShaderModule(m_context.getDevice(), "shaders/grass.mesh.spv");
    vk::raii::ShaderModule fragModule = ShaderUtils::createShaderModule(m_context.getDevice(), "shaders/grass.frag.spv");

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

    m_drawPipeline = PipelineUtils::createMeshPipeline(m_context.getDevice(), stages, m_drawPipelineLayout, colorFormat, depthFormat);
}

void Grass::createSimPipeline()
{
    vk::raii::ShaderModule module = ShaderUtils::createShaderModule(m_context.getDevice(), "shaders/grass_sim.comp.spv");
    m_simPipeline = PipelineUtils::createComputePipeline(m_context.getDevice(), module, m_simPipelineLayout);
}
