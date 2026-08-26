#include "camera_controller.h"
#include "core/input.h"
#include "scene/camera.h"

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Scene
    {
        CameraController::CameraController(const Core::Input& input, Camera& camera)
            : m_input(input), m_camera(camera)
        {
        }

        void CameraController::Update(float delta)
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
                m_camera.Pan(-pan.x, pan.y);
            }

            float zoom = m_input.GetScrollDelta();
            m_camera.Zoom(zoom * m_zoomSpeed);
        }

        CameraController& CameraController::SetRotateSpeed(float speed)
        {
            m_rotateSpeed = speed;
            return *this;
        }

        CameraController& CameraController::SetPanSpeed(float speed)
        {
            m_panSpeed = speed;
            return *this;
        }

        CameraController& CameraController::SetZoomSpeed(float speed)
        {
            m_zoomSpeed = speed;
            return *this;
        }
    }
}
