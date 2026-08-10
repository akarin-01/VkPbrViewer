#pragma once

#include "render/render_resource_types.h"

#include <unordered_map>
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

        RenderBufferHandle CreateBuffer(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped = false);
        RenderBufferHandle CreateBufferWithData(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, const void* data, size_t size);
        RenderBuffer* GetBuffer(const RenderBufferHandle& handle) const;
        void DestroyBuffer(const RenderBufferHandle& handle);
        void WriteBuffer(const RenderBufferHandle& handle, const void* data, size_t size, size_t offset = 0);

        RenderImageHandle CreateImage(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties);
        RenderImage* GetImage(const RenderImageHandle& handle) const;
        void DestroyImage(const RenderImageHandle& handle);

    private:
        std::unique_ptr<RenderBuffer> CreateBufferHelper(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped = false) const;
        void DestroyBufferHelper(const RenderBuffer& buffer) const;
        void WriteBufferHelper(const RenderBuffer& buffer, const void* data, size_t size, size_t offset = 0) const;

        std::unique_ptr<RenderImage> CreateImageHelper(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties) const;
        void DestroyImageHelper(const RenderImage& image) const;

    private:
        const RenderContext& m_context;

        RenderBufferHandle m_nextBufferHandle{ 1 };
        std::unordered_map<RenderBufferHandle, std::unique_ptr<RenderBuffer>> m_buffers;
        RenderImageHandle m_nextImageHandle{ 1 };
        std::unordered_map<RenderImageHandle, std::unique_ptr<RenderImage>> m_images;
    };
}