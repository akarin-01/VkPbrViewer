#pragma once

#include "render/render_resource_types.h"

namespace Kita::Pbrv
{
    struct RenderTarget
    {
    };

    class RenderContext;
    class RenderResources;
    class SwapChain;
    class DescriptorAllocator;

    class RenderPassBase
    {
    public:
        RenderPassBase(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator)
            : m_context(context),
            m_resources(resources),
            m_swapChain(swapChain),
            m_descriptorAllocator(descriptorAllocator)
        {
        }
        virtual ~RenderPassBase() = default;

        virtual void RecreateResources() = 0;
        virtual void Draw(const RenderList& list, const FrameInfo& frameInfo) const = 0;

    protected:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const SwapChain& m_swapChain;
        const DescriptorAllocator& m_descriptorAllocator;
    };
}