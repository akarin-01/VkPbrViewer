#pragma once

#include "rhi/constants.h"
#include "resource/render_texture.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }
    namespace Resource
    {
        class Resources;
    }

    namespace Render
    {
        /// Owns the lit pass output: color (MSAA) + resolve + depth attachments.
        /// Recreated on resize; the post pass reads the resolve texture through
        /// its own per-pass set.
        class RenderTarget
        {
        public:
            RenderTarget(const Rhi::Context& context,
                Resource::Resources& resources,
                VkExtent2D extent);
            ~RenderTarget();

            RenderTarget(const RenderTarget&) = delete;
            RenderTarget& operator=(const RenderTarget&) = delete;

            void Recreate(VkExtent2D extent);

            const Resource::RenderTexture& ColorTexture() const { return m_colorTex; }
            VkImageView GetColorImageView() const;
            VkFormat GetColorFormat() const;
            const Resource::RenderTexture& GetResolveTexture() const { return m_resolveTex; }
            VkImageView GetResolveImageView() const;
            VkFormat GetResolveFormat() const;
            const Resource::RenderTexture& GetDepthTexture() const { return m_depthTex; }
            VkImageView GetDepthImageView() const;
            VkFormat GetDepthFormat() const;

            void TransitionToWriteLayout(VkCommandBuffer commandBuffer);
            void TransitionToReadLayout(VkCommandBuffer commandBuffer);

        private:
            void Create(VkExtent2D extent);
            void Destroy();

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

        private:
            const Rhi::Context& m_context;
            Resource::Resources& m_resources;

            Resource::RenderTexture m_colorTex{};
            VkImageLayout m_colorLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

            Resource::RenderTexture m_resolveTex{};
            VkImageLayout m_resolveLayout{ VK_IMAGE_LAYOUT_UNDEFINED };

            Resource::RenderTexture m_depthTex{};
            VkImageLayout m_depthLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
        };
    }
}
