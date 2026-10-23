#include "camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <cmath>

Camera::Camera(glm::vec3 position) :
    m_position(position)
{
}

void Camera::update(float dt, const CameraInput &input)
{
    // Rotate
    m_yaw += input.look.x * m_lookSpeed;
    m_pitch -= input.look.y * m_lookSpeed;
    m_pitch = glm::clamp(m_pitch, -1.5f, 1.5f);

    // Move
    glm::vec3 forward = getForwardVector();
    glm::vec3 right = glm::normalize(glm::cross(forward, m_worldUp));

    m_position += (forward * input.move.z + right * input.move.x + m_worldUp * input.move.y) * m_moveSpeed * dt;
}

CameraData Camera::getCameraData(float aspect) const
{
    CameraData cameraData{};
    cameraData.view = glm::lookAt(m_position, m_position + getForwardVector(), m_worldUp);
    cameraData.proj = glm::perspective(m_fovy, aspect, m_far, m_near);
    cameraData.proj[1][1] *= -1.0f;  
    cameraData.viewProj = cameraData.proj * cameraData.view;
    cameraData.position = glm::vec4(m_position, 1.0);

    return cameraData;
}

glm::vec3 Camera::getForwardVector() const
{
    return glm::vec3(std::cos(m_pitch) * std::sin(m_yaw),
                    std::sin(m_pitch),
                    -std::cos(m_pitch) * std::cos(m_yaw));
}
