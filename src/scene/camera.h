#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Kita::Pbrv
{
    class Camera
    {
    public:
        Camera();
        ~Camera();

        Camera& SetTarget(const glm::vec3& target);
        Camera& Pan(float offsetX, float offsetY);
        Camera& SetYaw(float yaw);
        Camera& RotateYaw(float delta);
        Camera& SetPitch(float pitch);
        Camera& RotatePitch(float delta);
        Camera& SetDistance(float distance);
        Camera& Zoom(float delta);

        Camera& SetView(float fov, float near, float far);

        glm::vec3 GetPosition() const;
        glm::mat4 GetViewMatrix() const;
        glm::mat4 GetProjectMatrix(float aspect) const;

    private:
        /// Calculate the vec from target to camera 
        glm::vec3 GetOrbitDirection() const;

    private:
        glm::vec3 m_target{ 0.0f };
        float m_distance{ 3.0f };
        float m_yaw{ 0.0f };
        float m_pitch{ 0.0f };

        float m_fov{ 45.0f };
        float m_near{ 0.01f };
        float m_far{ 100.0f };
    };
}