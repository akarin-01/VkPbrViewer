#pragma once

#include "render/render_pass_base.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderFrameData;
    class RenderMaterialData;
    class RenderMeshData;

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

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
    };
}
