#include "fps_camera_controller.h"

#include "core/input.h"
#include "scene/camera.h"

#include <cmath>
#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Application
    {
        FpsCameraController::FpsCameraController(const Core::Input& input, Scene::Camera& camera)
            : m_input(input), m_camera(camera)
        {
        }

        void FpsCameraController::Update(float delta)
        {
            Rotate();
            Move(delta);
        }

        FpsCameraController& FpsCameraController::SetRotateSpeed(float speed)
        {
            m_rotateSpeed = speed;
            return *this;
        }

        FpsCameraController& FpsCameraController::SetMoveSpeed(float speed)
        {
            m_moveSpeed = speed;
            return *this;
        }

        void FpsCameraController::Rotate()
        {
            if (m_input.IsMouseButtonDown(Core::MouseButton::Right))
            {
                glm::vec2 rotate = m_input.GetCursorDelta() * m_rotateSpeed;
                m_camera.RotateYaw(-rotate.x);
                m_camera.RotatePitch(rotate.y);
            }
        }

        void FpsCameraController::Move(float delta)
        {
            glm::vec3 inputH(0.0f);
            float inputV = 0.0f;

            if (m_input.IsKeyDown(Core::Key::A))
            {
                inputH.x -= 1.0f;
            }
            if (m_input.IsKeyDown(Core::Key::D))
            {
                inputH.x += 1.0f;
            }
            if (m_input.IsKeyDown(Core::Key::W))
            {
                inputH.z += 1.0f;
            }
            if (m_input.IsKeyDown(Core::Key::S))
            {
                inputH.z -= 1.0f;
            }
            if (m_input.IsKeyDown(Core::Key::Q))
            {
                inputV -= 1.0f;
            }
            if (m_input.IsKeyDown(Core::Key::E))
            {
                inputV += 1.0f;
            }

            if (glm::length(inputH) > 0.001f)
            {
                glm::vec3 moveH = glm::normalize(inputH) * m_moveSpeed * delta;
                m_camera.MoveLocal(moveH);
            }

            if (std::abs(inputV) > 0.001f)
            {
                glm::vec3 moveV = glm::normalize(glm::vec3(0.0f, inputV, 0.0f)) * m_moveSpeed * delta;
                m_camera.MoveWorld(moveV);
            }
        }
    }
}
