#pragma once

#include "resource/types.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Resource
    {
        class Resources;

        struct RenderTexture
        {
            bool IsEmpty() const
            {
                return m_imageHandle == 0;
            }

            bool operator==(const RenderTexture& other) const
            {
                return (m_imageHandle == other.m_imageHandle)
                    && (m_imageViewHandle == other.m_imageViewHandle)
                    && (m_samplerHandle == other.m_samplerHandle);
            }

            bool operator!=(const RenderTexture& other) const
            {
                return !(*this == other);
            }

            RenderImageHandle m_imageHandle{ 0 };
            RenderImageViewHandle m_imageViewHandle{ 0 };
            RenderSamplerHandle m_samplerHandle{ 0 };
        };

        RenderTexture CreateCubemapFallback(Resources& resources, VkFormat format);

        /// Creates an empty 2D texture in UNDEFINED layout (e.g. a storage write
        /// target for conversion passes).
        RenderTexture Create2DTexture(Resources& resources,
            uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels,
            VkImageUsageFlags usage, RenderSamplerHandle sampler);

        /// Creates a 2D texture (upload + mip chain); the result is left in SHADER_READ_ONLY_OPTIMAL
        /// (mipLevels = 1 performs only the final layout transition).
        RenderTexture Create2DTextureWithData(Resources& resources,
            const void* data, size_t size,
            uint32_t width, uint32_t height, VkFormat format, uint32_t mipLevels,
            RenderSamplerHandle sampler);

        /// Creates an empty cubemap in UNDEFINED layout (e.g. a storage write target
        /// for conversion passes).
        RenderTexture CreateCubemapTexture(Resources& resources,
            uint32_t faceSize, VkFormat format, uint32_t mipLevels, VkImageUsageFlags usage,
            RenderSamplerHandle sampler);

        void DestroyTexture(Resources& resources, RenderTexture& texture);
    }
}
