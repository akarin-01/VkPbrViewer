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
        class RenderMeshData;
        class RenderIblData;
        class LitPass : public RenderPassBase
        {
        public:
            LitPass(const Rhi::RenderContext& context,
                Resource::RenderResources& resources,
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
            const RenderFrameData& m_frameData;
            const RenderMaterialData& m_materialData;
            const RenderIblData& m_iblData;
            const RenderMeshData& m_meshData;

            std::unique_ptr<Rhi::GraphicsPipeline> m_pipeline;
        };
    }
}
