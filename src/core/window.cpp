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
        m_pImpl->m_window = glfwCreateWindow(800, 600, "Vk Pbr Viewer", nullptr, nullptr);
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
}