#pragma once

#include "render/render_resource_types.h"

#include <memory>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class Window;
    class RenderContext;
    class SwapChain;
    class FrameSync;
    class RenderResources;
    class DescriptorAllocator;

    class RenderScene;
    class RenderPipeline;
    class Scene;

    class Renderer
    {
    public:
        Renderer(Window& window);
        ~Renderer();

        void DrawFrame(const Scene& scene);

    private:
        static std::unique_ptr<DescriptorAllocator> CreateDescriptorAllocator(const RenderContext& context);

    private:
        std::unique_ptr<RenderContext> m_context;
        std::unique_ptr<SwapChain> m_swapChain;
        std::unique_ptr<FrameSync> m_frameSync;
        std::unique_ptr<RenderResources> m_resources;

        std::unique_ptr<DescriptorAllocator> m_descriptorAllocator;

        std::unique_ptr<RenderScene> m_renderScene;
        std::unique_ptr<RenderPipeline> m_pipeline;
    };
}