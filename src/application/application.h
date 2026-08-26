#pragma once

#include <memory>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Window;
        class Input;
        class Time;
    }
    namespace Scene
    {
        class Scene;
    }
    namespace Ui
    {
        class UI;
    }
    namespace Render
    {
        class Renderer;
    }

    namespace Application
    {
        class Application
        {
        public:
            Application();
            ~Application();

            void Run();

        private:
            std::unique_ptr<Core::Window> m_window;
            std::unique_ptr<Core::Input> m_input;
            std::unique_ptr<Core::Time> m_time;
            std::unique_ptr<Scene::Scene> m_scene;
            std::unique_ptr<Ui::UI> m_ui;
            std::unique_ptr<Render::Renderer> m_renderer;
        };
    }
}
