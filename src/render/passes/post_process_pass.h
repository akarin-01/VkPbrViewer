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
        class RenderPostProcessData;
        class PostProcessPass : public RenderPassBase
        {
        public:
            PostProcessPass(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                RenderTargetData& targetData,
                const RenderScene& scene);
            ~PostProcessPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline();

        private:
            RenderTargetData& m_targetData;
            const RenderPostProcessData& m_postProcessData;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
