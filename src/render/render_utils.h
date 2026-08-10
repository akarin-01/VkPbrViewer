#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    struct QueueFamilyIndices
    {
        std::optional<uint32_t> m_graphicsFamily;
        std::optional<uint32_t> m_presentFamily;

        bool isComplete()
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

    uint32_t FindMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties);

    VkImageView CreateImageView(VkDevice device, const VkImageViewCreateInfo& createInfo);

    void TransitionImageLayout(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        uint32_t baseMipLevel = 0, uint32_t levelCount = 1,
        uint32_t baseArrayLayer = 0, uint32_t layerCount = 1);

    VkCommandBuffer BeginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
    void EndSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkQueue submitQueue, VkCommandBuffer commandBuffer);

    void CopyBuffer(VkCommandBuffer commandBuffer, VkBuffer src, VkBuffer dst, VkDeviceSize size);

    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo);
}