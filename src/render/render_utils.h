#pragma once

#include <optional>
#include <vector>
#include <vulkan/vulkan.h>
#include <string>

namespace Kita::Pbrv
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

    uint32_t FindMemoryType(
        VkPhysicalDevice physicalDevice,
        uint32_t typeFilter,
        VkMemoryPropertyFlags properties);

    uint32_t CalculateMipLevels(uint32_t width, uint32_t height);

    void TransitionImageLayout(
        VkCommandBuffer commandBuffer,
        VkImage image,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
        const VkImageSubresourceRange& range);

    /// Record commands for generating a full mipmap chain
    /// Precondition: every subresource must be in transfer dst layout
    /// Postcondition: every subresource is in final layout
    void GenerateImageMipmaps(
        VkPhysicalDevice physicalDevice,
        VkCommandBuffer commandBuffer,
        VkImage image,
        uint32_t width,
        uint32_t height,
        uint32_t mipLevels,
        uint32_t arrayLayers,
        VkFormat format,
        VkImageAspectFlags aspectMask,
        VkImageLayout finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        VkPipelineStageFlags2 finalStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT);

    void CopyBuffer(VkCommandBuffer commandBuffer, VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer src, VkImage dst, const VkBufferImageCopy& region);

    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo);
}
