#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        struct QueueFamilyIndices
        {
            std::optional<uint32_t> m_graphicsFamily;
            std::optional<uint32_t> m_presentFamily;

            bool IsComplete()
            {
                return m_graphicsFamily.has_value()
                    && m_presentFamily.has_value();
            }
        };

        struct SwapChainSupportDetails
        {
            VkSurfaceCapabilitiesKHR m_capabilities;
            std::vector<VkSurfaceFormatKHR> m_formats;
            std::vector<VkPresentModeKHR> m_presentModes;
        };

        QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface);
        SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface);

        void TransitionImageLayout(
            VkCommandBuffer commandBuffer,
            VkImage image,
            VkImageLayout oldLayout, VkImageLayout newLayout,
            VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
            VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
            const VkImageSubresourceRange& range);
    }
}
