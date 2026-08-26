#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Scene
    {
        enum class TextureType
        {
            None,               // undefined
            Srgb,               // sRGB, 4 channel
            Normal,             // linear, decode *2-1 in shader
            MetallicRoughness,  // linear, 4 channel
            Linear,             // linear, 1 channel
        };

        class Texture
        {
        public:
            Texture();
            ~Texture();

            const std::string& GetName() const { return m_name; }
            const std::vector<uint8_t>& GetPixels() const { return m_pixels; }
            size_t GetPixelCount() const { return m_pixels.size(); }
            uint32_t GetWidth() const { return m_width; }
            uint32_t GetHeight() const { return m_height; }
            TextureType GetType() const { return m_type; }

            uint64_t GetRevision() const { return m_revision; }

            bool IsEmpty() const { return m_pixels.empty(); }

            void SetData(const std::string& name, std::vector<uint8_t>&& pixels, uint32_t width, uint32_t height, TextureType type);
            void SetEmpty();

        private:
            std::string m_name{ "empty" };
            std::vector<uint8_t> m_pixels;
            uint32_t m_width{ 0 };
            uint32_t m_height{ 0 };
            TextureType m_type{ TextureType::None };
            uint64_t m_revision{ 0 };
        };
    }
}
