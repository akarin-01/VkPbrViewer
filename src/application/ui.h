#pragma once

namespace Kita::Pbrv
{
    namespace Resource
    {
        class AssetManager;
    }
    namespace Scene
    {
        class Scene;
    }

    namespace Application
    {
        class UI
        {
        public:
            UI(Scene::Scene& scene, Resource::AssetManager& assets);
            ~UI();

            void Update(float deltaTime);

            bool IsMouseHovered() const;

        private:
            void DrawPanel(float deltaTime);

        private:
            Scene::Scene& m_scene;
            Resource::AssetManager& m_assets;
        };
    }
}