#pragma once

#include "render/render_pass_base.h"

#include <memory>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class GraphicsPipeline;
    }

    namespace Render
    {
        class RenderScene;
        class RenderTarget;
        class RenderFrameData;

        class SkyboxPass : public RenderPassBase
        {
        public:
            SkyboxPass(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            ~SkyboxPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline();

        private:
            const RenderTarget& m_target;
            const RenderFrameData& m_frameData;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
