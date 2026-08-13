#include "render_resources.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_utils.h"

#include <stdexcept>
#include <cassert>

namespace Kita::Pbrv
{
    RenderResources::RenderResources(const RenderContext& context)
        : m_context(context),
        m_buffers([this](RenderBuffer& b) { DestroyBufferHelper(b); }),
        m_images([this](RenderImage& i) { DestroyImageHelper(i); }),
        m_imageViews([this](RenderImageView& v) { DestroyImageViewHelper(v); }),
        m_samplers([this](RenderSampler& s) { DestroySamplerHelper(s); }),
        m_bufferQueue([this](RenderBuffer& b) { DestroyBufferHelper(b); }),
        m_imageQueue([this](RenderImage& i) { DestroyImageHelper(i); }),
        m_imageViewQueue([this](RenderImageView& v) { DestroyImageViewHelper(v); }),
        m_samplerQueue([this](RenderSampler& s) { DestroySamplerHelper(s); })
    {
    }

    RenderResources::~RenderResources()
    {
        for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
        {
            FlushDeferred(i);
        }

        m_buffers.Clear();
        m_images.Clear();
        m_imageViews.Clear();
        m_samplers.Clear();
    }

    void RenderResources::FlushDeferred(uint32_t frameIndex)
    {
        size_t bufferCount = m_bufferQueue.Flush(frameIndex);
        size_t imageCount = m_imageQueue.Flush(frameIndex);
        size_t imageViewCount = m_imageViewQueue.Flush(frameIndex);
        size_t samplerCount = m_samplerQueue.Flush(frameIndex);

        bool empty = (bufferCount == 0
            && imageCount == 0
            && imageViewCount == 0
            && samplerCount == 0);

        if (!empty)
        {
            KITA_LOG_DEBUG("[Resources] Flush ",
                bufferCount, " buffers, ",
                imageCount, " images, ",
                imageViewCount, " imageViews, ",
                samplerCount, " samplers -> frame ",
                frameIndex);
        }

        m_frameIndex = frameIndex;
    }

    RenderBufferHandle RenderResources::CreateBuffer(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped)
    {
        auto renderBuffer = CreateBufferHelper(bufferInfo, properties, mapped);
        return m_buffers.Add(std::move(renderBuffer));
    }

    RenderBufferHandle RenderResources::CreateBufferWithData(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, const void* data, size_t size)
    {
        auto buffer = CreateBufferHelper(bufferInfo, properties);

        VkBufferCreateInfo stagingInfo{};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = size;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        auto stagingBuffer = CreateBufferHelper(stagingInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
        WriteBufferHelper(*stagingBuffer, data, size, 0);

        // Copy data
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_context.Device(), m_context.CommandPool());
        CopyBuffer(commandBuffer, stagingBuffer->m_buffer, buffer->m_buffer, size);
        EndSingleTimeCommands(m_context.Device(), m_context.CommandPool(), m_context.GraphicsQueue(), commandBuffer);

        // Clean
        DestroyBufferHelper(*stagingBuffer);

        return m_buffers.Add(std::move(buffer));
    }

    RenderBuffer* RenderResources::GetBuffer(RenderBufferHandle handle) const
    {
        return m_buffers.Get(handle);
    }

    void RenderResources::DestroyBuffer(RenderBufferHandle handle)
    {
        auto buffer = m_buffers.Remove(handle);
        if (!buffer)
        {
            return;
        }

        // Deferred destruction
        m_bufferQueue.Push(m_frameIndex, std::move(buffer));
        KITA_LOG_DEBUG("[Resources] Defer destroy buffer(", handle, ") -> frame ", m_frameIndex);
    }

    RenderImageHandle RenderResources::CreateImage(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties)
    {
        auto renderImage = CreateImageHelper(imageInfo, properties);
        return m_images.Add(std::move(renderImage));
    }

    RenderImageHandle RenderResources::CreateImageWithData(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties, const void* data, size_t size, VkImageAspectFlags aspect)
    {
        auto image = CreateImageHelper(imageInfo, properties);

        // Copy data
        VkBufferCreateInfo stagingInfo{};
        stagingInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        stagingInfo.size = size;
        stagingInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        auto stagingBuffer = CreateBufferHelper(stagingInfo, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true);
        WriteBufferHelper(*stagingBuffer, data, size, 0);

        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_context.Device(), m_context.CommandPool());

        // Transition the image layout to transfer dst optimal
        VkImageSubresourceRange range{};
        range.aspectMask = aspect;
        range.baseMipLevel = 0;
        range.levelCount = imageInfo.mipLevels;
        range.baseArrayLayer = 0;
        range.layerCount = imageInfo.arrayLayers;
        TransitionImageLayout(commandBuffer, image->m_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            range);

        // Copy data
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = aspect;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = imageInfo.extent;
        CopyBufferToImage(commandBuffer, stagingBuffer->m_buffer, image->m_image, region);

        // Generate mipmap
        GenerateImageMipmaps(m_context.PhysicalDevice(), commandBuffer, image->m_image,
            imageInfo.extent.width, imageInfo.extent.height,
            imageInfo.mipLevels, imageInfo.format, aspect);

        EndSingleTimeCommands(m_context.Device(), m_context.CommandPool(), m_context.GraphicsQueue(), commandBuffer);

        // Clean
        DestroyBufferHelper(*stagingBuffer);

