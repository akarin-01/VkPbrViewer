#pragma once

#include "resource/asset_types.h"
#include "resource/cache_table.h"
#include "resource/resource_id.h"

#include <cstdint>
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

            MeshView::Handle LoadMesh(const std::string& path, uint32_t partIndex = 0);
            TextureView::Handle LoadTexture(const std::string& path, TextureAsset::Type type);
            ModelLoadResult LoadModel(const std::string& path);

            /// Returns the borrowed MeshView entry for a view id.
            /// The returned pointer is valid only while the MeshView::Handle is alive.
            const MeshView* GetMesh(ResourceId id) const;

            /// Returns the borrowed TextureView entry for a view id.
            /// The returned pointer is valid only while the TextureView::Handle is alive.
            const TextureView* GetTexture(ResourceId id) const;

            size_t GetMeshCount() const;
            size_t GetTextureCount() const;
            size_t GetModelCount() const;

        private:
            ModelAsset::Handle GetOrCreateModel(const std::string& path);
            MeshView::Handle GetOrCreateMeshView(const ModelAsset::Handle& model,
                const std::string& path, uint32_t partIndex);

        private:
            CacheTable<ModelAsset, std::string> m_modelCache;
            CacheTable<MeshView, MeshKey, MeshKey::Hash> m_meshViewCache;

            CacheTable<TextureAsset, TextureKey, TextureKey::Hash> m_textureAssetCache;
            CacheTable<TextureView, TextureKey, TextureKey::Hash> m_textureViewCache;
        };
    }
}
