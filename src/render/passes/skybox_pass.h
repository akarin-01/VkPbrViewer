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
        struct TargetTextures;
        struct FrameState;

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
            void CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts);

        private:
            const TargetTextures& m_target;

            const FrameState& m_frame;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
