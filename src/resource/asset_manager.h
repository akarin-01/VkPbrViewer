#pragma once

#include "resource/resource_table.h"
#include "resource/handle.h"
#include "resource/mesh.h"
#include "resource/texture.h"

#include <string>
#include <unordered_map>

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

            size_t GetMeshCount() const { return m_meshTable.Size(); }
            size_t GetTextureCount() const { return m_textureTable.Size(); }

        private:
            ResourceTable<Mesh> m_meshTable;
            ResourceTable<Texture> m_textureTable;

            std::unordered_map<std::string, ResourceId> m_meshPathToId;
            std::unordered_map<std::string, ResourceId> m_texturePathToId;
        };
    }
}
