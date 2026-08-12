#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <unordered_map>
#include <vector>
#include <array>
#include <memory>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderContext;

    /// @brief 管理渲染基础资源，与业务逻辑类型无关
    class RenderResources
    {
    public:
        RenderResources(const RenderContext& context);
        ~RenderResources();

        void FlushDeferred(uint32_t frameIndex);

        RenderBufferHandle CreateBuffer(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped = false);
        RenderBufferHandle CreateBufferWithData(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, const void* data, size_t size);
        RenderBuffer* GetBuffer(const RenderBufferHandle& handle) const;
        void DestroyBuffer(const RenderBufferHandle& handle);
        void WriteBuffer(const RenderBufferHandle& handle, const void* data, size_t size, size_t offset = 0);

        RenderImageHandle CreateImage(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties);
        RenderImageHandle CreateImageWithData(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties, const void* data, size_t size, VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT);
        RenderImage* GetImage(const RenderImageHandle& handle) const;
        void DestroyImage(const RenderImageHandle& handle);

        RenderImageViewHandle CreateImageView(const VkImageViewCreateInfo& createInfo);
        RenderImageView* GetImageView(const RenderImageViewHandle& handle) const;
        void DestroyImageView(const RenderImageViewHandle& handle);

        RenderSamplerHandle CreateSampler(const VkSamplerCreateInfo& createInfo);
        RenderSampler* GetSampler(const RenderSamplerHandle& handle) const;
        void DestroySampler(const RenderSamplerHandle& handle);

    private:
        struct DeferredQueue
        {
            std::vector<std::unique_ptr<RenderBuffer>> m_bufferQueue;
            std::vector<std::unique_ptr<RenderImage>> m_imageQueue;
            std::vector<std::unique_ptr<RenderImageView>> m_imageViewQueue;
            std::vector<std::unique_ptr<RenderSampler>> m_samplerQueue;
        };

    private:
        std::unique_ptr<RenderBuffer> CreateBufferHelper(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped = false) const;
        void DestroyBufferHelper(const RenderBuffer& buffer) const;
        void WriteBufferHelper(const RenderBuffer& buffer, const void* data, size_t size, size_t offset = 0) const;

        std::unique_ptr<RenderImage> CreateImageHelper(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties) const;
        void DestroyImageHelper(const RenderImage& image) const;

        std::unique_ptr<RenderImageView> CreateImageViewHelper(const VkImageViewCreateInfo& createInfo) const;
        void DestroyImageViewHelper(const RenderImageView& imageView) const;

        std::unique_ptr<RenderSampler> CreateSamplerHelper(const VkSamplerCreateInfo& createInfo) const;
        void DestroySamplerHelper(const RenderSampler& sampler) const;

        void FlushFrameDeferedQueue(uint32_t frameIndex);

    private:
        const RenderContext& m_context;

        RenderBufferHandle m_nextBufferHandle{ 1 };
        std::unordered_map<RenderBufferHandle, std::unique_ptr<RenderBuffer>> m_buffers;

        RenderImageHandle m_nextImageHandle{ 1 };
        std::unordered_map<RenderImageHandle, std::unique_ptr<RenderImage>> m_images;

        RenderImageViewHandle m_nextImageViewHandle{ 1 };
        std::unordered_map<RenderImageViewHandle, std::unique_ptr<RenderImageView>> m_imageViews;

        RenderSamplerHandle m_nextSamplerHandle{ 1 };
        std::unordered_map<RenderSamplerHandle, std::unique_ptr<RenderSampler>> m_samplers;

        uint32_t m_frameIndex{ 0 };
        std::array<DeferredQueue, kMaxFramesInFlight> m_deferredQueues;
    };
}