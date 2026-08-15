#pragma once

#include "render/render_pass_base.h"
#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <array>

namespace Kita::Pbrv
{
    class PostProcessPass : public RenderPassBase
    {
    public:
        PostProcessPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderList& list,
            const RenderTarget& target);
        ~PostProcessPass();

        void RecreateResources() override;
        void Draw(const RenderList& list, const FrameInfo& frameInfo) const override;

    private:
        void CreateDescriptorSetLayouts();
        void AllocateDescriptorSets(const RenderList& list);
        void UpdateInputSet(VkDescriptorSet set) const;
        void CreatePipeline(const RenderList& list);

    private:
        VkDescriptorSetLayout m_inputLayout{ VK_NULL_HANDLE };
        VkDescriptorSet m_inputSet{ VK_NULL_HANDLE };

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
    };
}
