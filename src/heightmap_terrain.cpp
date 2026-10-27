#include "heightmap_terrain.h"

#include <glm/common.hpp>

#include "vulkan_context.h"
#include "gpu_types.h"
#include "shader_utils.h"
#include "pipeline_utils.h"
#include "render_settings.h"

static constexpr vk::ShaderStageFlags stageFlags = vk::ShaderStageFlagBits::eMeshEXT | vk::ShaderStageFlagBits::eFragment;
static constexpr float PATCH_SIZE = 8.0f;
static constexpr int PATCHES_PER_SIDE = 64;

HeightmapTerrain::HeightmapTerrain(const VulkanContext &context, vk::Format colorFormat, vk::Format depthFormat) :
    m_context(context)
{
    createPipelineLayout();
    createPipeline(colorFormat, depthFormat);
}

void HeightmapTerrain::update(const vk::raii::CommandBuffer &/*cmd*/, const glm::vec3 &cameraPos)
{
    glm::vec2 cameraXZ(cameraPos.x, cameraPos.z);
    m_center = glm::floor(cameraXZ / PATCH_SIZE) * PATCH_SIZE;
}

void HeightmapTerrain::draw(const vk::raii::CommandBuffer &cmd, vk::DeviceAddress cameraAddress) const
{
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_pipeline);
    
    float half = 0.5 * PATCH_SIZE * PATCHES_PER_SIDE;
    HeightmapPushConstants pc = {
        .camera = cameraAddress,
        .origin = m_center - glm::vec2(half),
        .patchSize = PATCH_SIZE,
        .patchesPerSide = PATCHES_PER_SIDE
    };

    cmd.pushConstants<HeightmapPushConstants>(*m_pipelineLayout, stageFlags, 0, pc);
    
    cmd.drawMeshTasksEXT(PATCHES_PER_SIDE, PATCHES_PER_SIDE, 1);
}

void HeightmapTerrain::createPipelineLayout()
{
    vk::PushConstantRange pcRange = {
        .stageFlags = stageFlags,
        .offset = 0,
        .size = sizeof(HeightmapPushConstants)
    };
    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pcRange
    };
    m_pipelineLayout = vk::raii::PipelineLayout(m_context.getDevice(), pipelineLayoutCreateInfo);
}

void HeightmapTerrain::createPipeline(vk::Format colorFormat, vk::Format depthFormat)
{
    const vk::raii::Device& device = m_context.getDevice();
    
    // Create shader modules
    vk::raii::ShaderModule meshModule = ShaderUtils::createShaderModule(device, "shaders/heightmap.mesh.spv");
    vk::raii::ShaderModule fragModule = ShaderUtils::createShaderModule(device, "shaders/heightmap.frag.spv");
    
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
