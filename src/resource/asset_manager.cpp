#include "asset_manager.h"

#include "core/log.h"
#include "resource/asset_utils.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        AssetManager::AssetManager()
            : m_meshTable([](const std::string& path)
                {
                    Mesh mesh = AssetUtils::LoadGltfMesh(path);

                    Core::Log::Info("[Resource] Add mesh: ", mesh.m_name, ", ",
                        mesh.GetVertexCount(), " vertices, ", mesh.GetIndexCount(), " indices");

                    return mesh;
                },
                [](const std::string& path)
                {
                    KITA_LOG_DEBUG("[Resource] Reuse mesh: ", path);
                },
                [](Mesh&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh: ", mesh.m_name);
                }),
            m_textureTable([](const TextureKey& key)
                {
                    Texture texture = AssetUtils::LoadTexture(key.m_path, key.m_type);

                    Core::Log::Info("[Resource] Add texture: ", texture.m_name, ", ",
                        texture.m_width, "x", texture.m_height);

                    return texture;
                },
                [](const TextureKey& key)
                {
                    KITA_LOG_DEBUG("[Resource] Reuse texture: ", key.m_path);
                },
                [](Texture&& tex)
                {
                    Core::Log::Info("[Resource] Release texture: ", tex.m_name);
                })
        {
        }

        AssetManager::~AssetManager() = default;

        AssetManager::MeshHandle AssetManager::LoadMesh(const std::string& path)
        {
            return m_meshTable.GetOrCreate(path);
        }

        AssetManager::TextureHandle AssetManager::LoadTexture(const std::string& path, TextureType type)
        {
            return m_textureTable.GetOrCreate(TextureKey{ path, type });
        }
    }
}
