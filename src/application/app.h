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
    namespace Resource
    {
        class AssetManager;
    }
    namespace Render
    {
        class Renderer;
    }
    namespace Scene
    {
        class Scene;
    }

    namespace Application
    {
        class UI;

        class App
        {
        public:
            App();
            ~App();

            void Run();

        private:
            std::unique_ptr<Core::Window> m_window;
            std::unique_ptr<Core::Input> m_input;
            std::unique_ptr<Core::Time> m_time;

            std::unique_ptr<Resource::AssetManager> m_assetManager;
            std::unique_ptr<Render::Renderer> m_renderer;
            std::unique_ptr<Scene::Scene> m_scene;
            std::unique_ptr<UI> m_ui;
        };
    }
}
