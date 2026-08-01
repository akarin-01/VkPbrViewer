#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderContext;

    class RenderImage
    {
    public:
        RenderImage(const RenderContext& context, VkImageCreateInfo imageInfo, VkMemoryPropertyFlags properties);
        ~RenderImage();

        VkImage Image() const { return m_image; }
        VkDeviceMemory Memory() const { return m_memory; }

    private:
        const RenderContext& m_context;

        VkImage m_image{ VK_NULL_HANDLE };
        VkDeviceMemory m_memory{ VK_NULL_HANDLE };
    };
}