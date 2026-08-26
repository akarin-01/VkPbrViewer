#pragma once

#include "rhi/frame_info.h"
#include "resource/types.h"

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class RenderContext;
        class SwapChain;
    }
    namespace Resource
    {
        class RenderResources;
    }

    namespace Render
    {
        class RenderPassBase
        {
        public:
            RenderPassBase(const Rhi::RenderContext& context,
                Resource::RenderResources& resources,
                const Rhi::SwapChain& swapChain)
                : m_context(context),
                m_resources(resources),
                m_swapChain(swapChain)
            {
            }
            virtual ~RenderPassBase() = default;

            virtual void RecreateResources() = 0;
            virtual void Draw(const Rhi::FrameInfo& frameInfo) const = 0;

        protected:
            const Rhi::RenderContext& m_context;
            Resource::RenderResources& m_resources;
            const Rhi::SwapChain& m_swapChain;
        };
    }
}
