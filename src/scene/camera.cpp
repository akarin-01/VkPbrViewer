#include "camera.h"

#include <algorithm>

namespace Kita::Pbrv
{
    namespace Scene
    {
        namespace
        {
            constexpr glm::vec3 kWorldUp{ 0.0f, 1.0f, 0.0f };
        }

        Camera::Camera() = default;

        Camera::~Camera() = default;

        Camera& Camera::SetTarget(const glm::vec3& target)
        {
            m_target = target;
            return *this;
        }

        Camera& Camera::Pan(float offsetX, float offsetY)
        {
            glm::vec3 front = -GetOrbitDirection();
            glm::vec3 right = glm::normalize(glm::cross(front, kWorldUp));
            glm::vec3 up = glm::normalize(glm::cross(right, front));

            return SetTarget(m_target + right * offsetX + up * offsetY);
        }

        Camera& Camera::SetYaw(float yaw)
        {
            m_yaw = yaw;
            return *this;
        }

        Camera& Camera::RotateYaw(float delta)
        {
            return SetYaw(m_yaw + delta);
        }

        Camera& Camera::SetPitch(float pitch)
        {
            m_pitch = std::clamp(pitch, -89.0f, 89.0f);
            return *this;
        }

        Camera& Camera::RotatePitch(float delta)
        {
            return SetPitch(m_pitch + delta);
        }

        Camera& Camera::SetDistance(float distance)
        {
            m_distance = std::clamp(distance, m_near, m_far);
            return *this;
        }

        Camera& Camera::Zoom(float delta)
        {
            return SetDistance(m_distance - delta);
        }

        Camera& Camera::SetView(float fov, float near, float far)
        {
            m_fov = fov;
            m_near = near;
            m_far = far;
            return *this;
        }

        glm::vec3 Camera::GetPosition() const
        {
            return m_target + GetOrbitDirection() * m_distance;
        }

        glm::mat4 Camera::GetViewMatrix() const
        {
            return glm::lookAt(GetPosition(), m_target, kWorldUp);
        }

        glm::mat4 Camera::GetProjectMatrix(float aspect) const
        {
            glm::mat4 proj = glm::perspective(glm::radians(m_fov), aspect, m_near, m_far);
            proj[1][1] *= -1.0f;        // Reverse y axis
            return proj;
        }

        glm::vec3 Camera::GetOrbitDirection() const
        {
            /*
             * +x -> right
             * +y -> up
             * +z -> back
            */
            float x = cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));
            float y = sin(glm::radians(m_pitch));
            float z = cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
            return { x, y, z };
        }
    }
}
