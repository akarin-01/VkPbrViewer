#include "camera.h"

#include "core/math.h"
#include "render/scene_proxy.h"

#include <algorithm>
#include <cmath>

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

        void Camera::Update() const
        {
            Render::SceneProxy::Get().WriteCameraData(
                m_position, GetFront(), GetUp(), m_fov, m_near, m_far);
        }

        Camera& Camera::SetPosition(const glm::vec3& position)
        {
            m_position = position;
            return *this;
        }

        Camera& Camera::Move(const glm::vec3& local)
        {
            m_position += GetRight() * local.x + GetUp() * local.y + GetFront() * local.z;
            return *this;
        }

        Camera& Camera::SetYaw(float yaw)
        {
            m_yaw = Core::WrapDegrees(yaw);
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

        Camera& Camera::SetView(float fov, float near, float far)
        {
            m_fov = std::clamp(fov, 30.0f, 90.0f);
            m_near = near;
            m_far = far;
            return *this;
        }

        glm::vec3 Camera::GetFront() const
        {
            // yaw = pitch = 0 looks down -Z
            float x = -cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));
            float y = -sin(glm::radians(m_pitch));
            float z = -cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
            return { x, y, z };
        }

        glm::vec3 Camera::GetRight() const
        {
            return glm::normalize(glm::cross(GetFront(), kWorldUp));
        }

        glm::vec3 Camera::GetUp() const
        {
            return glm::normalize(glm::cross(GetRight(), GetFront()));
        }
    }
}
