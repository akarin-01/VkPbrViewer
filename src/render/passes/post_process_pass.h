#pragma once

#include "render/render_pass_base.h"

#include <memory>

namespace Kita::Pbrv
{
    class RenderTargetData;
    class GraphicsPipeline;

    class PostProcessPass : public RenderPassBase
    {
    public:
        PostProcessPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            RenderTargetData& targetData);
        ~PostProcessPass();

        void RecreateResources() override;
        void Draw(const FrameInfo& frameInfo) const override;

    private:
        void CreatePipeline();

    private:
        RenderTargetData& m_targetData;

        std::unique_ptr<GraphicsPipeline> m_pipeline;
    };
}
