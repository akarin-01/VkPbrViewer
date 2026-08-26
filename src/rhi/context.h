#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Window;
    }

    namespace Rhi
    {
        class RenderContext
        {
        public:
            RenderContext(const Core::Window& window);
            ~RenderContext();

            VkInstance Instance() const { return m_instance; }
            VkSurfaceKHR Surface() const { return m_surface; }
            VkPhysicalDevice PhysicalDevice() const { return m_physicalDevice; }
            VkDevice Device() const { return m_device; }
            VkQueue GraphicsQueue() const { return m_graphicsQueue; }
            uint32_t GraphicsFamily() const { return m_graphicsFamily; }
            VkQueue PresentQueue() const { return m_presentQueue; }
            uint32_t PresentFamily() const { return m_presentFamily; }
            VkCommandPool CommandPool() const { return m_commandPool; }

            VkFormat DepthFormat() const { return m_depthFormat; }
            VkFormat HdrFormat() const { return m_hdrFormat; }
            float MaxAnisotropy() const { return m_maxAnisotropy; }
            VkSampleCountFlags SupportedSampleCounts() const { return m_supportedSampleCounts; }
            VkSampleCountFlagBits MaxSampleCount() const { return m_maxSampleCount; }
            VkSampleCountFlagBits SampleCount() const { return m_sampleCount; }

        private:
            void CreateInstance();
            void SetupDebugMessenger();
            void CreateSurface();
            void PickPhysicalDevice();
            void CreateLogicalDevice();
            void CreateCommandPool();

            void CachePhysicalDeviceCaps();

        private:
            const Core::Window& m_window;

            VkInstance m_instance{ VK_NULL_HANDLE };
            VkDebugUtilsMessengerEXT m_debugMessenger{ VK_NULL_HANDLE };
            VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
            VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
            VkDevice m_device{ VK_NULL_HANDLE };
            VkQueue m_graphicsQueue{ VK_NULL_HANDLE };
            uint32_t m_graphicsFamily{ 0 };
            VkQueue m_presentQueue{ VK_NULL_HANDLE };
            uint32_t m_presentFamily{ 0 };
            VkCommandPool m_commandPool{ VK_NULL_HANDLE };

            VkFormat m_depthFormat{ VK_FORMAT_UNDEFINED };
            VkFormat m_hdrFormat{ VK_FORMAT_UNDEFINED };
            float m_maxAnisotropy{ 1.0f };
            VkSampleCountFlags m_supportedSampleCounts{ VK_SAMPLE_COUNT_1_BIT };
            VkSampleCountFlagBits m_maxSampleCount{ VK_SAMPLE_COUNT_1_BIT };
            VkSampleCountFlagBits m_sampleCount{ VK_SAMPLE_COUNT_1_BIT };
        };
    }
}
