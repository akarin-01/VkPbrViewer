#include "orbit_camera_controller.h"

#include "core/input.h"
#include "scene/camera.h"

#include <algorithm>
#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Application
    {
        OrbitCameraController::OrbitCameraController(const Core::Input& input, Scene::Camera& camera)
            : m_input(input), m_camera(camera)
        {
        }

        void OrbitCameraController::Update(float delta)
        {
            if (m_input.IsMouseButtonDown(Core::MouseButton::Right))
            {
                glm::vec2 rotate = m_input.GetCursorDelta() * m_rotateSpeed * delta;
                m_camera.RotateYaw(-rotate.x);
                m_camera.RotatePitch(rotate.y);
            }
            else if (m_input.IsMouseButtonDown(Core::MouseButton::Middle))
            {
                glm::vec2 pan = m_input.GetCursorDelta() * m_panSpeed * delta;
                m_target += m_camera.GetRight() * (-pan.x) + m_camera.GetUp() * pan.y;
            }

            m_distance = std::clamp(m_distance - m_input.GetScrollDelta() * m_zoomSpeed,
                m_camera.GetNear(), m_camera.GetFar());

            // The camera looks along GetFront(); place it on the opposite side of the target
            m_camera.SetPosition(m_target - m_camera.GetFront() * m_distance);
        }

        OrbitCameraController& OrbitCameraController::SetRotateSpeed(float speed)
        {
            m_rotateSpeed = speed;
            return *this;
        }

        OrbitCameraController& OrbitCameraController::SetPanSpeed(float speed)
        {
            m_panSpeed = speed;
            return *this;
        }

        OrbitCameraController& OrbitCameraController::SetZoomSpeed(float speed)
        {
            m_zoomSpeed = speed;
            return *this;
        }
    }
}
