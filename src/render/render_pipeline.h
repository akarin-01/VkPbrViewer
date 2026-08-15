#pragma once

#include "render/render_resource_types.h"

#include <vector>
#include <memory>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class SwapChain;
    class DescriptorAllocator;
    class RenderPassBase;

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
        std::vector<std::unique_ptr<RenderPassBase>> m_passes;
    };
}