#pragma once

#include <array>
#include <cstdint>

struct FrameStats {
    float cpuFrameMs = 0.0f;
    float gpuSimMs = 0.0f;
    float gpuGrassMs = 0.0f;
    float gpuTotalMs = 0.0f;
    uint32_t bladeCount = 0;
    uint32_t visibleBladeCount = 0;

    std::array<float, 200> frameHistory{};
    uint32_t historyIndex = 0;
};