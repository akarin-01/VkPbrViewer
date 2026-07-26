#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace Kita::Pbrv
{
    class Window::Impl
    {
    public:
        GLFWwindow* m_window{ nullptr };
    };

    Window::Window(int width, int height, const char* title)
        : m_pImpl(std::make_unique<Impl>())
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_pImpl->m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_pImpl->m_window)
        {
            throw std::runtime_error("Failed to create GLFW window");
        }
    }

    Window::~Window()
    {
        if (m_pImpl->m_window)
        {
            glfwDestroyWindow(m_pImpl->m_window);
        }
        glfwTerminate();
    }

    bool Window::ShouldClose() const
    {
        return glfwWindowShouldClose(m_pImpl->m_window);
    }

    void Window::PollEvents() const
    {
        glfwPollEvents();
    }

    std::vector<const char*> Window::GetRequiredInstanceExtensions() const
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;

        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        return std::vector<const char*>(glfwExtensions, glfwExtensions + glfwExtensionCount);
    }

    VkSurfaceKHR Window::CreateSurface(VkInstance instance) const
    {
        VkSurfaceKHR surface;
        if (glfwCreateWindowSurface(instance, m_pImpl->m_window, nullptr, &surface))
        {
            throw std::runtime_error("Failed to create window surface!");
        }
        return surface;
    }

    void Window::GetFramebufferSize(int* width, int* height) const
    {
        glfwGetFramebufferSize(m_pImpl->m_window, width, height);
    }
}