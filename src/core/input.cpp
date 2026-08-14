#include "input.h"

#include "core/window.h"

#include <GLFW/glfw3.h>

namespace Kita::Pbrv
{
    namespace
    {
        constexpr int kKeyMap[static_cast<size_t>(Key::Count)] =
        {
            GLFW_KEY_W, GLFW_KEY_S, GLFW_KEY_A, GLFW_KEY_D,
            GLFW_KEY_ESCAPE
        };

        constexpr int kMouseButtonMap[static_cast<size_t>(MouseButton::Count)] =
        {
            GLFW_MOUSE_BUTTON_LEFT,
            GLFW_MOUSE_BUTTON_MIDDLE,
            GLFW_MOUSE_BUTTON_RIGHT,
        };
    }

    class InputBackend
    {
    public:
        explicit InputBackend(Window& window)
            : m_window(static_cast<GLFWwindow*>(window.GetNativeHandle()))
        {
            window.SetScrollCallback(
                [this](double /*x*/, double y)
                {
                    m_scrollDelta += static_cast<float>(y);
                });
        }

        bool IsKeyDown(Key key) const
        {
            return glfwGetKey(m_window, kKeyMap[static_cast<size_t>(key)]) == GLFW_PRESS;
        }

        bool IsMouseButtonDown(MouseButton button) const
        {
            return glfwGetMouseButton(m_window, kMouseButtonMap[static_cast<size_t>(button)]) == GLFW_PRESS;
        }

        glm::vec2 GetCursorPosition() const
        {
            double x, y;
            glfwGetCursorPos(m_window, &x, &y);
            return { static_cast<float>(x), static_cast<float>(y) };
        }

        float ConsumeScrollDelta()
        {
            float ret = m_scrollDelta;
            m_scrollDelta = 0.0f;
            return ret;
        }

    private:
        GLFWwindow* m_window{ nullptr };

        float m_scrollDelta{ 0.0f };
    };

    Input::Input(Window& window)
    {
        m_backend = std::make_unique<InputBackend>(window);

        // Init key and mouse button states
        m_cursorPos = m_backend->GetCursorPosition();
    }

    Input::~Input() = default;

    void Input::Update()
    {
        UpdateKeyStates();
        UpdateMouseButtonStates();

        glm::vec2 cursorPos = m_backend->GetCursorPosition();
        m_cursorDelta = cursorPos - m_cursorPos;
        m_cursorPos = cursorPos;

        m_scrollDelta = m_backend->ConsumeScrollDelta();
    }

    void Input::UpdateKeyStates()
    {
        for (size_t i = 0; i < m_keyStates.size(); ++i)
        {
            Key key = static_cast<Key>(i);

            bool oldDown = m_keyStates[i];
            bool newDown = m_backend->IsKeyDown(key);
            m_keyStates[i] = newDown;

            if (oldDown && !newDown)
            {
                // Just release
                m_keyPressedThisFrame[i] = false;
                m_keyReleasedThisFrame[i] = true;
            }
            else if (!oldDown && newDown)
            {
                // Just press
                m_keyPressedThisFrame[i] = true;
                m_keyReleasedThisFrame[i] = false;
            }
            else
            {
                // Clear
                m_keyPressedThisFrame[i] = false;
                m_keyReleasedThisFrame[i] = false;
            }
        }
    }

    void Input::UpdateMouseButtonStates()
    {
        for (size_t i = 0; i < m_mouseButtonStates.size(); ++i)
        {
            MouseButton button = static_cast<MouseButton>(i);

            bool oldDown = m_mouseButtonStates[i];
            bool newDown = m_backend->IsMouseButtonDown(button);
            m_mouseButtonStates[i] = newDown;

            if (oldDown && !newDown)
            {
                // Just release
                m_mouseButtonPressedThisFrame[i] = false;
                m_mouseButtonReleasedThisFrame[i] = true;
            }
            else if (!oldDown && newDown)
            {
                // Just press
                m_mouseButtonPressedThisFrame[i] = true;
                m_mouseButtonReleasedThisFrame[i] = false;
            }
            else
            {
                // Clear
                m_mouseButtonPressedThisFrame[i] = false;
                m_mouseButtonReleasedThisFrame[i] = false;
            }
        }
    }
}