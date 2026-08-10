#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class SwapChain;

    class RenderPass
    {
    public:
        RenderPass(const RenderContext& context, RenderResources& resources, const SwapChain& swapChain);
        ~RenderPass();

        void Initialize(const RenderList& list);
        void RecreateResources();
        void Draw(const RenderList& list, const FrameInfo& frameInfo);

    private:
        void CreateDescriptorPool();
        void CreateDescriptorSetLayouts();
        void AllocateDescriptorSets(const RenderList& list);
        void CreatePipeline();
        void CreateDepthImage();
        void DestroyDepthImage();
        VkShaderModule CreateShaderModule(const std::string& filePath) const;
        std::vector<char> ReadFile(const std::string& path) const;

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const SwapChain& m_swapChain;

        VkDescriptorPool m_descriptorPool{ VK_NULL_HANDLE };
        VkDescriptorSetLayout m_frameLayout{ VK_NULL_HANDLE };
        std::vector<VkDescriptorSet> m_frameSets;

        VkPipeline m_pipeline{ VK_NULL_HANDLE };
        VkPipelineLayout m_pipelineLayout{ VK_NULL_HANDLE };

        RenderImageHandle m_depthImageHandle;
        VkImageView m_depthImageView{ VK_NULL_HANDLE };
    };
}