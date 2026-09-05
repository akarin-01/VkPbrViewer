#include "scene/utils.h"

#include "resource/asset_manager.h"
#include "resource/asset_types.h"
#include "resource/constants.h"
#include "scene/scene.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        namespace Utils
        {
            void SpawnModel(Scene& scene, Resource::AssetManager& assets, const std::string& path)
            {
                auto result = assets.LoadModel(path);

                for (const auto& instance : result.m_instances)
                {
                    Object& object = scene.CreateObject()
                        .SetName(instance.m_name)
                        .SetMesh(instance.m_mesh)
                        .SetTransform(instance.m_transform);

                    Material& material = object.GetMaterial()
                        .SetParams(instance.m_material);

                    // Missing textures stay unset and fall back on the GPU side
                    for (uint32_t slot = 0; slot < Resource::kMaterialSlotCount; ++slot)
                    {
                        const auto& texture = instance.m_textures[slot];
                        if (texture.IsValid())
                        {
                            material.SetTexture(static_cast<Resource::MaterialSlot>(slot), texture);
                        }
                    }
                }
            }
        }
    }
}
