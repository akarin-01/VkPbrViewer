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