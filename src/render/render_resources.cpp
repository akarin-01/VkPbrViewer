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
        RenderBufferHandle handle = m_buffers.Add(std::move(renderBuffer));
        KITA_LOG_DEBUG("[Resources] Create buffer(", handle, "), ", bufferInfo.size, " bytes");
        return handle;
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

        RenderBufferHandle handle = m_buffers.Add(std::move(buffer));
        KITA_LOG_DEBUG("[Resources] Create buffer(", handle, "), ", bufferInfo.size, " bytes");
        return handle;
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

    void RenderResources::WriteBuffer(RenderBufferHandle handle, const void* data, size_t size, size_t offset)
    {
        RenderBuffer* buffer = GetBuffer(handle);

        assert(buffer && "Invalid buffer handle to write");
        WriteBufferHelper(*buffer, data, size, offset);
    }

    RenderImageHandle RenderResources::CreateImage(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties)
    {
        auto renderImage = CreateImageHelper(imageInfo, properties);
        RenderImageHandle handle = m_images.Add(std::move(renderImage));
        KITA_LOG_DEBUG("[Resources] Create image(", handle, "), ",
            imageInfo.extent.width, "x", imageInfo.extent.height, ", ", imageInfo.mipLevels, " mips");
        return handle;
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
        ::Kita::Pbrv::TransitionImageLayout(commandBuffer, image->m_image,
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

        EndSingleTimeCommands(m_context.Device(), m_context.CommandPool(), m_context.GraphicsQueue(), commandBuffer);

        // Clean
        DestroyBufferHelper(*stagingBuffer);

        // Add image
        RenderImageHandle handle = m_images.Add(std::move(image));
        KITA_LOG_DEBUG("[Resources] Create image(", handle, "), ",
            imageInfo.extent.width, "x", imageInfo.extent.height, ", ", imageInfo.mipLevels, " mips");
        return handle;
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

    void RenderResources::GenerateImageMipmaps(RenderImageHandle handle, VkImageAspectFlags aspectMask, VkImageLayout finalLayout, VkPipelineStageFlags2 finalStageMask) const
    {
        RenderImage* image = GetImage(handle);
        if (!image)
        {
            Log::Warning("[Resources] Generate mipmaps: image(", handle, ") is invalid");
            return;
        }

        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_context.Device(), m_context.CommandPool());

        ::Kita::Pbrv::GenerateImageMipmaps(m_context.PhysicalDevice(), commandBuffer, image->m_image,
            image->m_extent.width, image->m_extent.height, image->m_mipLevels, image->m_arrayLayers, image->m_format, aspectMask,
            finalLayout, finalStageMask);

        EndSingleTimeCommands(m_context.Device(), m_context.CommandPool(), m_context.GraphicsQueue(), commandBuffer);
    }

    void RenderResources::TransitionImageLayout(RenderImageHandle handle, VkImageLayout oldLayout, VkImageLayout newLayout, VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask, VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask, VkImageAspectFlags aspectMask)
    {
        RenderImage* image = GetImage(handle);
        if (!image)
        {
            Log::Warning("[Resources] Transition layout: image(", handle, ") is invalid");
            return;
        }

        VkCommandBuffer commandBuffer = BeginSingleTimeCommands(m_context.Device(), m_context.CommandPool());

        VkImageSubresourceRange range{};
        range.aspectMask = aspectMask;
        range.baseMipLevel = 0;
        range.levelCount = image->m_mipLevels;
        range.baseArrayLayer = 0;
        range.layerCount = image->m_arrayLayers;

        ::Kita::Pbrv::TransitionImageLayout(commandBuffer, image->m_image,
            oldLayout, newLayout,
            srcStageMask, srcAccessMask,
            dstStageMask, dstAccessMask,
            range);

        EndSingleTimeCommands(m_context.Device(), m_context.CommandPool(), m_context.GraphicsQueue(), commandBuffer);
    }

    RenderImageViewHandle RenderResources::CreateImageView(const VkImageViewCreateInfo& createInfo)
    {
        auto imageView = CreateImageViewHelper(createInfo);
        RenderImageViewHandle handle = m_imageViews.Add(std::move(imageView));
        KITA_LOG_DEBUG("[Resources] Create image view(", handle, ")");
        return handle;
    }

    RenderImageViewHandle RenderResources::CreateImageView(RenderImageHandle imageHandle, VkImageViewType viewType, VkImageAspectFlags aspectMask)
    {
        RenderImage* image = GetImage(imageHandle);
        if (!image)
        {
            Log::Warning("[Resources] CreateImageView: image handle(", imageHandle, ") is invalid");
            return RenderImageViewHandle{};
        }

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image->m_image;
        imageViewInfo.viewType = viewType;
        imageViewInfo.format = image->m_format;
        imageViewInfo.subresourceRange.aspectMask = aspectMask;
        imageViewInfo.subresourceRange.baseMipLevel = 0;
        imageViewInfo.subresourceRange.levelCount = image->m_mipLevels;
        imageViewInfo.subresourceRange.baseArrayLayer = 0;
        imageViewInfo.subresourceRange.layerCount = image->m_arrayLayers;

        return CreateImageView(imageViewInfo);
    }

    RenderImageViewHandle RenderResources::CreateImageView(RenderImageHandle imageHandle, VkImageViewType viewType, const VkImageSubresourceRange& range)
    {
        RenderImage* image = GetImage(imageHandle);
        if (!image)
        {
            Log::Warning("[Resources] CreateImageView: image handle(", imageHandle, ") is invalid");
            return RenderImageViewHandle{};
        }

        VkImageViewCreateInfo imageViewInfo{};
        imageViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        imageViewInfo.image = image->m_image;
        imageViewInfo.viewType = viewType;
        imageViewInfo.format = image->m_format;
        imageViewInfo.subresourceRange = range;

        return CreateImageView(imageViewInfo);
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
        RenderSamplerHandle handle = m_samplers.Add(std::move(sampler));
        KITA_LOG_DEBUG("[Resources] Create sampler(", handle, ")");
        return handle;
    }

    RenderSamplerHandle RenderResources::CreateSamplerLinearRepeatMip()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        // TODO: Enable Anisotropy
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

        return CreateSampler(samplerInfo);
    }

    RenderSamplerHandle RenderResources::CreateSamplerLinearClampNoMip()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        // TODO: Enable Anisotropy
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        return CreateSampler(samplerInfo);
    }

    RenderSamplerHandle RenderResources::CreateSamplerLinearClampMip()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        // TODO: Enable Anisotropy
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = VK_LOD_CLAMP_NONE;

        return CreateSampler(samplerInfo);
    }

    RenderSamplerHandle RenderResources::CreateSamplerNearestClampNoMip()
    {
        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_NEAREST;
        samplerInfo.minFilter = VK_FILTER_NEAREST;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 0.0f;

        return CreateSampler(samplerInfo);
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

    std::unique_ptr<RenderImage> RenderResources::CreateImageHelper(const VkImageCreateInfo& imageInfo, VkMemoryPropertyFlags properties) const
    {
        RenderImage image{};
        image.m_format = imageInfo.format;
        image.m_extent = imageInfo.extent;
        image.m_mipLevels = imageInfo.mipLevels;
        image.m_arrayLayers = imageInfo.arrayLayers;

        // Image
        if (vkCreateImage(m_context.Device(), &imageInfo, nullptr, &image.m_image) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image!");
        }

        // Memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_context.Device(), image.m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(m_context.PhysicalDevice(), memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_context.Device(), &allocInfo, nullptr, &image.m_memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(m_context.Device(), image.m_image, image.m_memory, 0);

        return std::make_unique<RenderImage>(std::move(image));
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
}
