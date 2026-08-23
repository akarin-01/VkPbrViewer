#pragma once

#include "render/render_pass_base.h"

#include <memory>

namespace Kita::Pbrv
{
    class RenderFrameData;
    class RenderMaterialData;
    class RenderMeshData;
    class RenderIblData;
    class GraphicsPipeline;
    class RenderTargetData;

    class LitPass : public RenderPassBase
    {
    public:
        LitPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            RenderTargetData& targetData,
            const RenderFrameData& frameData,
            const RenderMaterialData& materialData,
            const RenderMeshData& meshData,
            const RenderIblData& iblData);
        ~LitPass();

        void RecreateResources() override;
        void Draw(const FrameInfo& frameInfo) const override;

    private:
        void CreatePipeline();

    private:
        RenderTargetData& m_targetData;
        const RenderFrameData& m_frameData;
        const RenderMaterialData& m_materialData;
        const RenderIblData& m_iblData;
        const RenderMeshData& m_meshData;

        std::unique_ptr<GraphicsPipeline> m_pipeline;
    };
}
