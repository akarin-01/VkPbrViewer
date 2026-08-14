#include "window.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <stdexcept>

namespace Kita::Pbrv
{
    class Window::Impl
    {
    public:
        static void OnScroll(GLFWwindow* window, double xOffset, double yOffset)
        {
            auto impl = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
            if (impl->m_scrollCallback)
            {
                impl->m_scrollCallback(xOffset, yOffset);
            }
        }

        static void OnFramebufferResized(GLFWwindow* window, int /*width*/, int /*height*/)
        {
            auto impl = reinterpret_cast<Window::Impl*>(glfwGetWindowUserPointer(window));
            impl->m_framebufferResized = true;
        }

    public:
        GLFWwindow* m_window{ nullptr };
        bool m_framebufferResized{ false };

        std::function<void(double, double)> m_scrollCallback;
    };

    Window::Window(int width, int height, const char* title)
        : m_pImpl(std::make_unique<Impl>())
    {
        if (!glfwInit())
        {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_pImpl->m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!m_pImpl->m_window)
        {
            throw std::runtime_error("Failed to create GLFW window");
        }
        glfwSetWindowUserPointer(m_pImpl->m_window, m_pImpl.get());
        glfwSetScrollCallback(m_pImpl->m_window, Impl::OnScroll);
        glfwSetFramebufferSizeCallback(m_pImpl->m_window, Impl::OnFramebufferResized);
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

    void Window::RequestClose() const
    {
        glfwSetWindowShouldClose(m_pImpl->m_window, GLFW_TRUE);
    }

    void Window::PollEvents() const
    {
        glfwPollEvents();
    }

    void Window::WaitEvents() const
    {
        glfwWaitEvents();
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

    bool Window::FramebufferWasResized() const
    {
        return m_pImpl->m_framebufferResized;
    }

    void Window::ResetFramebufferResized()
    {
        m_pImpl->m_framebufferResized = false;
    }

    void Window::SetScrollCallback(ScrollCallback callback)
    {
        m_pImpl->m_scrollCallback = std::move(callback);
    }

    void* Window::GetNativeHandle() const
    {
        return m_pImpl->m_window;
    }
}