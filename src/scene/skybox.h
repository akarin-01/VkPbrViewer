#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Kita::Pbrv
{
    class Skybox
    {
    public:
        Skybox();
        ~Skybox();

        const std::string& GetName() const { return m_name; }
        const std::vector<float>& GetPixels() const { return m_pixels; }
        size_t GetPixelCount() const { return m_pixels.size(); }
        uint32_t GetWidth() const { return m_width; }
        uint32_t GetHeight() const { return m_height; }

        bool IsDirty() const { return m_isDirty; }
        void ClearDirty() const { m_isDirty = false; }

        bool IsEmpty() const { return m_pixels.empty(); }

        void SetData(const std::string& name, std::vector<float>&& pixels, uint32_t width, uint32_t height);
        void SetEmpty();

    private:
        std::string m_name{ "empty" };
        std::vector<float> m_pixels;
        uint32_t m_width{ 0 };
        uint32_t m_height{ 0 };
        mutable bool m_isDirty{ false };
    };
}