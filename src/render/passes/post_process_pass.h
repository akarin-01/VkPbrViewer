#pragma once

#include "render/render_pass_base.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
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

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
    };
}
