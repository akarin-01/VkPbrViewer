#include "asset_manager.h"

#include "core/log.h"
#include "core/path.h"
#include "resource/asset_utils.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        AssetManager::AssetManager()
            : m_meshTable([](MeshAsset&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh: ", mesh.m_name);
                }),
            m_textureTable([](TextureAsset&& tex)
                {
                    Core::Log::Info("[Resource] Release texture: ", tex.m_name);
                })
        {
        }

        AssetManager::~AssetManager() = default;

        MeshAsset::Handle AssetManager::LoadMesh(const std::string& path)
        {
            const std::string key = Core::Path::Normalize(path);

            auto it = m_meshIds.find(key);
            if (it != m_meshIds.end())
            {
                const ResourceId id = it->second;
                // The mapping can outlive its entry (every handle released):
                // a stale id falls through and is rebuilt below.
                if (m_meshTable.Has(id))
                {
                    // Cache hit: add ref
                    KITA_LOG_DEBUG("[Resource] Reuse mesh: ", key);
                    return m_meshTable.GetShared(id);
                }
            }

            // Cache miss: load. LoadGltfMesh throws on failure, so a failed
            // path is never cached and the next call retries from scratch.
            MeshAsset mesh = AssetUtils::LoadGltfMesh(key);
            KITA_LOG_DEBUG("[Resource] Create mesh: ", key);
            Core::Log::Info("[Resource] Create mesh: ", mesh.m_name, ", ",
                mesh.GetVertexCount(), " vertices, ", mesh.GetIndexCount(), " indices");

            MeshAsset::Handle handle = m_meshTable.Create(std::move(mesh));
            m_meshIds[key] = handle.GetId();
            return handle;
        }

        TextureAsset::Handle AssetManager::LoadTexture(const std::string& path, TextureAsset::Type type)
        {
            TextureKey key{ Core::Path::Normalize(path), type };
            auto it = m_textureIds.find(key);
            if (it != m_textureIds.end())
            {
                const ResourceId id = it->second;
                // The mapping can outlive its entry (every handle released):
                // a stale id falls through and is rebuilt below.
                if (m_textureTable.Has(id))
                {
                    // Cache hit: add ref
                    KITA_LOG_DEBUG("[Resource] Reuse texture: ", key.m_path);
                    return m_textureTable.GetShared(id);
                }
            }

            // Cache miss: load. LoadTexture throws on failure, so a failed
            // key is never cached and the next call retries from scratch.
            TextureAsset texture = AssetUtils::LoadTexture(key.m_path, key.m_type);
            KITA_LOG_DEBUG("[Resource] Create texture: ", key.m_path);
            Core::Log::Info("[Resource] Create texture: ", texture.m_name, ", ",
                texture.m_width, "x", texture.m_height);

            TextureAsset::Handle handle = m_textureTable.Create(std::move(texture));
            m_textureIds[key] = handle.GetId();
            return handle;
        }
    }
}
