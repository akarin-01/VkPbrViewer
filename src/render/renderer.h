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
        class RenderContext;
        class SwapChain;
        class FrameSync;
    }
    namespace Resource
    {
        class RenderResources;
        class DescriptorAllocator;
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
            Renderer(Core::Window& window);
            ~Renderer();

            void NewFrame() const;
            void DrawFrame(const Scene::Scene& scene);

        private:
            std::unique_ptr<Rhi::RenderContext> m_context;
            std::unique_ptr<Rhi::SwapChain> m_swapChain;
            std::unique_ptr<Rhi::FrameSync> m_frameSync;
            std::unique_ptr<Resource::RenderResources> m_resources;

            std::unique_ptr<Resource::DescriptorAllocator> m_descriptorAllocator;

            std::unique_ptr<RenderScene> m_renderScene;
            std::unique_ptr<RenderPipeline> m_pipeline;
        };
    }
}
