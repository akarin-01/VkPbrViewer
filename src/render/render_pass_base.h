#pragma once

#include "rhi/frame_info.h"

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
        class SwapChain;
    }

    namespace Render
    {
        class RenderPassBase
        {
        public:
            RenderPassBase(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain)
                : m_context(context),
                m_swapChain(swapChain)
            {
            }
            virtual ~RenderPassBase() = default;

            virtual void RecreateResources() = 0;
            virtual void Draw(const Rhi::FrameInfo& frameInfo) const = 0;

        protected:
            const Rhi::Context& m_context;
            const Rhi::SwapChain& m_swapChain;
        };
    }
}
