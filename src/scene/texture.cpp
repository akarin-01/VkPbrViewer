#include "texture.h"

#include "core/log.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <stdexcept>

namespace Kita::Pbrv
{
    static std::string TypeToString(Texture::Type type)
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

    Texture::Texture() = default;

    Texture::~Texture() = default;

    void Texture::LoadFromFile(const std::string& path, Type type)
    {
        // Load texture data
        Log::Info("[Scene] Load texture: ", path);

        std::vector<uint8_t> pixels;
        uint32_t width, height;
        {
            stbi_set_flip_vertically_on_load(true);

            bool isHdr = (type == Type::Hdr);
            if (isHdr)
            {
                // Todo: Load hdr texture
                throw std::runtime_error("Hdr not implemented");
            }
            else
            {
                int desiredChannels = (type == Type::Linear) ? 1 : 4;

                int texWidth, texHeight, texChannels;
                stbi_uc* data = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, desiredChannels);
                if (!data)
                {
                    throw std::runtime_error("Failed to load texture image!");
                }
                size_t byteSize = static_cast<size_t>(texWidth)
                    * static_cast<size_t>(texHeight)
                    * static_cast<size_t>(desiredChannels);
                pixels.assign(data, data + byteSize);
                width = static_cast<uint32_t>(texWidth);
                height = static_cast<uint32_t>(texHeight);

                stbi_image_free(data);
            }
        }

        std::string name = std::filesystem::path(path).stem().string();

        SetData(name, std::move(pixels), width, height, type);
    }

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