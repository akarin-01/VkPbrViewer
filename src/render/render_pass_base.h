#pragma once

#include "rhi/frame_info.h"
#include "resource/types.h"

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
        class SwapChain;
    }
    namespace Resource
    {
        class Resources;
    }

    namespace Render
    {
        class RenderPassBase
        {
        public:
            RenderPassBase(const Rhi::Context& context,
                Resource::Resources& resources,
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
            const Rhi::Context& m_context;
            Resource::Resources& m_resources;
            const Rhi::SwapChain& m_swapChain;
        };
    }
}
