#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "gpu_types.h"

struct CameraInput {
    glm::vec3 move = glm::vec3(0.0f);
    glm::vec2 look = glm::vec2(0.0f);
};

class Camera {
public:
    Camera() = default;
    Camera(glm::vec3 position);

    void update(float dt, const CameraInput& input);
    CameraData getCameraData(float aspect) const;

    glm::vec3 getPosition() const { return m_position; }
    float getYaw() const { return m_yaw; }
    float getPitch() const { return m_pitch; }

private:
    glm::vec3 getForwardVector() const;

    glm::vec3 m_position = glm::vec3(0.0);
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_fovy = 1.0f;
    float m_near = 0.01f;
    float m_far = 100.f;
    float m_moveSpeed = 1.0f;
    float m_lookSpeed = 0.003f;

    const glm::vec3 m_worldUp = glm::vec3(0.0, 1.0, 0.0);
};