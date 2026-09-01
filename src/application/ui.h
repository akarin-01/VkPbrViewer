#pragma once

namespace Kita::Pbrv
{
    namespace Resource
    {
        class AssetManager;
        class ResourceManager;
    }
    namespace Render
    {
        class RenderScene;
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
            UI(Scene::Scene& scene, Resource::AssetManager& assets,
                const Resource::ResourceManager& resources,
                const Render::RenderScene& renderScene);
            ~UI();

            void Update(float deltaTime);

            bool IsMouseHovered() const;

        private:
            void DrawPanel(float deltaTime);

        private:
            Scene::Scene& m_scene;
            Resource::AssetManager& m_assets;
            const Resource::ResourceManager& m_resources;
            const Render::RenderScene& m_renderScene;
        };
    }
}