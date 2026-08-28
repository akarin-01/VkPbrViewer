#pragma once

#include "resource/handle.h"
#include "resource/vertex.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace Kita::Pbrv
{
    namespace Resource
    {
        struct TextureAsset
        {
            using Handle = Resource::Handle<TextureAsset>;

            enum class Type
            {
                None,               // undefined
                Srgb,               // sRGB, 4 ch uint8 (albedo, emissive)
                Normal,             // linear, 4 ch uint8 (normal)
                MetallicRoughness,  // linear, 4 ch uint8 (metallic roughness)
                Linear,             // linear, 1 ch uint8 (AO)
                Hdr,                // linear, 4 ch float (skybox)
            };

            std::string m_name{ "empty" };
            std::vector<uint8_t> m_bytes;
            uint32_t m_width{ 0 };
            uint32_t m_height{ 0 };
            Type m_type{ Type::None };

            size_t GetByteCount() const { return m_bytes.size(); }
        };

        struct TextureKey
        {
            std::string m_path;
            TextureAsset::Type m_type{ TextureAsset::Type::None };

            bool operator==(const TextureKey& other) const
            {
                return m_path == other.m_path
                    && m_type == other.m_type;
            }

            struct Hash
            {
                size_t operator()(const TextureKey& key) const
                {
                    size_t h = std::hash<std::string>{}(key.m_path);
                    h ^= static_cast<size_t>(key.m_type) + 0x9e3779b9 + (h << 6) + (h >> 2);
                    return h;
                }
            };
        };

        struct MeshAsset
        {
            using Handle = Resource::Handle<MeshAsset>;

            std::string m_name{ "empty" };
            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;

            size_t GetVertexCount() const { return m_vertices.size(); }
            size_t GetVertexDataSize() const { return sizeof(Vertex) * m_vertices.size(); }
            size_t GetIndexCount() const { return m_indices.size(); }
            size_t GetIndexDataSize() const { return sizeof(uint32_t) * m_indices.size(); }
        };
    }
}
