#pragma once

#include "render/render_pass_base.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class FrameData;
    class MaterialCache;
    class MeshCache;

    class LitPass : public RenderPassBase
    {
    public:
        LitPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const FrameData& frameData,
            const MaterialCache& materialCache,
            const MeshCache& meshCache,
            const RenderTarget& target);
        ~LitPass();

        void RecreateResources() override;
        void Draw(const FrameInfo& frameInfo) const override;

    private:
        void CreatePipeline();

    private:
        const FrameData& m_frameData;
        const MaterialCache& m_materialCache;
        const MeshCache& m_meshCache;

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
    };
}
