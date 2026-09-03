#pragma once

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Input;
    }
    namespace Scene
    {
        class Camera;
    }

    namespace Application
    {
        /// FPS-style camera controller: WASD moves horizontally in local space,
        /// Q/E moves vertically in world space, and right mouse button enables look.
        class FpsCameraController
        {
        public:
            FpsCameraController(const Core::Input& input, Scene::Camera& camera);

            void Update(float delta);

            FpsCameraController& SetRotateSpeed(float speed);
            FpsCameraController& SetMoveSpeed(float speed);

        private:
            void Rotate();
            void Move(float delta);

        private:
            const Core::Input& m_input;
            Scene::Camera& m_camera;

            float m_rotateSpeed{ 0.08f };
            float m_moveSpeed{ 5.0f };
        };
    }
}
