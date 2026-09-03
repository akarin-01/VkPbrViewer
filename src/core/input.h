#pragma once

#include <glm/glm.hpp>
#include <memory>
#include <array>

namespace Kita::Pbrv
{
    namespace Core
    {
        enum class Key
        {
            W, S, A, D, Q, E,
            Escape,
            Count,
        };

        enum class MouseButton
        {
            Left,
            Middle,
            Right,
            Count,
        };

        enum class CursorMode
        {
            Normal,
            Disabled,
            Count,
        };

        class Window;
        class InputBackend;

        class Input
        {
        public:
            explicit Input(Window& window);
            ~Input();

            void Update();

            bool IsKeyDown(Key key) const { return m_keyStates[static_cast<size_t>(key)]; }
            bool IsKeyPressed(Key key) const { return m_keyPressedThisFrame[static_cast<size_t>(key)]; }
            bool IsKeyReleased(Key key) const { return m_keyReleasedThisFrame[static_cast<size_t>(key)]; }

            bool IsMouseButtonDown(MouseButton button) const { return m_mouseButtonStates[static_cast<size_t>(button)]; }
            bool IsMouseButtonPressed(MouseButton button) const { return m_mouseButtonPressedThisFrame[static_cast<size_t>(button)]; }
            bool IsMouseButtonReleased(MouseButton button) const { return m_mouseButtonReleasedThisFrame[static_cast<size_t>(button)]; }

            void SetCursorMode(CursorMode mode);

            /// Current cursor position in window coordinates (pixels, origin top-left, y-down).
            glm::vec2 GetCursorPosition() const { return m_cursorPos; }
            /// Cursor movement since the last frame, measured in pixels per frame.
            glm::vec2 GetCursorDelta() const { return m_cursorDelta; }
            /// Scroll wheel offset accumulated since the last frame (positive = scroll up).
            float GetScrollDelta() const { return m_scrollDelta; }

        private:
            void UpdateKeyStates();
            void UpdateMouseButtonStates();

        private:
            std::unique_ptr<InputBackend> m_backend;

            std::array<bool, static_cast<size_t>(Key::Count)> m_keyStates{};
            std::array<bool, static_cast<size_t>(Key::Count)> m_keyPressedThisFrame{};
            std::array<bool, static_cast<size_t>(Key::Count)> m_keyReleasedThisFrame{};

            std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtonStates{};
            std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtonPressedThisFrame{};
            std::array<bool, static_cast<size_t>(MouseButton::Count)> m_mouseButtonReleasedThisFrame{};

            glm::vec2 m_cursorPos{ 0.0f };
            glm::vec2 m_cursorDelta{ 0.0f };
            float m_scrollDelta{ 0.0f };
        };
    }
}
