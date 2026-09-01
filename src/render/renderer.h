#pragma once

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
        class FrameSync;
        class SwapChain;
    }
    namespace Resource
    {
        class AssetManager;
        class ResourceManager;
    }

    namespace Render
    {
        class RenderPipeline;
        class RenderScene;
        class Renderer
        {
        public:
            Renderer(Core::Window& window, const Resource::AssetManager& assetMgr);
            ~Renderer();

            void NewFrame() const;
            void DrawFrame();

            const Resource::ResourceManager& GetResourceManager() const { return *m_resourceMgr; }
            const RenderScene& GetRenderScene() const { return *m_renderScene; }

        private:
            std::unique_ptr<Rhi::Context> m_context;
            std::unique_ptr<Rhi::SwapChain> m_swapChain;
            std::unique_ptr<Rhi::FrameSync> m_frameSync;

            std::unique_ptr<Resource::ResourceManager> m_resourceMgr;

            std::unique_ptr<RenderScene> m_renderScene;
            std::unique_ptr<RenderPipeline> m_pipeline;
        };
    }
}
