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
        class RenderTargetData;
        class RenderScene;
        class RenderFrameData;
        class RenderSkyboxData;
        class SkyboxPass : public RenderPassBase
        {
        public:
            SkyboxPass(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                RenderTargetData& targetData,
                const RenderScene& scene);
            ~SkyboxPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline();

        private:
            RenderTargetData& m_targetData;
            const RenderFrameData& m_frameData;
            const RenderSkyboxData& m_skybox;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
