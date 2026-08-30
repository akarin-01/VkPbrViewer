#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        struct BufferDesc;
        struct BufferRhi;
        struct ImageDesc;
        struct ImageRhi;
        struct ImageViewDesc;
        struct ImageViewRhi;
        struct SamplerDesc;
        struct SamplerRhi;

        namespace ResourceUtils
        {
            BufferRhi CreateBufferRhi(const Rhi::Context& context,
                BufferDesc desc, const void* data = nullptr, size_t size = 0);

            ImageRhi CreateImageRhi(const Rhi::Context& context,
                ImageDesc desc, const void* data = nullptr, size_t size = 0);

            ImageViewRhi CreateImageViewRhi(const Rhi::Context& context,
                const ImageRhi& image, const ImageViewDesc& desc);

            SamplerRhi CreateSamplerRhi(const Rhi::Context& context,
                const SamplerDesc& desc);

            uint32_t CalculateMipLevels(uint32_t width, uint32_t height);

            /// Full-image layout transition (all mips/layers, aspect from the image).
            void TransitionImageLayout(VkCommandBuffer commandBuffer, const ImageRhi& image,
                VkImageLayout oldLayout, VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask);

            /// Layout transition restricted to `range` (e.g. per-mip in mip generation).
            void TransitionImageLayout(VkCommandBuffer commandBuffer, const ImageRhi& image,
                VkImageLayout oldLayout, VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
                const VkImageSubresourceRange& range);

            /// Blit the mip chain. Pre: every subresource in TRANSFER_DST.
            /// Post: every subresource in `finalLayout`, ordered for `finalStageMask`.
            void GenerateImageMipmaps(VkCommandBuffer commandBuffer, const ImageRhi& image,
                VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VkPipelineStageFlags2 finalStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);
        };
    }
}
