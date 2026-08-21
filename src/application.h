#pragma once

#include <memory>

namespace Kita::Pbrv
{
    class Window;
    class Input;
    class Time;
    class Scene;
    class UI;
    class Renderer;

    class Application
    {
    public:
        Application();
        ~Application();

        void Run();

    private:
        std::unique_ptr<Window> m_window;
        std::unique_ptr<Input> m_input;
        std::unique_ptr<Time> m_time;
        std::unique_ptr<Scene> m_scene;
        std::unique_ptr<UI> m_ui;
        std::unique_ptr<Renderer> m_renderer;
    };
}
