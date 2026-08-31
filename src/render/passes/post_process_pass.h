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
        struct PostProcessSet;
    }

    namespace Render
    {
        class RenderScene;

        class PostProcessPass : public RenderPassBase
        {
        public:
            PostProcessPass(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            ~PostProcessPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline(VkDescriptorSetLayout emptyLayout);

        private:
            const Resource::PostProcessSet& m_postProcessSet;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
