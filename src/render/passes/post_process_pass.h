#pragma once

#include "render/render_pass_base.h"

#include <memory>

namespace Kita::Pbrv
{
    class GraphicsPipeline;

    class PostProcessPass : public RenderPassBase
    {
    public:
        PostProcessPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderTarget& target);
        ~PostProcessPass();

        void RecreateResources() override;
        void Draw(const FrameInfo& frameInfo) const override;

    private:
        void CreateDescriptorSetLayouts();
        void AllocateDescriptorSets();
        void UpdateInputSet(VkDescriptorSet set) const;
        void CreatePipeline();

    private:
        VkDescriptorSetLayout m_inputLayout{ VK_NULL_HANDLE };
        VkDescriptorSet m_inputSet{ VK_NULL_HANDLE };

        std::unique_ptr<GraphicsPipeline> m_pipeline;
    };
}
