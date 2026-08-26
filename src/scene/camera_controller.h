#pragma once

namespace Kita::Pbrv
{
    namespace Core
    {
        class Input;
    }

    namespace Scene
    {
    class Camera;

    class CameraController
    {
    public:
        explicit CameraController(const Core::Input& input, Camera& camera);

        void Update(float delta);

        CameraController& SetRotateSpeed(float speed);
        CameraController& SetPanSpeed(float speed);
        CameraController& SetZoomSpeed(float speed);

    private:
        const Core::Input& m_input;
        Camera& m_camera;

        float m_rotateSpeed{ 240.0f };
        float m_panSpeed{ 1.8f };
        float m_zoomSpeed{ 0.08f };
    };
    }
}
