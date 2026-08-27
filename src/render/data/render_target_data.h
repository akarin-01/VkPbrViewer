#pragma once

#include "resource/render_texture.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class RenderContext;
    }
    namespace Resource
    {
        class RenderResources;
        class DescriptorManager;
    }

    namespace Render
    {
        class RenderTargetData
        {
        public:
            RenderTargetData(const Rhi::RenderContext& context,
                Resource::RenderResources& resources,
                Resource::DescriptorManager& descriptorMgr,
                VkExtent2D extent);
            ~RenderTargetData();

            void Recreate(VkExtent2D extent);

            const Resource::RenderTexture& GetColorTexture() const { return m_colorTex; }
            VkImageView GetColorImageView() const;
            VkFormat GetColorFormat() const;
            const Resource::RenderTexture& GetResolveTexture() const { return m_resolveTex; }
            VkImageView GetResolveImageView() const;
            VkFormat GetResolveFormat() const;
            const Resource::RenderTexture& GetDepthTexture() const { return m_depthTex; }
            VkImageView GetDepthImageView() const;
            VkFormat GetDepthFormat() const;

            void TransitionColorImageLayout(VkCommandBuffer commandBuffer,
                VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);

            void TransitionResolveImageLayout(VkCommandBuffer commandBuffer,
                VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);

            void TransitionDepthImageLayout(VkCommandBuffer commandBuffer,
                VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet() const { return m_set; }

        private:
            void Create(VkExtent2D extent);
            void Destroy();

        private:
            const Rhi::RenderContext& m_context;
            Resource::RenderResources& m_resources;
            Resource::DescriptorManager& m_descriptorMgr;

            Resource::RenderTexture m_colorTex{};
            VkImageLayout m_colorLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

            Resource::RenderTexture m_resolveTex{};
            VkImageLayout m_resolveLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

            Resource::RenderTexture m_depthTex{};
            VkImageLayout m_depthLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

            VkDescriptorSet m_set{ VK_NULL_HANDLE };
        };
    }
}
