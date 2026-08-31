#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Window;
    }

    namespace Rhi
    {
        class Context;

        /// Swapchain images are driver-owned (vkGetSwapchainImagesKHR only
        /// borrows them), so their views are created and destroyed here too —
        /// no resource manager involvement; recreation waits for the device.
        class SwapChain
        {
        public:
            SwapChain(Core::Window& window, const Context& context);
            ~SwapChain();

            bool AcquireNextImage(uint64_t timeout, VkSemaphore semaphore, VkFence fence, uint32_t* pImageIndex);
            bool QueuePresent(const VkPresentInfoKHR& presentInfo);

            VkSwapchainKHR Handle() const { return m_swapChain; }
            size_t ImageCount() const { return m_images.size(); }
            VkImage Image(uint32_t index) const;
            VkImageView ImageView(uint32_t index) const;
            VkFormat Format() const { return m_format; }
            VkExtent2D Extent() const { return m_extent; }
            float Aspect() const { return static_cast<float>(m_extent.width) / static_cast<float>(m_extent.height); }

        private:
            void CreateSwapChain();
            void DestroySwapChain();
            void RecreateSwapChain();

        private:
            Core::Window& m_window;
            const Context& m_context;

            VkSwapchainKHR m_swapChain{ VK_NULL_HANDLE };
            std::vector<VkImage> m_images;
            std::vector<VkImageView> m_imageViews;
            VkFormat m_format{ VK_FORMAT_UNDEFINED };
            VkExtent2D m_extent{ 0, 0 };
        };
    }
}
