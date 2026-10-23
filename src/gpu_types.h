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

struct PushConstants {
    vk::DeviceAddress camera; // Gpu address of camera buffer
};