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
        /// Orbit camera controller: rotates around a target, pans the target
        /// and zooms the distance. Writes the final camera position each frame
        class OrbitCameraController
        {
        public:
            explicit OrbitCameraController(const Core::Input& input, Scene::Camera& camera);

            void Update(float delta);

            OrbitCameraController& SetRotateSpeed(float speed);
            OrbitCameraController& SetPanSpeed(float speed);
            OrbitCameraController& SetZoomSpeed(float speed);

        private:
            void Rotate();
            void Pan();
            void Zoom();

        private:
            const Core::Input& m_input;
            Scene::Camera& m_camera;

            glm::vec3 m_target{ 0.0f };
            float m_distance{ 3.0f };

            float m_rotateSpeed{ 0.08f };
            float m_panSpeed{ 0.001f };
            float m_zoomSpeed{ 0.1f };
        };
    }
}
