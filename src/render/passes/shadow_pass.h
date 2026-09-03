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
        struct ShadowTextures;
        struct FrameState;
        struct RenderObject;

        class ShadowPass : public RenderPassBase
        {
        public:
            ShadowPass(const Rhi::Context& context,
                const Rhi::SwapChain& swapChain,
                const RenderScene& scene);
            ~ShadowPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts);

        private:
            const ShadowTextures& m_shadow;

            const FrameState& m_frame;
            const std::vector<RenderObject>& m_objects;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
