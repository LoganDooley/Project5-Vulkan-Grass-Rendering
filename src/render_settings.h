#pragma once

#include <cstdint>

struct RenderSettings {
    uint32_t getCullFlags() const {
        return (bOrientationCullingEnabled ? 1u : 0u) |
            (bFrustumCullingEnabled ? 2u : 0u) |
            (bDistanceCullingEnabled ? 4u : 0u);
    }
    // Culling Settings
    bool bOrientationCullingEnabled = true;
    bool bFrustumCullingEnabled = true;
    bool bDistanceCullingEnabled = true;
    float distanceCullingMaxDistance = 30.0f;

    // Simulation Settings
    bool bSimulate = true;
    float windStrength = 2.0f;
    float gustStrength = 22.0f;
    float gravity = 9.8f;

    // Grass Settings
    float grassDensity = 25.0f;
    float grassRadius = 50.0f;

    // Display Settings
    bool bUseVsync = true;
};