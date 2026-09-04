#include "asset_manager.h"

#include "core/log.h"
#include "core/path.h"
#include "resource/asset_utils.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        AssetManager::AssetManager()
            : m_modelCache("model asset", [](ModelAsset&& model)
                {
                    Core::Log::Info("[Resource] Release model asset: ", model.m_name);
                }),
            m_meshViewCache("mesh view", [](MeshView&&)
                {
                    Core::Log::Info("[Resource] Release mesh view");
                }),
            m_textureAssetCache("texture asset", [](TextureAsset&& tex)
                {
                    Core::Log::Info("[Resource] Release texture asset: ", tex.m_name);
                }),
            m_textureViewCache("texture view", [](TextureView&&)
                {
                    Core::Log::Info("[Resource] Release texture view");
                })
        {
        }

        AssetManager::~AssetManager() = default;

        MeshView::Handle AssetManager::LoadMesh(const std::string& path, uint32_t partIndex)
        {
            ModelAsset::Handle model = GetOrCreateModel(path);
            if (!model || model->m_parts.empty())
            {
                return {};
            }

            if (partIndex >= model->m_parts.size())
            {
                Core::Log::Warning("[Resource] LoadMesh part index out of range: ", partIndex);
                return {};
            }

            return GetOrCreateMeshView(model, path, partIndex);
        }

        TextureView::Handle AssetManager::LoadTexture(const std::string& path, TextureAsset::Type type)
        {
            if (path.empty())
            {
                return {};
            }

            const TextureKey key{ Core::Path::Normalize(path), type };

            TextureAsset::Handle asset = m_textureAssetCache.GetOrCreate(key,
                [this](const TextureKey& texKey) -> std::optional<TextureAsset>
                {
                    TextureAsset texture = AssetUtils::LoadTexture(texKey.m_path, texKey.m_type);
                    Core::Log::Info("[Resource] Create texture asset: ", texture.m_name, ", ",
                        texture.m_width, "x", texture.m_height);

                    return texture;
                });

            if (!asset)
            {
                return {};
            }

            return m_textureViewCache.GetOrCreate(key,
                [asset](const TextureKey&) -> std::optional<TextureView>
                {
                    return TextureView(asset);
                });
        }

        ModelLoadResult AssetManager::LoadModel(const std::string& path)
        {
            ModelAsset::Handle model = GetOrCreateModel(path);

            ModelLoadResult result;
            if (!model)
            {
                return result;
            }

            for (uint32_t i = 0; i < model->m_parts.size(); ++i)
            {
                const ModelAsset::Part& part = model->m_parts[i];

                ModelInstance instance;
                instance.m_mesh = GetOrCreateMeshView(model, path, i);
                instance.m_material = part.m_material;
                instance.m_transform = part.m_transform;

                for (uint32_t slot = 0; slot < kMaterialSlotCount; ++slot)
                {
                    const TextureKey& key = part.m_textureKeys[slot];
                    instance.m_textures[slot] = LoadTexture(key.m_path, key.m_type);
                }

                result.m_instances.push_back(std::move(instance));
            }

            return result;
        }

        const MeshView* AssetManager::GetMesh(ResourceId id) const
        {
            return m_meshViewCache.Get(id);
        }

        const TextureView* AssetManager::GetTexture(ResourceId id) const
        {
            return m_textureViewCache.Get(id);
        }

        size_t AssetManager::GetMeshCount() const
        {
            return m_meshViewCache.Size();
        }

        size_t AssetManager::GetTextureCount() const
        {
            return m_textureViewCache.Size();
        }

        size_t AssetManager::GetModelCount() const
        {
            return m_modelCache.Size();
        }

        ModelAsset::Handle AssetManager::GetOrCreateModel(const std::string& path)
        {
            const std::string key = Core::Path::Normalize(path);

            ModelAsset::Handle handle = m_modelCache.GetOrCreate(key,
                [this](const std::string& modelKey) -> std::optional<ModelAsset>
                {
                    ModelAsset model = AssetUtils::LoadGltf(modelKey);
                    Core::Log::Info("[Resource] Create model asset: ", model.m_name, ", ",
                        model.m_parts.size(), " part(s)");

                    return model;
                });

            return handle;
        }

        MeshView::Handle AssetManager::GetOrCreateMeshView(const ModelAsset::Handle& model,
            const std::string& path, uint32_t partIndex)
        {
            MeshKey key{ Core::Path::Normalize(path), partIndex };

            return m_meshViewCache.GetOrCreate(key,
                [model, partIndex](const MeshKey&) -> std::optional<MeshView>
                {
                    return MeshView(model, partIndex);
                });
        }
    }
}
