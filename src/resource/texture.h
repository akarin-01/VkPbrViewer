#pragma once

#include "resource/handle.h"

#include <string>
#include <vector>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Resource
    {
        enum class TextureType
        {
            None,               // undefined
            Srgb,               // sRGB, 4 ch uint8 (albedo, emissive)
            Normal,             // linear, 4 ch uint8 (normal)
            MetallicRoughness,  // linear, 4 ch uint8 (metallic roughness)
            Linear,             // linear, 1 ch uint8 (AO)
            Hdr,                // linear, 4 ch float (skybox)
        };

        struct Texture
        {
            using Handle = Resource::Handle<Texture>;   // asset handle

            std::string m_name{ "empty" };
            std::vector<uint8_t> m_bytes;
            uint32_t m_width{ 0 };
            uint32_t m_height{ 0 };
            TextureType m_type{ TextureType::None };

            size_t GetByteCount() const { return m_bytes.size(); }
        };

        struct TextureKey
        {
            std::string m_path;
            TextureType m_type{ TextureType::None };
            bool operator==(const TextureKey& other) const
            {
                return m_path == other.m_path
                    && m_type == other.m_type;
            }
        };

        struct TextureKeyHash
        {
            size_t operator()(const TextureKey& key) const
            {
                size_t h = std::hash<std::string>{}(key.m_path);
                h ^= static_cast<size_t>(key.m_type) + 0x9e3779b9 + (h << 6) + (h >> 2);
                return h;
            }
        };
    }
}
