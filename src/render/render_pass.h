#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>
#include <array>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class SwapChain;
    class DescriptorAllocator;

    class RenderPass
    {
    public:
        RenderPass(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderList& list);
        ~RenderPass();

        void RecreateResources();
        void Draw(const RenderList& list, const FrameInfo& frameInfo) const;

    private:
        void CreateDescriptorSetLayouts();
        void AllocateDescriptorSets(const RenderList& list);
        void CreatePipeline(const RenderList& list);
        void CreateDepthImage();
        void DestroyDepthImage();

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const SwapChain& m_swapChain;
        const DescriptorAllocator& m_descriptorAllocator;

        VkDescriptorSetLayout m_frameLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_frameSets{};

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };

        RenderImageHandle m_depthImageHandle;
        RenderImageViewHandle m_depthImageViewHandle;
    };
}