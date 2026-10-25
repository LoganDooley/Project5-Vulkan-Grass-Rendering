#pragma once

#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>

struct CameraData {
    glm::mat4 view;
    glm::mat4 proj;
    glm::mat4 viewProj;
    glm::vec4 position;
};
static_assert(sizeof(CameraData) == 208, "CameraData must match the size in Slang");

struct GroundPushConstants {
    vk::DeviceAddress camera; // Gpu address of camera buffer
};

struct Blade {
    glm::vec4 v0;
    glm::vec4 v1;
    glm::vec4 v2;
    glm::vec4 up;
};
static_assert(sizeof(Blade) == 64, "Blade must match the Slang struct");

struct GrassInitPushConstants {
    vk::DeviceAddress blades;
    uint32_t bladeCount;
    uint32_t gridWidth;
    float areaSize;
    uint32_t seed;
};

struct GrassDrawPushConstants {
    vk::DeviceAddress camera;
    vk::DeviceAddress cullCamera;
    vk::DeviceAddress blades;
    vk::DeviceAddress visibleCounter;
    uint32_t bladeCount;
    uint32_t cullFlags;
    float distanceCullingMaxDistance;
};

struct GrassSimPushConstants {
    vk::DeviceAddress blades;
    uint32_t bladeCount;
    float deltaTime;
    float totalTime;
    float windStrength;
    float gustStrength;
    float gravity;
};