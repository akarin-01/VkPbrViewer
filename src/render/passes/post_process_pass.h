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
        class RenderPostProcessData;

        class PostProcessPass : public RenderPassBase
        {
        public:
            PostProcessPass(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            ~PostProcessPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline(VkDescriptorSetLayout emptyLayout);

        private:
            const RenderTarget& m_target;
            const RenderFrameData& m_frameData;
            const RenderPostProcessData& m_postProcessData;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
