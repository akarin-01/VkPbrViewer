#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Kita::Pbrv
{
    class Texture
    {
    public:
        enum class Type
        {
            None,       // undefined
            Albedo,     // sRGB
            Normal,     // linear, decode *2-1 in shader
            Linear,     // linear, single channel
            Hdr,        // linear, high-precision float
        };

        Texture();
        ~Texture();

        const std::string& GetName() const { return m_name; }
        const std::vector<uint8_t>& GetPixels() const { return m_pixels; }
        size_t GetPixelCount() const { return m_pixels.size(); }
        uint32_t GetWidth() const { return m_width; }
        uint32_t GetHeight() const { return m_height; }
        Type GetType() const { return m_type; }

        bool IsDirty() const { return m_isDirty; }
        void ClearDirty() const { m_isDirty = false; }

        bool IsEmpty() const { return m_pixels.empty(); }

        void SetData(const std::string& name, std::vector<uint8_t>&& pixels, uint32_t width, uint32_t height, Type type);
        void SetEmpty();

    private:
        std::string m_name{ "empty" };
        std::vector<uint8_t> m_pixels;
        uint32_t m_width{ 0 };
        uint32_t m_height{ 0 };
        Type m_type{ Type::None };
        mutable bool m_isDirty{ false };
    };
}
