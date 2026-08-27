#pragma once

#include "render/render_pass_base.h"

namespace Kita::Pbrv
{
    namespace Core
    {
        class Window;
    }

    namespace Render
    {
        class UIPass : public RenderPassBase
        {
        public:
            UIPass(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                const Core::Window& window);
            ~UIPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;
        };
    }
}
