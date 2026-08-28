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
        class RenderMaterialData;
        class RenderIblData;
        struct ObjectData;

        class LitPass : public RenderPassBase
        {
        public:
            LitPass(const Rhi::Context& context,
                Resource::Resources& resources,
                const Rhi::SwapChain& swapChain,
                RenderTargetData& targetData,
                const RenderScene& scene);
            ~LitPass();

            void RecreateResources() override;
            void Draw(const Rhi::FrameInfo& frameInfo) const override;

        private:
            void CreatePipeline();

        private:
            RenderTargetData& m_targetData;
            const ObjectData& m_object;
            const RenderFrameData& m_frameData;
            const RenderMaterialData& m_materialData;
            const RenderIblData& m_iblData;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
