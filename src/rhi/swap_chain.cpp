#include "swap_chain.h"

#include "core/window.h"
#include "rhi/context.h"
#include "rhi/utils.h"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <cassert>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        namespace
        {
            VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
            {
                for (const auto& availableFormat : availableFormats)
                {
                    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB
                        && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
                    {
                        return availableFormat;
                    }
                }

                return availableFormats[0];
            }

            VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
            {
                for (const auto& availablePresentMode : availablePresentModes)
                {
                    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
                    {
                        return availablePresentMode;
                    }
                }
                return VK_PRESENT_MODE_FIFO_KHR;
            }

            VkExtent2D ChooseSwapExtent(const Core::Window& window, const VkSurfaceCapabilitiesKHR& capabilities)
            {
                if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
                {
                    return capabilities.currentExtent;
                }

                int width, height;
                window.GetFramebufferSize(&width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                actualExtent.width = std::clamp(actualExtent.width,
                    capabilities.minImageExtent.width,
                    capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height,
                    capabilities.minImageExtent.height,
                    capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }

        SwapChain::SwapChain(Core::Window& window, const Context& context)
            : m_window(window), m_context(context)
        {
            CreateSwapChain();
        }

        SwapChain::~SwapChain()
        {
            DestroySwapChain();
        }

        bool SwapChain::AcquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex)
        {
            VkResult result = vkAcquireNextImageKHR(m_context.Device(), m_swapChain, timeout, semaphore, fence, pImageIndex);
            if (result == VK_ERROR_OUT_OF_DATE_KHR)
            {
                RecreateSwapChain();
                return true;
            }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
            {
                throw std::runtime_error("Failed to acquire swap chain image!");
            }

            return false;
        }

        bool SwapChain::QueuePresent(const VkPresentInfoKHR& presentInfo)
        {
            VkResult result = vkQueuePresentKHR(m_context.PresentQueue(), &presentInfo);
            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_window.FramebufferWasResized())
            {
                m_window.ResetFramebufferResized();
                RecreateSwapChain();
                return true;
            }
            else if (result != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to present swap chain image!");
            }

            return false;
        }

        VkImage SwapChain::Image(uint32_t index) const
        {
            assert(index < m_images.size() && "Swap chain image index out of range");
            return m_images[index];
        }

        VkImageView SwapChain::ImageView(uint32_t index) const
        {
            assert(index < m_imageViews.size() && "Swap chain image view index out of range");
            return m_imageViews[index];
        }

        void SwapChain::CreateSwapChain()
        {
            SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(m_context.PhysicalDevice(), m_context.Surface());

            VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.m_formats);
            VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.m_presentModes);
            VkExtent2D extent = ChooseSwapExtent(m_window, swapChainSupport.m_capabilities);

            uint32_t imageCount = swapChainSupport.m_capabilities.minImageCount + 1;
            if (swapChainSupport.m_capabilities.maxImageCount > 0
                && imageCount > swapChainSupport.m_capabilities.maxImageCount)
            {
                imageCount = swapChainSupport.m_capabilities.maxImageCount;
            }

            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = m_context.Surface();
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = FindQueueFamilies(m_context.PhysicalDevice(), m_context.Surface());
            uint32_t queueFamilyIndices[] = { indices.m_graphicsFamily.value(), indices.m_presentFamily.value() };

            if (indices.m_graphicsFamily != indices.m_presentFamily)
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else
            {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // Optional
                createInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            createInfo.preTransform = swapChainSupport.m_capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(m_context.Device(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to create swap chain!");
            }

            m_format = surfaceFormat.format;
            m_extent = extent;

            vkGetSwapchainImagesKHR(m_context.Device(), m_swapChain, &imageCount, nullptr);
            m_images.resize(imageCount);
            vkGetSwapchainImagesKHR(m_context.Device(), m_swapChain, &imageCount, m_images.data());

            m_imageViews.resize(m_images.size());
            for (size_t i = 0; i < m_imageViews.size(); ++i)
            {
                VkImageViewCreateInfo imageViewCreateInfo{};
                imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
                imageViewCreateInfo.image = m_images[i];
                imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
                imageViewCreateInfo.format = m_format;
                imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
                imageViewCreateInfo.subresourceRange.levelCount = 1;
                imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
                imageViewCreateInfo.subresourceRange.layerCount = 1;

                if (vkCreateImageView(m_context.Device(), &imageViewCreateInfo, nullptr, &m_imageViews[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create swap chain image view!");
                }
            }
        }

        void SwapChain::DestroySwapChain()
        {
            for (const auto& imageView : m_imageViews)
            {
                vkDestroyImageView(m_context.Device(), imageView, nullptr);
            }
            m_imageViews.clear();

            vkDestroySwapchainKHR(m_context.Device(), m_swapChain, nullptr);
        }

        void SwapChain::RecreateSwapChain()
        {
            int width = 0, height = 0;
            m_window.GetFramebufferSize(&width, &height);
            while (width == 0 || height == 0)
            {
                m_window.GetFramebufferSize(&width, &height);
                m_window.WaitEvents();
            }

            vkDeviceWaitIdle(m_context.Device());

            DestroySwapChain();
            CreateSwapChain();
        }
    }
}
