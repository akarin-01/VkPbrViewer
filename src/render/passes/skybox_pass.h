#pragma once

#include "render/render_pass_base.h"

#include <memory>

namespace Kita::Pbrv
{
    class RenderFrameData;
    class RenderSkyboxData;
    class GraphicsPipeline;

    class SkyboxPass : public RenderPassBase
    {
    public:
        SkyboxPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderFrameData& frameData,
            const RenderSkyboxData& skybox,
            const RenderTarget& target);
        ~SkyboxPass();

        void RecreateResources() override;
        void Draw(const FrameInfo& frameInfo) const override;

    private:
        void CreatePipeline();

    private:
        const RenderFrameData& m_frameData;
        const RenderSkyboxData& m_skybox;

        std::unique_ptr<GraphicsPipeline> m_pipeline;
    };
}
