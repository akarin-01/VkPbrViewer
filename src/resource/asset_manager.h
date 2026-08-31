#pragma once

#include "resource/cache_table.h"
#include "resource/resource_id.h"
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

            const MeshAsset* GetMesh(ResourceId id) const { return m_meshCache.Get(id); }
            const TextureAsset* GetTexture(ResourceId id) const { return m_textureCache.Get(id); }

            size_t GetMeshCount() const { return m_meshCache.Size(); }
            size_t GetTextureCount() const { return m_textureCache.Size(); }

        private:
            CacheTable<MeshAsset, std::string> m_meshCache;
            CacheTable<TextureAsset, TextureKey, TextureKey::Hash> m_textureCache;
        };
    }
}
