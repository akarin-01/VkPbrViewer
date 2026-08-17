#pragma once

#include "render/render_resource_types.h"

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class SwapChain;

    class RenderPassBase
    {
    public:
        RenderPassBase(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain)
            : m_context(context),
            m_resources(resources),
            m_swapChain(swapChain)
        {
        }
        virtual ~RenderPassBase() = default;

        virtual void RecreateResources() = 0;
        virtual void Draw(const FrameInfo& frameInfo) const = 0;

    protected:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const SwapChain& m_swapChain;
    };
}