        // Add image
        return m_images.Add(std::move(image));
    }

    RenderImage* RenderResources::GetImage(RenderImageHandle handle) const
    {
        return m_images.Get(handle);
    }

    void RenderResources::DestroyImage(RenderImageHandle handle)
    {
        auto image = m_images.Remove(handle);
        if (!image)
        {
            return;
        }

        // Deferred destruction
        m_imageQueue.Push(m_frameIndex, std::move(image));
        KITA_LOG_DEBUG("[Resources] Defer destroy image(", handle, ") -> frame ", m_frameIndex);
    }

    RenderImageViewHandle RenderResources::CreateImageView(const VkImageViewCreateInfo& createInfo)
    {
        auto imageView = CreateImageViewHelper(createInfo);
        return m_imageViews.Add(std::move(imageView));
    }

    RenderImageView* RenderResources::GetImageView(RenderImageViewHandle handle) const
    {
        return m_imageViews.Get(handle);
    }

    void RenderResources::DestroyImageView(RenderImageViewHandle handle)
    {
        auto imageView = m_imageViews.Remove(handle);
        if (!imageView)
        {
            return;
        }

        // Deferred destruction
        m_imageViewQueue.Push(m_frameIndex, std::move(imageView));
        KITA_LOG_DEBUG("[Resources] Defer destroy image view(", handle, ") -> frame ", m_frameIndex);
    }

    RenderSamplerHandle RenderResources::CreateSampler(const VkSamplerCreateInfo& createInfo)
    {
        auto sampler = CreateSamplerHelper(createInfo);
        return m_samplers.Add(std::move(sampler));
    }

    RenderSampler* RenderResources::GetSampler(RenderSamplerHandle handle) const
    {
        return m_samplers.Get(handle);
    }

    void RenderResources::DestroySampler(RenderSamplerHandle handle)
    {
        auto sampler = m_samplers.Remove(handle);
        if (!sampler)
        {
            return;
        }

        // Deferred destruction
        m_samplerQueue.Push(m_frameIndex, std::move(sampler));
        KITA_LOG_DEBUG("[Resources] Defer destroy sampler(", handle, ") -> frame ", m_frameIndex);
    }

    std::unique_ptr<RenderBuffer> RenderResources::CreateBufferHelper(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped) const
    {
        // Buffer
        VkBuffer buffer{};
        if (vkCreateBuffer(m_context.Device(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create buffer!");
        }

        // memory
        VkDeviceMemory memory{};
        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_context.Device(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(m_context.PhysicalDevice(), memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_context.Device(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate buffer memory!");
        }

        // bind buffer and memory
        vkBindBufferMemory(m_context.Device(), buffer, memory, 0);

        void* data{ nullptr };
        if (mapped)
        {
            vkMapMemory(m_context.Device(), memory, 0, VK_WHOLE_SIZE, 0, &data);
        }

        return std::make_unique<RenderBuffer>(RenderBuffer{ buffer, memory, data });
    }

    void RenderResources::DestroyBufferHelper(const RenderBuffer& buffer) const
    {
        if (buffer.m_mapped)
        {
            vkUnmapMemory(m_context.Device(), buffer.m_memory);
        }
        vkDestroyBuffer(m_context.Device(), buffer.m_buffer, nullptr);
        vkFreeMemory(m_context.Device(), buffer.m_memory, nullptr);
    }

    void RenderResources::WriteBufferHelper(const RenderBuffer& buffer, const void* data, size_t size, size_t offset) const
    {
        if (!buffer.m_mapped)
        {
            throw std::runtime_error("Buffer is not mapped!");
        }

        memcpy(static_cast<char*>(buffer.m_mapped) + offset, data, size);
    }

    std::unique_ptr<RenderImage> RenderResources::CreateImageHelper(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties) const
    {
        // Image
        VkImage image{};
        if (vkCreateImage(m_context.Device(), &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image!");
        }

        // Memory
        VkDeviceMemory memory{};
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_context.Device(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(m_context.PhysicalDevice(), memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_context.Device(), &allocInfo, nullptr, &memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(m_context.Device(), image, memory, 0);

        return std::make_unique<RenderImage>(RenderImage{ image, memory });
    }

    void RenderResources::DestroyImageHelper(const RenderImage& image) const
    {
        vkDestroyImage(m_context.Device(), image.m_image, nullptr);
        vkFreeMemory(m_context.Device(), image.m_memory, nullptr);
    }

    std::unique_ptr<RenderImageView> RenderResources::CreateImageViewHelper(const VkImageViewCreateInfo& createInfo) const
    {
        VkImageView imageView;

        if (vkCreateImageView(m_context.Device(), &createInfo, nullptr, &imageView) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image views!");
        }

        return std::make_unique<RenderImageView>(RenderImageView{ imageView });
    }

    void RenderResources::DestroyImageViewHelper(const RenderImageView& imageView) const
    {
        vkDestroyImageView(m_context.Device(), imageView.m_imageView, nullptr);
    }

    std::unique_ptr<RenderSampler> RenderResources::CreateSamplerHelper(const VkSamplerCreateInfo& createInfo) const
    {
        VkSampler sampler;

        if (vkCreateSampler(m_context.Device(), &createInfo, nullptr, &sampler) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create sampler!");
        }

        return std::make_unique<RenderSampler>(RenderSampler{ sampler });
    }

    void RenderResources::DestroySamplerHelper(const RenderSampler& sampler) const
    {
        vkDestroySampler(m_context.Device(), sampler.m_sampler, nullptr);
    }

    void RenderResources::WriteBuffer(RenderBufferHandle handle, const void* data, size_t size, size_t offset)
    {
        RenderBuffer* buffer = GetBuffer(handle);

        assert(buffer && "Invalid buffer handle to write");
        WriteBufferHelper(*buffer, data, size, offset);
    }
}
