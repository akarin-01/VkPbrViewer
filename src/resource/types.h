#pragma once

#include "resource/constants.h"

#include <vulkan/vulkan.h>
#include <vector>
#include <array>

namespace Kita::Pbrv
{
    namespace Resource
    {
        using RenderBufferHandle = uint64_t;
        using RenderImageHandle = uint64_t;
        using RenderImageViewHandle = uint64_t;
        using RenderSamplerHandle = uint64_t;

        struct RenderBuffer
        {
            using Handle = RenderBufferHandle;
            VkBuffer m_buffer{ VK_NULL_HANDLE };
            VkDeviceMemory m_memory{ VK_NULL_HANDLE };
            void* m_mapped{ nullptr };
        };

        struct RenderImage
        {
            using Handle = RenderImageHandle;
            VkImage m_image{ VK_NULL_HANDLE };
            VkDeviceMemory m_memory{ VK_NULL_HANDLE };

            VkFormat m_format{ VK_FORMAT_UNDEFINED };
            VkExtent3D m_extent{ 0, 0, 1 };
            uint32_t m_mipLevels{ 1 };
            uint32_t m_arrayLayers{ 1 };
        };

        struct RenderImageView
        {
            using Handle = RenderImageViewHandle;
            VkImageView m_imageView{ VK_NULL_HANDLE };
        };

        struct RenderSampler
        {
            using Handle = RenderSamplerHandle;
            VkSampler m_sampler{ VK_NULL_HANDLE };
        };

        struct FrameInfo
        {
            bool m_swapChainRecreated{ false };
            VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
            uint32_t m_frameIndex{ 0 };
            uint32_t m_imageIndex{ 0 };
        };
    }
}
