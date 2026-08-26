#pragma once

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <functional>

namespace Kita::Pbrv
{
    namespace Core
    {
        class Window
        {
        public:
            using ScrollCallback = std::function<void(double, double)>;

            Window(int width, int height, const char* title);
            ~Window();

            bool ShouldClose() const;
            void RequestClose() const;
            void PollEvents() const;
            void WaitEvents() const;

            std::vector<const char*> GetRequiredInstanceExtensions() const;
            VkSurfaceKHR CreateSurface(VkInstance instance) const;
            void GetFramebufferSize(int* width, int* height) const;

            bool FramebufferWasResized() const;
            void ResetFramebufferResized();

            void SetScrollCallback(ScrollCallback callback);

            void* GetNativeHandle() const;

        private:
            class Impl;
            std::unique_ptr<Impl> m_pImpl;
        };
    }
}
