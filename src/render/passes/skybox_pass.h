#pragma once

#include "render/render_pass_base.h"

#include <memory>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class GraphicsPipeline;
    }
    namespace Resource
    {
        struct PerFrameSet;
        struct TargetResource;
    }

    namespace Render
    {
        class RenderScene;

        class SkyboxPass : public RenderPassBase
        {
        public:
            SkyboxPass(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            ~SkyboxPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline();

        private:
            const Resource::TargetResource& m_target;
            const Resource::PerFrameSet& m_frameSet;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
