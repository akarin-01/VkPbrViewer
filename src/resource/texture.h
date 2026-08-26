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
    }
}
