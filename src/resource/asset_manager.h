#pragma once

#include "resource/mapping_table.h"
#include "resource/handle.h"
#include "resource/mesh.h"
#include "resource/texture.h"

#include <string>

namespace Kita::Pbrv
{
    namespace Resource
    {
        class AssetManager
        {
        public:
            using MeshHandle = Mesh::Handle;
            using TextureHandle = Texture::Handle;

            AssetManager();
            ~AssetManager();

            MeshHandle LoadMesh(const std::string& path);
            TextureHandle LoadTexture(const std::string& path, TextureType type);

            const Mesh* GetMesh(ResourceId id) { return m_meshTable.Get(id); }
            const Texture* GetTexture(ResourceId id) { return m_textureTable.Get(id); }

            size_t GetMeshCount() const { return m_meshTable.Size(); }
            size_t GetTextureCount() const { return m_textureTable.Size(); }

        private:
            MappingTable<std::string, Mesh>          m_meshTable;
            MappingTable<TextureKey, Texture, TextureKeyHash> m_textureTable;
        };
    }
}
