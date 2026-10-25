#include "gpu_profiler.h"

#include "vulkan_context.h"

#include <stdexcept>

static constexpr uint32_t COUNT = static_cast<uint32_t>(GpuTimestamp::Count);
static uint32_t idx(GpuTimestamp t) {
    return static_cast<uint32_t>(t);
}

GpuProfiler::GpuProfiler(const VulkanContext &context, uint32_t framesInFlight)
{
    const vk::raii::PhysicalDevice& physicalDevice = context.getPhysicalDevice();
    float periodNs = physicalDevice.getProperties().limits.timestampPeriod;
    uint32_t validBits = physicalDevice.getQueueFamilyProperties()[context.getQueueFamilyIndex()].timestampValidBits;

    if(validBits == 0){
        throw std::runtime_error("Vulkan queue does not support timestamps!");
    }
    m_msPerTick = periodNs * 1e-6f;
    m_validMask = (validBits >= 60) ? ~0ull : ((1ull << validBits) - 1);

    m_pools.reserve(framesInFlight);
    vk::QueryPoolCreateInfo poolCreateInfo = {
        .queryType = vk::QueryType::eTimestamp,
        .queryCount = COUNT,
    };
    for(uint32_t i = 0; i < framesInFlight; i++){
        m_pools.emplace_back(context.getDevice(), poolCreateInfo);
    }
    m_submitted.assign(framesInFlight, false);
}

void GpuProfiler::collect(uint32_t frameIndex)
{
    if(!m_submitted[frameIndex]){
        // Nothing to read
        return;
    }

    // Retrieve timestamps
    auto [result, ticks] = m_pools[frameIndex].getResults<uint64_t>(
        0, COUNT, COUNT * sizeof(uint64_t), sizeof(uint64_t),
        vk::QueryResultFlagBits::e64);
    if(result != vk::Result::eSuccess){
        return;
    }

    auto ms = [&](GpuTimestamp a, GpuTimestamp b){
        return static_cast<float>((ticks[idx(b)] - ticks[idx(a)]) & m_validMask) * m_msPerTick;
    };

    m_runningSum.simMs += ms(GpuTimestamp::SimStart, GpuTimestamp::SimEnd);
    m_runningSum.grassMs += ms(GpuTimestamp::GrassStart, GpuTimestamp::GrassEnd);
    m_runningSum.totalMs += ms(GpuTimestamp::FrameStart, GpuTimestamp::FrameEnd);
    m_frameCounter++;

    if(m_frameCounter >= 60){
        // Update benchmark every 60 frames
        m_timings.simMs = m_runningSum.simMs / static_cast<float>(m_frameCounter);
        m_timings.grassMs = m_runningSum.grassMs / static_cast<float>(m_frameCounter);
        m_timings.totalMs = m_runningSum.totalMs / static_cast<float>(m_frameCounter);

        m_runningSum = {};
        m_frameCounter = 0;
    }
}

void GpuProfiler::beginFrame(const vk::raii::CommandBuffer &cmd, uint32_t frameIndex)
{
    m_currentFrame = frameIndex;
    cmd.resetQueryPool(*m_pools[m_currentFrame], 0, COUNT);
    mark(cmd, GpuTimestamp::FrameStart);
}

void GpuProfiler::mark(const vk::raii::CommandBuffer &cmd, GpuTimestamp slot)
{
    cmd.writeTimestamp2(vk::PipelineStageFlagBits2::eAllCommands, *m_pools[m_currentFrame], idx(slot));
}

void GpuProfiler::endFrame()
{
    m_submitted[m_currentFrame] = true;
}
