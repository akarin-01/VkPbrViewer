#pragma once

#include "render/render_pass_base.h"
#include "resource/resource_id.h"

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
        struct LitState;
        struct RenderObject;

        class LitPass : public RenderPassBase
        {
        public:
            LitPass(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            ~LitPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts);

        private:
            const TargetTextures& m_target;

            const FrameState& m_frame;
            const LitState& m_lit;
            const std::vector<RenderObject>& m_objects;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
