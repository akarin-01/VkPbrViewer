#pragma once

#include "render/render_pass_base.h"

#include <memory>
#include <vector>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class GraphicsPipeline;
    }

    namespace Render
    {
        class RenderScene;
        struct PostProcessState;

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
            void CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts);

        private:
            const PostProcessState& m_postProcess;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
