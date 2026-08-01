#include "render_image.h"

#include "render/render_context.h"
#include "render/render_utils.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    RenderImage::RenderImage(const RenderContext& context, VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties)
        : m_context(context)
    {
        // Image
        if (vkCreateImage(m_context.Device(), &imageInfo, nullptr, &m_image) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create image!");
        }

        // Memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_context.Device(), m_image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(m_context.PhysicalDevice(), memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_context.Device(), &allocInfo, nullptr, &m_memory) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate image memory!");
        }

        vkBindImageMemory(m_context.Device(), m_image, m_memory, 0);
    }

    RenderImage::~RenderImage()
    {
        vkDestroyImage(m_context.Device(), m_image, nullptr);
        vkFreeMemory(m_context.Device(), m_memory, nullptr);
    }
}