#pragma once

#include "render/render_resource_types.h"
#include "render/render_pass_base.h"

#include <vector>
#include <memory>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class SwapChain;
    class DescriptorAllocator;
    class RenderPipeline
    {
    public:
        RenderPipeline(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderList& list);
        ~RenderPipeline();

        void RecreateResources();
        void Draw(const RenderList& list, const FrameInfo& frameInfo) const;

    private:
        void CreateRenderTarget();
        void DestroyRenderTarget();
        void RecreateRenderTarget();
        void CreateRenderPasses(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderList& list);
        void DestroyRenderPasses();

    private:
        RenderResources& m_resources;
        const SwapChain& m_swapChain;

        std::vector<std::unique_ptr<RenderPassBase>> m_passes;
        RenderTarget m_target{};
    };
}