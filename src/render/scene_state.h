#pragma once

#include "resource/constants.h"
#include "resource/resource_id.h"
#include "resource/resource_types.h"

#include <array>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Render
    {
        /// Scene-level render state: plain container of the scene's GPU
        /// resources; passes reach members directly.
        struct GlobalState
        {
            Resource::TargetResource m_target{};

            Resource::PerFrameSet m_frameSet{};
            Resource::PostProcessSet m_postProcessSet{};

            Resource::ResourceId m_lastEquirectId{ Resource::kInvalidId };
        };

        /// One render-scene entity: material set (set 2) + per-object set
        /// (set 3) + mesh; plain container, updated per frame.
        struct ObjectState
        {
            Resource::PerMaterialSet::Handle m_materialSet{};
            Resource::PerObjectSet m_objectSet{};
            std::array<Resource::ResourceId, Resource::kMaterialSlotCount> m_lastTextureIds{};

            Resource::MeshResource::Handle m_mesh{};
            Resource::ResourceId m_lastMeshId{ Resource::kInvalidId };
        };
    }
}
