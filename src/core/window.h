#pragma once

#include <memory>

namespace Kita::Pbrv
{
    class Window
    {
    public:
        Window(int width, int height, const char* title);
        ~Window();

        bool ShouldClose() const;
        void PollEvents() const;

    private:
        class Impl;
        std::unique_ptr<Impl> m_pImpl;
    };
}