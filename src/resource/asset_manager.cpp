#include "asset_manager.h"

#include "core/log.h"
#include "resource/asset_utils.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        AssetManager::AssetManager()
            : m_meshTable([](Mesh&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh: ", mesh.m_name);
                }),
            m_textureTable([](Texture&& tex)
                {
                    Core::Log::Info("[Resource] Release texture: ", tex.m_name);
                })
        {
        }

        AssetManager::~AssetManager() = default;

        AssetManager::MeshHandle AssetManager::LoadMesh(const std::string& path)
        {
            auto it = m_meshPathToId.find(path);
            if (it != m_meshPathToId.end() && m_meshTable.Has(it->second))
            {
                m_meshTable.AddRef(it->second);
                KITA_LOG_DEBUG("[Resource] Reuse mesh: ", path);
                return MeshHandle(it->second, &m_meshTable);
            }

            Mesh mesh = AssetUtils::LoadGltfMesh(path);

            Core::Log::Info("[Resource] Add mesh: ", mesh.m_name, ", ",
                mesh.GetVertexCount(), " vertices, ", mesh.GetIndexCount(), " indices");

            const ResourceId id = m_meshTable.Add(std::move(mesh));
            m_meshPathToId[path] = id;
            return MeshHandle(id, &m_meshTable);
        }

        AssetManager::TextureHandle AssetManager::LoadTexture(const std::string& path, TextureType type)
        {
            auto it = m_texturePathToId.find(path);
            if (it != m_texturePathToId.end() && m_textureTable.Has(it->second))
            {
                m_textureTable.AddRef(it->second);
                KITA_LOG_DEBUG("[Resource] Reuse texture: ", path);
                return TextureHandle(it->second, &m_textureTable);
            }

            Texture texture = AssetUtils::LoadTexture(path, type);

            Core::Log::Info("[Resource] Add texture: ", texture.m_name, ", ",
                texture.m_width, "x", texture.m_height);

            const ResourceId id = m_textureTable.Add(std::move(texture));
            m_texturePathToId[path] = id;
            return TextureHandle(id, &m_textureTable);
        }
    }
}
