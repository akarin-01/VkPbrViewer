#pragma once

#include "resource/constants.h"
#include "resource/handle.h"
#include "resource/vertex.h"

#include <array>
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
            bool IsEmpty() const { return m_bytes.empty(); }
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

        struct MeshKey
        {
            std::string m_path;
            uint32_t m_partIndex{ 0 };

            bool operator==(const MeshKey& other) const
            {
                return m_path == other.m_path
                    && m_partIndex == other.m_partIndex;
            }

            struct Hash
            {
                size_t operator()(const MeshKey& key) const
                {
                    size_t h = std::hash<std::string>{}(key.m_path);
                    h ^= static_cast<size_t>(key.m_partIndex) + 0x9e3779b9 + (h << 6) + (h >> 2);
                    return h;
                }
            };
        };

        struct MeshAsset
        {
            std::string m_name{ "empty" };
            std::vector<Vertex> m_vertices;
            std::vector<uint32_t> m_indices;

            size_t GetVertexCount() const { return m_vertices.size(); }
            const Vertex* GetVertexData() const { return m_vertices.data(); }
            size_t GetVertexDataSize() const { return sizeof(Vertex) * m_vertices.size(); }

            size_t GetIndexCount() const { return m_indices.size(); }
            const uint32_t* GetIndexData() const { return m_indices.data(); }
            size_t GetIndexDataSize() const { return sizeof(uint32_t) * m_indices.size(); }
        };

        struct MaterialParams
        {
            glm::vec4 m_baseColorFactor{ 1.0f, 1.0f, 1.0f, 1.0f };
            float m_metallicFactor{ 1.0f };
            float m_roughnessFactor{ 1.0f };
            float m_ao{ 1.0f };
            glm::vec3 m_emissiveFactor{ 0.0f };
            float m_emissiveIntensity{ 1.0f };
        };

        struct Transform
        {
            glm::vec3 m_position{ 0.0f };
            glm::vec3 m_rotation{ 0.0f };   // Euler angles in degrees
            glm::vec3 m_scale{ 1.0f };
        };

        struct ModelAsset
        {
            using Handle = Resource::Handle<ModelAsset>;

            struct Part
            {
                std::string m_name{ "empty" };
                MeshAsset m_mesh;
                MaterialParams m_material;
                std::array<TextureKey, kMaterialSlotCount> m_textureKeys{};
                Transform m_transform{};
            };

            std::string m_name{ "empty" };
            std::vector<Part> m_parts;
        };

        class MeshView
        {
        public:
            using Handle = Resource::Handle<MeshView>;

            MeshView() = default;

            /// A valid view implies its ModelAsset handle is valid and alive.
            /// A default-constructed view has no asset and is invalid.
            bool IsValid() const { return m_model.IsValid(); }
            explicit operator bool() const { return IsValid(); }

            const std::string& GetName() const
            {
                return m_model->m_parts[m_partIndex].m_name;
            }

            size_t GetVertexCount() const { return GetMesh().GetVertexCount(); }
            const Vertex* GetVertexData() const { return GetMesh().GetVertexData(); }
            size_t GetVertexDataSize() const { return GetMesh().GetVertexDataSize(); }

            size_t GetIndexCount() const { return GetMesh().GetIndexCount(); }
            const uint32_t* GetIndexData() const { return GetMesh().GetIndexData(); }
            size_t GetIndexDataSize() const { return GetMesh().GetIndexDataSize(); }

        private:
            friend class AssetManager;

            MeshView(Resource::Handle<ModelAsset> model, uint32_t partIndex)
                : m_model(std::move(model))
                , m_partIndex(partIndex)
            {
            }

            const MeshAsset& GetMesh() const
            {
                return m_model->m_parts[m_partIndex].m_mesh;
            }

            Resource::Handle<ModelAsset> m_model;
            uint32_t m_partIndex{ 0 };
        };

        class TextureView
        {
        public:
            using Handle = Resource::Handle<TextureView>;

            TextureView() = default;

            /// A valid view implies its TextureAsset handle is valid and alive.
            /// A default-constructed view has no asset and is invalid.
            bool IsValid() const { return m_texture.IsValid(); }
            explicit operator bool() const { return IsValid(); }

            const std::string& GetName() const { return m_texture->m_name; }
            uint32_t GetWidth() const { return m_texture->m_width; }
            uint32_t GetHeight() const { return m_texture->m_height; }
            TextureAsset::Type GetType() const { return m_texture->m_type; }
            bool IsEmpty() const { return m_texture->IsEmpty(); }
            size_t GetByteCount() const { return m_texture->GetByteCount(); }
            const uint8_t* GetData() const { return m_texture->m_bytes.data(); }

        private:
            friend class AssetManager;

            explicit TextureView(TextureAsset::Handle texture)
                : m_texture(std::move(texture))
            {
            }

            TextureAsset::Handle m_texture;
        };

        struct ModelInstance
        {
            std::string m_name;
            MeshView::Handle m_mesh;
            std::array<TextureView::Handle, kMaterialSlotCount> m_textures;
            MaterialParams m_material;
            Transform m_transform;
        };

        struct ModelLoadResult
        {
            std::vector<ModelInstance> m_instances;
        };
    }
}
