#include "texture.h"

#include "core/log.h"

namespace Kita::Pbrv
{
    namespace
    {
        const char* ToString(TextureType type)
        {
            switch (type)
            {
            case TextureType::Srgb:
                return "Srgb";
            case TextureType::Normal:
                return "Normal";
            case TextureType::MetallicRoughness:
                return "MetallicRoughness";
            case TextureType::Linear:
                return "Linear";
            default:
                return "Unknown";
            }
        }
    }

    Texture::Texture() = default;

    Texture::~Texture() = default;

    void Texture::SetData(const std::string& name, std::vector<uint8_t>&& pixels, uint32_t width, uint32_t height, TextureType type)
    {
        bool unchanged = m_name == name
            && m_pixels == pixels
            && m_width == width
            && m_height == height
            && m_type == type;
        if (unchanged)
        {
            KITA_LOG_DEBUG("[Scene] Set texture data unchanged, skip: ", name);
            return;
        }

        m_name = name;
        m_pixels = std::move(pixels);
        m_width = width;
        m_height = height;
        m_type = type;
        ++m_revision;

        KITA_LOG_DEBUG("[Scene] Set texture data: ", m_name, ", ",
            m_pixels.size(), " pixels, ",
            m_width, " * ", m_height, " ",
            ToString(m_type), " type");
    }

    void Texture::SetEmpty()
    {
        SetData("empty", {}, 0, 0, TextureType::None);
    }
}
