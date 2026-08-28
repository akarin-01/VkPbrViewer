#pragma once

#include "resource/key_table.h"
#include "resource/handle.h"
#include "resource/asset_types.h"

#include <string>

namespace Kita::Pbrv
{
    namespace Resource
    {
        class AssetManager
        {
        public:
            AssetManager();
            ~AssetManager();

            MeshAsset::Handle LoadMesh(const std::string& path);
            TextureAsset::Handle LoadTexture(const std::string& path, TextureAsset::Type type);

            const MeshAsset* GetMesh(ResourceId id) { return m_meshTable.Get(id); }
            const TextureAsset* GetTexture(ResourceId id) { return m_textureTable.Get(id); }

            size_t GetMeshCount() const { return m_meshTable.Size(); }
            size_t GetTextureCount() const { return m_textureTable.Size(); }

        private:
            KeyTable<std::string, MeshAsset> m_meshTable;
            KeyTable<TextureKey, TextureAsset, TextureKey::Hash> m_textureTable;
        };
    }
}
