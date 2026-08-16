#pragma once

#include "render/render_pass_base.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class FrameData;
    class SkyboxEnvironment;

    class SkyboxPass : public RenderPassBase
    {
    public:
        SkyboxPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const FrameData& frameData,
            const SkyboxEnvironment& skybox,
            const RenderTarget& target);
        ~SkyboxPass();

        void RecreateResources() override;
        void Draw(const FrameInfo& frameInfo) const override;

    private:
        void CreatePipeline();

    private:
        const FrameData& m_frameData;
        const SkyboxEnvironment& m_skybox;

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };
    };
}
