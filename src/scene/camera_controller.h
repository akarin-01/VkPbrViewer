#pragma once

namespace Kita::Pbrv
{
    class Input;
    class Camera;

    class CameraController
    {
    public:
        explicit CameraController(const Input& input, Camera& camera);

        void Update(float delta);

        CameraController& SetRotateSpeed(float speed);
        CameraController& SetPanSpeed(float speed);
        CameraController& SetZoomSpeed(float speed);

    private:
        const Input& m_input;
        Camera& m_camera;

        float m_rotateSpeed{ 200.0f };
        float m_panSpeed{ 1.5f };
        float m_zoomSpeed{ 0.1f };
    };
}