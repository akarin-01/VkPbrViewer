#pragma once

#include "render/render_resource_types.h"
#include "render/data/render_target_data.h"

#include <vector>
#include <memory>

namespace Kita::Pbrv
{
    class Window;
    class RenderContext;
    class RenderResources;
    class SwapChain;
    class RenderScene;
    class RenderPassBase;
    class RenderTargetData;

    class RenderPipeline
    {
    public:
        RenderPipeline(const Window& window,
            const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderScene& scene);
        ~RenderPipeline();

        void RecreateResources();
        void Draw(const FrameInfo& frameInfo) const;

    private:
        void CreateRenderPasses(const Window& window,
            const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const RenderScene& scene);
        void DestroyRenderPasses();

    private:
        const SwapChain& m_swapChain;

        std::vector<std::unique_ptr<RenderPassBase>> m_passes;
        RenderTargetData m_targetData;
    };
}
