#pragma once

#include <vulkan/vulkan_raii.hpp>
#include <glm/vec3.hpp>

class TerrainMode {
public:
    virtual ~TerrainMode() = default;

    virtual void update(const vk::raii::CommandBuffer& /*cmd*/, const glm::vec3& /*cameraPos*/){}
    virtual void draw(const vk::raii::CommandBuffer& cmd, vk::DeviceAddress cameraAddress) const = 0;
};