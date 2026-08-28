#pragma once

#include "resource/handle_table.h"
#include "resource/resource_id.h"
#include "resource/asset_types.h"

#include <string>
#include <unordered_map>

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

            const MeshAsset* GetMesh(ResourceId id) const { return m_meshTable.Get(id); }
            const TextureAsset* GetTexture(ResourceId id) const { return m_textureTable.Get(id); }

            size_t GetMeshCount() const { return m_meshTable.Size(); }
            size_t GetTextureCount() const { return m_textureTable.Size(); }

        private:
            HandleTable<MeshAsset> m_meshTable;
            HandleTable<TextureAsset> m_textureTable;

            std::unordered_map<std::string, ResourceId> m_meshIds;
            std::unordered_map<TextureKey, ResourceId, TextureKey::Hash> m_textureIds;
        };
    }
}
