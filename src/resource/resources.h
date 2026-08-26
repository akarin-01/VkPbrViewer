#pragma once

#include "resource/types.h"
#include "resource/constants.h"
#include "rhi/constants.h"

#include <unordered_map>
#include <vector>
#include <array>
#include <memory>
#include <functional>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class RenderContext;
    }

    namespace Resource
    {
        template <typename TResource>
        class ResourcePool
        {
        public:
            using Handle = typename TResource::Handle;
            using DestroyFn = std::function<void(TResource&)>;

            explicit ResourcePool(DestroyFn destroyer)
                : m_destroyer(destroyer)
            {
            }

            ~ResourcePool()
            {
                Clear();
            }

            Handle Add(std::unique_ptr<TResource> res)
            {
                Handle handle = m_nextHandle++;
                m_resources.emplace(handle, std::move(res));
                return handle;
            }

            TResource* Get(Handle handle) const
            {
                auto it = m_resources.find(handle);
                if (it == m_resources.end())
                {
                    return nullptr;
                }

                return it->second.get();
            }

            std::unique_ptr<TResource> Remove(Handle handle)
            {
                auto it = m_resources.find(handle);
                if (it == m_resources.end())
                {
                    return nullptr;
                }

                auto res = std::move(it->second);
                m_resources.erase(it);
                return res;
            }

            void Clear()
            {
                for (auto& [handle, res] : m_resources)
                {
                    m_destroyer(*res);
                }
                m_resources.clear();
            }

        private:
            DestroyFn m_destroyer;

            Handle m_nextHandle{ 1 };
            std::unordered_map<Handle, std::unique_ptr<TResource>> m_resources;
        };

        template <typename TResource>
        class DeferredQueue
        {
        public:
            using DestroyFn = std::function<void(TResource&)>;

            explicit DeferredQueue(DestroyFn destroyer)
                : m_destroyer(destroyer)
            {
            }

            ~DeferredQueue()
            {
                for (uint32_t i = 0; i < Rhi::kMaxFramesInFlight; ++i)
                {
                    Flush(i);
                }
            }

            void Push(uint32_t frameIndex, std::unique_ptr<TResource> res)
            {
                m_queues[frameIndex].push_back(std::move(res));
            }

            size_t Flush(uint32_t frameIndex)
            {
                auto& queue = m_queues[frameIndex];
                size_t count = queue.size();
                for (auto& res : queue)
                {
                    m_destroyer(*res);
                }
                queue.clear();
                return count;
            }

        private:
            DestroyFn m_destroyer;
            std::array<std::vector<std::unique_ptr<TResource>>, Rhi::kMaxFramesInFlight> m_queues{};
        };

        /// Manages low-level rendering resources, independent of business logic types
        class RenderResources
        {
        public:
            RenderResources(const Rhi::RenderContext& context);
            ~RenderResources();

            void FlushDeferred(uint32_t frameIndex);

            RenderBufferHandle CreateBuffer(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped = false);
            RenderBufferHandle CreateBufferWithData(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, const void* data, size_t size);
            RenderBuffer* GetBuffer(RenderBufferHandle handle) const;
            void DestroyBuffer(RenderBufferHandle handle);
            void WriteBuffer(RenderBufferHandle handle, const void* data, size_t size, size_t offset = 0);

            RenderImageHandle CreateImage(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties);
            RenderImageHandle CreateImageWithData(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties, const void* data, size_t size, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
            RenderImage* GetImage(RenderImageHandle handle) const;
            void DestroyImage(RenderImageHandle handle);
            void GenerateImageMipmaps(RenderImageHandle handle,
                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VkPipelineStageFlags2 finalStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT) const;
            void TransitionImageLayout(RenderImageHandle handle,
                VkImageLayout oldLayout, VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
                VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
            void TransitionImageLayout(RenderImageHandle handle,
                VkImageLayout oldLayout, VkImageLayout newLayout,
                VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
                VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
                const VkImageSubresourceRange& range);

            RenderImageViewHandle CreateImageView(const VkImageViewCreateInfo& createInfo);
            RenderImageViewHandle CreateImageView(RenderImageHandle imageHandle, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D, VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);
            RenderImageViewHandle CreateImageView(RenderImageHandle imageHandle, VkImageViewType viewType, const VkImageSubresourceRange& range);
            RenderImageView* GetImageView(RenderImageViewHandle handle) const;
            void DestroyImageView(RenderImageViewHandle handle);

            RenderSamplerHandle CreateSampler(const VkSamplerCreateInfo& createInfo);
            RenderSamplerHandle CreateSamplerLinearRepeatMip();
            RenderSamplerHandle CreateSamplerLinearClampNoMip();
            RenderSamplerHandle CreateSamplerLinearClampMip();
            RenderSamplerHandle CreateSamplerNearestClampNoMip();
            RenderSamplerHandle CreateSamplerEquirect();
            RenderSampler* GetSampler(RenderSamplerHandle handle) const;
            void DestroySampler(RenderSamplerHandle handle);

        private:
            std::unique_ptr<RenderBuffer> CreateBufferHelper(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped = false) const;
            void DestroyBufferHelper(const RenderBuffer& buffer) const;
            void WriteBufferHelper(const RenderBuffer& buffer, const void* data, size_t size, size_t offset = 0) const;

            std::unique_ptr<RenderImage> CreateImageHelper(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties) const;
            void DestroyImageHelper(const RenderImage& image) const;

            std::unique_ptr<RenderImageView> CreateImageViewHelper(const VkImageViewCreateInfo& createInfo) const;
            void DestroyImageViewHelper(const RenderImageView& imageView) const;

            std::unique_ptr<RenderSampler> CreateSamplerHelper(const VkSamplerCreateInfo& createInfo) const;
            void DestroySamplerHelper(const RenderSampler& sampler) const;

        private:
            const Rhi::RenderContext& m_context;

            ResourcePool<RenderBuffer> m_buffers;
            ResourcePool<RenderImage> m_images;
            ResourcePool<RenderImageView> m_imageViews;
            ResourcePool<RenderSampler> m_samplers;

            uint32_t m_frameIndex{ 0 };
            DeferredQueue<RenderBuffer> m_bufferQueue;
            DeferredQueue<RenderImage> m_imageQueue;
            DeferredQueue<RenderImageView> m_imageViewQueue;
            DeferredQueue<RenderSampler> m_samplerQueue;
        };
    }
}
