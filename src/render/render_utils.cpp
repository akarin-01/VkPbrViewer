#include "render_utils.h"

#include <stdexcept>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cmath>

namespace Kita::Pbrv
{
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies)
        {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                indices.m_graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
            if (presentSupport)
            {
                indices.m_presentFamily = i;
            }

            if (indices.IsComplete())
            {
                break;
            }

            ++i;
        }

        return indices;
    }

    SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device, VkSurfaceKHR surface)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.m_capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
        if (formatCount != 0)
        {
            details.m_formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.m_formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
        if (presentModeCount != 0)
        {
            details.m_presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.m_presentModes.data());
        }

        return details;
    }

    uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memoryProperties;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
        {
            if (typeFilter & (1 << i)
                && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }

        throw std::runtime_error("Failed to find suitable memory type!");
    }

    uint32_t CalculateMipLevels(uint32_t width, uint32_t height)
    {
        return static_cast<uint32_t>(
            std::floor(std::log2(std::max(width, height))) + 1
            );
    }

    void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkPipelineStageFlags2 srcStageMask, VkAccessFlags2 srcAccessMask,
        VkPipelineStageFlags2 dstStageMask, VkAccessFlags2 dstAccessMask,
        const VkImageSubresourceRange& range)
    {
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcStageMask = srcStageMask;
        barrier.srcAccessMask = srcAccessMask;
        barrier.dstStageMask = dstStageMask;
        barrier.dstAccessMask = dstAccessMask;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.image = image;
        barrier.subresourceRange = range;

        VkDependencyInfo dependencyInfo{};
        dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependencyInfo.dependencyFlags = 0;
        dependencyInfo.imageMemoryBarrierCount = 1;
        dependencyInfo.pImageMemoryBarriers = &barrier;

        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
    }

    void GenerateImageMipmaps(VkPhysicalDevice physicalDevice, VkCommandBuffer commandBuffer, VkImage image,
        uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t arrayLayers, VkFormat format, VkImageAspectFlags aspectMask,
        VkImageLayout finalLayout, VkPipelineStageFlags2 finalStageMask)
    {
        if (mipLevels == 0 || arrayLayers == 0)
        {
            throw std::runtime_error("Miplevels or arrayLayers argument is invalid");
        }

        // Check format support
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
        {
            throw std::runtime_error("Texture image format does not support linear blitting!");
        }

        int32_t mipWidth = static_cast<int32_t>(width);
        int32_t mipHeight = static_cast<int32_t>(height);

        const VkImageLayout srcLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        const VkImageLayout dstLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        VkImageSubresourceRange levelRange{};
        levelRange.aspectMask = aspectMask;
        levelRange.levelCount = 1;
        levelRange.baseArrayLayer = 0;
        levelRange.layerCount = arrayLayers;

        for (uint32_t i = 1; i < mipLevels; ++i)
        {
            // 1. Make sure level(i - 1) is src layout
            levelRange.baseMipLevel = i - 1;
            TransitionImageLayout(commandBuffer, image,
                dstLayout, srcLayout,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                levelRange);

            // 2. Make sure level(i) is dst layout
            // initial layout is dst layout

            // 3. Blit level(i - 1) to level(i)
            VkImageBlit blit{};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = aspectMask;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = arrayLayers;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] =
            {
                (mipWidth > 1 ? mipWidth / 2 : 1),
                (mipHeight > 1 ? mipHeight / 2 : 1),
                1
            };
            blit.dstSubresource.aspectMask = aspectMask;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = arrayLayers;

            vkCmdBlitImage(commandBuffer,
                image, srcLayout,
                image, dstLayout,
                1, &blit,
                VK_FILTER_LINEAR);

            // 4.1 Transition level(i - 1) to final layout
            levelRange.baseMipLevel = i - 1;
            TransitionImageLayout(commandBuffer, image,
                srcLayout, finalLayout,
                VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_READ_BIT,
                finalStageMask, VK_ACCESS_2_SHADER_READ_BIT,
                levelRange);

            if (mipWidth > 1)
            {
                mipWidth /= 2;
            }
            if (mipHeight > 1)
            {
                mipHeight /= 2;
            }
        }

        // 4.2 Transition last level to final layout
        // last level is dst layout instead of src layout
        levelRange.baseMipLevel = mipLevels - 1;
        TransitionImageLayout(commandBuffer, image,
            dstLayout, finalLayout,
            VK_PIPELINE_STAGE_2_TRANSFER_BIT, VK_ACCESS_2_TRANSFER_WRITE_BIT,
            finalStageMask, VK_ACCESS_2_SHADER_READ_BIT,
            levelRange);
    }

    void CopyBuffer(VkCommandBuffer commandBuffer, VkBuffer src, VkBuffer dst, VkDeviceSize size)
    {
        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = size;

        vkCmdCopyBuffer(commandBuffer, src, dst, 1, &copyRegion);
    }

    void CopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer src, VkImage dst, const VkBufferImageCopy& region)
    {
        vkCmdCopyBufferToImage(commandBuffer, src, dst,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    }

    VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo& createInfo)
    {
        VkDescriptorSetLayout layout;

        if (vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor set layout!");
        }

        return layout;
    }
}
