#include "render_resources.h"

#include "render/render_context.h"
#include "render/render_utils.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    RenderResources::RenderResources(const RenderContext& context)
        : m_context(context)
    {
    }

    RenderResources::~RenderResources()
    {
        for (auto& [handle, buffer] : m_buffers)
        {
            // Todo: Destroy immediately
            DestroyBufferHelper(*buffer);
        }
        m_buffers.clear();

        for (auto& [handle, image] : m_images)
        {
            // Todo: Destroy immediately
            DestroyImageHelper(*image);
        }
        m_images.clear();

        for (auto& [handle, imageView] : m_imageViews)
        {
            // Todo: Destroy immediately
            DestroyImageViewHelper(*imageView);
        }
        m_imageViews.clear();

        for (auto& [handle, sampler] : m_samplers)
        {
            // Todo: Destroy immediately
            DestroySamplerHelper(*sampler);
        }
        m_samplers.clear();
    }

    RenderBufferHandle RenderResources::CreateBuffer(const VkBufferCreateInfo& bufferInfo, VkMemoryPropertyFlags properties, bool mapped)
    {
        auto handle = m_nextBufferHandle++;
        auto renderBuffer = CreateBufferHelper(bufferInfo, properties, mapped);
        m_buffers.emplace(handle, std::move(renderBuffer));
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

        auto handle = m_nextBufferHandle++;
        m_buffers.emplace(handle, std::move(buffer));
        return handle;
    }

    RenderBuffer* RenderResources::GetBuffer(const RenderBufferHandle& handle) const
    {
        auto it = m_buffers.find(handle);
        if (it == m_buffers.end())
        {
            return nullptr;
        }

        return it->second.get();
    }

    void RenderResources::DestroyBuffer(const RenderBufferHandle& handle)
    {
        auto it = m_buffers.find(handle);
        if (it == m_buffers.end())
        {
            return;
        }

        DestroyBufferHelper(*it->second);
        m_buffers.erase(it);
    }

    RenderImageHandle RenderResources::CreateImage(VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties)
    {
        // Add image
        auto handle = m_nextImageHandle++;
        auto renderImage = CreateImageHelper(imageInfo, properties);
        m_images.emplace(handle, std::move(renderImage));
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
        TransitionImageLayout(commandBuffer, image->m_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            aspect, 0, imageInfo.mipLevels, 0, imageInfo.arrayLayers);

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
        auto handle = m_nextImageHandle++;
        m_images.emplace(handle, std::move(image));
        return handle;
    }

    RenderImage* RenderResources::GetImage(const RenderImageHandle& handle) const
    {
        auto it = m_images.find(handle);
        if (it == m_images.end())
        {
            return nullptr;
        }

        return it->second.get();
    }

    void RenderResources::DestroyImage(const RenderImageHandle& handle)
    {
        auto it = m_images.find(handle);
        if (it == m_images.end())
        {
            return;
        }

        DestroyImageHelper(*it->second);
        m_images.erase(it);
    }

    RenderImageViewHandle RenderResources::CreateImageView(const VkImageViewCreateInfo& createInfo)
    {
        // Add image view
        auto handle = m_nextImageViewHandle++;
        auto renderImageView = CreateImageViewHelper(createInfo);
        m_imageViews.emplace(handle, std::move(renderImageView));
        return handle;
    }

    RenderImageView* RenderResources::GetImageView(const RenderImageViewHandle& handle) const
    {
        auto it = m_imageViews.find(handle);
        if (it == m_imageViews.end())
        {
            return nullptr;
        }

        return it->second.get();
    }

    void RenderResources::DestroyImageView(const RenderImageViewHandle& handle)
    {
        auto it = m_imageViews.find(handle);
        if (it == m_imageViews.end())
        {
            return;
        }

        DestroyImageViewHelper(*it->second);
        m_imageViews.erase(it);
    }

    RenderSamplerHandle RenderResources::CreateSampler(const VkSamplerCreateInfo& createInfo)
    {
        // Add sampler
        auto handle = m_nextSamplerHandle++;
        auto renderSampler = CreateSamplerHelper(createInfo);
        m_samplers.emplace(handle, std::move(renderSampler));
        return handle;
    }

    RenderSampler* RenderResources::GetSampler(const RenderSamplerHandle& handle) const
    {
        auto it = m_samplers.find(handle);
        if (it == m_samplers.end())
        {
            return nullptr;
        }

        return it->second.get();
    }

    void RenderResources::DestroySampler(const RenderSamplerHandle& handle)
    {
        auto it = m_samplers.find(handle);
        if (it == m_samplers.end())
        {
            return;
        }

        DestroySamplerHelper(*it->second);
        m_samplers.erase(it);
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
        // Todo: Delay destroy
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
        // Todo: Delay destroy
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
            throw std::runtime_error("Failed to create shadow sampler!");
        }

        return std::make_unique<RenderSampler>(RenderSampler{ sampler });
    }

    void RenderResources::DestroySamplerHelper(const RenderSampler& sampler) const
    {
        vkDestroySampler(m_context.Device(), sampler.m_sampler, nullptr);
    }

    void RenderResources::WriteBuffer(const RenderBufferHandle& handle, const void* data, size_t size, size_t offset)
    {
        RenderBuffer* buffer = GetBuffer(handle);
        if (!buffer)
        {
            return;
        }

        WriteBufferHelper(*buffer, data, size, offset);
    }
}