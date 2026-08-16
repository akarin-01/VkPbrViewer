#pragma once

#include "render/render_pass_base.h"

#include <memory>

namespace Kita::Pbrv
{
    class RenderFrameData;
    class RenderMaterialData;
    class RenderMeshData;
    class GraphicsPipeline;

    class LitPass : public RenderPassBase
    {
    public:
        LitPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderFrameData& frameData,
            const RenderMaterialData& materialCache,
            const RenderMeshData& meshCache,
            const RenderTarget& target);
        ~LitPass();

        void RecreateResources() override;
        void Draw(const FrameInfo& frameInfo) const override;

    private:
        void CreatePipeline();

    private:
        const RenderFrameData& m_frameData;
        const RenderMaterialData& m_materialData;
        const RenderMeshData& m_meshData;

        std::unique_ptr<GraphicsPipeline> m_pipeline;
    };
}
