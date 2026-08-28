#pragma once

#include "resource/types.h"

#include <memory>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Window;
    }
    namespace Rhi
    {
        class Context;
        class SwapChain;
        class FrameSync;
    }
    namespace Resource
    {
        class Resources;
        class DescriptorManager;
        class AssetManager;
        class ResourceManager;
    }

    namespace Scene
    {
        class Scene;
    }

    namespace Render
    {
        class RenderScene;
        class RenderPipeline;
        class Renderer
        {
        public:
            Renderer(Core::Window& window, const Resource::AssetManager& assetMgr);
            ~Renderer();

            void NewFrame() const;
            void DrawFrame(const Scene::Scene& scene);

        private:
            std::unique_ptr<Rhi::Context> m_context;
            std::unique_ptr<Resource::Resources> m_resources;
            std::unique_ptr<Rhi::SwapChain> m_swapChain;
            std::unique_ptr<Rhi::FrameSync> m_frameSync;

            std::unique_ptr<Resource::DescriptorManager> m_descriptorMgr;
            std::unique_ptr<Resource::ResourceManager> m_resourceMgr;

            std::unique_ptr<RenderScene> m_renderScene;
            std::unique_ptr<RenderPipeline> m_pipeline;
        };
    }
}
