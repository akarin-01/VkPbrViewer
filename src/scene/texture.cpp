#include "texture.h"

#include "core/log.h"

namespace Kita::Pbrv
{
    namespace
    {
        std::string TypeToString(Texture::Type type)
        {
            switch (type)
            {
            case Texture::Type::Albedo:
                return "Albedo";
            case Texture::Type::Normal:
                return "Normal";
            case Texture::Type::Linear:
                return "Linear";
            case Texture::Type::Hdr:
                return "Hdr";
            default:
                return "Unknown";
            }
        }
    }

    Texture::Texture() = default;

    Texture::~Texture() = default;

    void Texture::SetData(const std::string& name, std::vector<uint8_t>&& pixels, uint32_t width, uint32_t height, Type type)
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
        m_isDirty = true;

        KITA_LOG_DEBUG("[Scene] Set texture data: ", m_name, ", ",
            m_pixels.size(), " pixels, ",
            m_width, " * ", m_height, " ",
            TypeToString(m_type), " type");
    }

    void Texture::SetEmpty()
    {
        SetData("empty", {}, 0, 0, Type::None);
    }
}