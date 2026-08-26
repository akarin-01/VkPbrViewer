#pragma once

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Scene;
    }

    namespace Ui
    {
        class UI
        {
        public:
            UI(Scene::Scene& scene);
            ~UI();

            void Update(float deltaTime);

            bool IsMouseHovered() const;

        private:
            void DrawPanel(float deltaTime);

        private:
            Scene::Scene& m_scene;
        };
    }
}
