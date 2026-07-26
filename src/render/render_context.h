#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class Window;

    class RenderContext
    {
    public:
        RenderContext(const Window& window);
        ~RenderContext();

        VkInstance Instance() const { return m_instance; }
        VkSurfaceKHR Surface() const { return m_surface; }
        VkPhysicalDevice PhysicalDevice() const { return m_physicalDevice; }
        VkDevice Device() const { return m_device; }
        VkQueue GraphicsQueue() const { return m_graphicsQueue; }
        VkQueue PresentQueue() const { return m_presentQueue; }
        VkCommandPool CommandPool() const { return m_commandPool; }

    private:
        void CreateInstance();
        void CreateSurface();
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void CreateCommandPool();

    private:
        const Window& m_window;

        VkInstance m_instance{ VK_NULL_HANDLE };
        VkSurfaceKHR m_surface{ VK_NULL_HANDLE };
        VkPhysicalDevice m_physicalDevice{ VK_NULL_HANDLE };
        VkDevice m_device{ VK_NULL_HANDLE };
        VkQueue m_graphicsQueue{ VK_NULL_HANDLE };
        VkQueue m_presentQueue{ VK_NULL_HANDLE };
        VkCommandPool m_commandPool{ VK_NULL_HANDLE };
    };
}