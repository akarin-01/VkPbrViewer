#include "asset_manager.h"

#include "core/log.h"
#include "core/path.h"
#include "resource/asset_utils.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        AssetManager::AssetManager()
            : m_meshCache("mesh asset", [](MeshAsset&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh: ", mesh.m_name);
                }),
            m_textureCache("texture asset", [](TextureAsset&& tex)
                {
                    Core::Log::Info("[Resource] Release texture: ", tex.m_name);
                })
        {
        }

        AssetManager::~AssetManager() = default;

        MeshAsset::Handle AssetManager::LoadMesh(const std::string& path)
        {
            const std::string key = Core::Path::Normalize(path);

            return m_meshCache.GetOrCreate(key, [this](const std::string& meshKey) -> std::optional<MeshAsset>
            {
                // LoadGltfMesh throws on failure, so a failed path is never
                // cached and the next call retries from scratch.
                MeshAsset mesh = AssetUtils::LoadGltfMesh(meshKey);
                KITA_LOG_DEBUG("[Resource] Create mesh: ", meshKey);
                Core::Log::Info("[Resource] Create mesh: ", mesh.m_name, ", ",
                    mesh.GetVertexCount(), " vertices, ", mesh.GetIndexCount(), " indices");

                return mesh;
            });
        }

        TextureAsset::Handle AssetManager::LoadTexture(const std::string& path, TextureAsset::Type type)
        {
            TextureKey key{ Core::Path::Normalize(path), type };

            return m_textureCache.GetOrCreate(key, [this](const TextureKey& texKey) -> std::optional<TextureAsset>
            {
                // LoadTexture throws on failure, so a failed key is never
                // cached and the next call retries from scratch.
                TextureAsset texture = AssetUtils::LoadTexture(texKey.m_path, texKey.m_type);
                KITA_LOG_DEBUG("[Resource] Create texture: ", texKey.m_path);
                Core::Log::Info("[Resource] Create texture: ", texture.m_name, ", ",
                    texture.m_width, "x", texture.m_height);

                return texture;
            });
        }
    }
}
