#pragma once

#include "resource/types.h"

#include <vulkan/vulkan.h>
#include <vector>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Window;
    }
    namespace Resource
    {
        class RenderResources;
    }

    namespace Rhi
    {
        class RenderContext;

        class SwapChain
        {
        public:
            SwapChain(Core::Window& window, const RenderContext& context, Resource::RenderResources& resources);
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
            const RenderContext& m_context;
            Resource::RenderResources& m_resources;

            VkSwapchainKHR m_swapChain{ VK_NULL_HANDLE };
            std::vector<VkImage> m_images;
            std::vector<Resource::RenderImageViewHandle> m_imageViewHandles;
            VkFormat m_format{ VK_FORMAT_UNDEFINED };
            VkExtent2D m_extent{ 0, 0 };
        };
    }
}
