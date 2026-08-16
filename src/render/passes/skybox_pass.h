#pragma once

#include "render/render_pass_base.h"
#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <array>

namespace Kita::Pbrv
{
    class SkyboxPass : public RenderPassBase
    {
    public:
        SkyboxPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderList& list,
            const RenderTarget& target);
        ~SkyboxPass();

        void RecreateResources() override;
        void Draw(const RenderList& list, const FrameInfo& frameInfo) const override;

    private:
        void CreateDescriptorSetLayouts();
        void AllocateDescriptorSets(const RenderList& list);
        void CreatePipeline(const RenderList& list);

    private:
        VkDescriptorSetLayout m_frameLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_frameSets{};

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
    };
}
