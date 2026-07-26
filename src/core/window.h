#pragma once

#include <memory>
#include <vector>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class Window
    {
    public:
        Window(int width, int height, const char* title);
        ~Window();

        bool ShouldClose() const;
        void PollEvents() const;

        std::vector<const char*> GetRequiredInstanceExtensions() const;
        VkSurfaceKHR CreateSurface(VkInstance instance) const;
        void GetFramebufferSize(int* width, int* height) const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_pImpl;
    };
}