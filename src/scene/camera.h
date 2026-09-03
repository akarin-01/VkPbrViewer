#pragma once

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Scene
    {
        /// Pure view state: position, yaw/pitch and projection parameters.
        /// Axes are derived on demand; matrices are assembled in the SceneProxy
        class Camera
        {
        public:
            Camera();
            ~Camera();

            /// Writes position + derived axes + projection params into the
            /// SceneProxy, which assembles the view matrix immediately
            void Update() const;

            Camera& SetPosition(const glm::vec3& position);
            /// Moves camera by local offset along its right/up/front axes.
            Camera& MoveLocal(const glm::vec3& offset);
            /// Moves camera by world-space offset (absolute X/Y/Z).
            Camera& MoveWorld(const glm::vec3& offset);

            Camera& SetYaw(float yaw);
            Camera& RotateYaw(float delta);

            Camera& SetPitch(float pitch);
            Camera& RotatePitch(float delta);

            Camera& SetView(float fov, float near, float far);

            glm::vec3 GetPosition() const { return m_position; }
            float GetYaw() const { return m_yaw; }
            float GetPitch() const { return m_pitch; }
            float GetFov() const { return m_fov; }
            float GetNear() const { return m_near; }
            float GetFar() const { return m_far; }

            glm::vec3 GetFront() const;
            glm::vec3 GetRight() const;
            glm::vec3 GetUp() const;

        private:
            glm::vec3 m_position{ 0.0f, 0.0f, 3.0f };
            float m_yaw{ 0.0f };
            float m_pitch{ 0.0f };

            float m_fov{ 45.0f };
            float m_near{ 0.01f };
            float m_far{ 100.0f };
        };
    }
}
