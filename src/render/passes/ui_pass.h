#pragma once

#include "render/render_pass_base.h"

namespace Kita::Pbrv
{
    class Window;

    class UIPass : public RenderPassBase
    {
    public:
        UIPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const Window& window);
        ~UIPass();

        void RecreateResources() override;
        void Draw(const FrameInfo& frameInfo) const override;
    };
}
