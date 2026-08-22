#pragma once

#include "render/render_resource_types.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;

    class RenderTargetData
    {
    public:
        RenderTargetData(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator,
            VkExtent2D extent);
        ~RenderTargetData();

        void Recreate(VkExtent2D extent);

        const RenderTexture& GetColorTexture() const { return m_colorTex; }
        VkImageView GetColorImageView() const;
        VkFormat GetColorFormat() const;
        const RenderTexture& GetDepthTexture() const { return m_depthTex; }
        VkImageView GetDepthImageView() const;
        VkFormat GetDepthFormat() const;

        void TransitionColorImageLayout(VkCommandBuffer commandBuffer,
            VkImageLayout newLayout,
            VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
            VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);

        void TransitionDepthImageLayout(VkCommandBuffer commandBuffer,
            VkImageLayout newLayout,
            VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
            VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);

        VkDescriptorSetLayout GetSetLayout() const { return m_setLayout; }
        const VkDescriptorSet& GetSet() const { return m_set; }

    private:
        void Create(VkExtent2D extent);
        void Destroy();

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        RenderTexture m_colorTex{};
        VkImageLayout m_colorLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

        RenderTexture m_depthTex{};
        VkImageLayout m_depthLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        VkDescriptorSet m_set{ VK_NULL_HANDLE };
    };
}
