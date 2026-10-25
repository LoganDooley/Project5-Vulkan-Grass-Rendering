#pragma once

#include <vulkan/vulkan_raii.hpp>

#include <cstdint>
#include <vector>

class VulkanContext;

// Enum for the timestamps to be tracked
enum class GpuTimestamp : uint32_t {
    FrameStart, SimStart, SimEnd, GrassStart, GrassEnd, FrameEnd, 
    Count
};

struct GpuTimings {
    float simMs = 0.0f;
    float grassMs = 0.0f;
    float totalMs = 0.0f;
};

class GpuProfiler {
public:
    GpuProfiler(const VulkanContext& context, uint32_t framesInFlight);

    void collect(uint32_t frameIndex);
    void beginFrame(const vk::raii::CommandBuffer& cmd, uint32_t frameIndex);
    void mark(const vk::raii::CommandBuffer& cmd, GpuTimestamp slot);
    void endFrame();

    const GpuTimings& getTimings() const { return m_timings; }

private:
    std::vector<vk::raii::QueryPool> m_pools{};
    std::vector<bool> m_submitted{};
    uint32_t m_currentFrame = 0;
    float m_msPerTick = 0.0f;
    uint64_t m_validMask = 0;
    GpuTimings m_runningSum{};
    uint32_t m_frameCounter = 0;
    GpuTimings m_timings{};
};