#pragma once

#include "resource/resource_types.h"
#include "resource/resource_id.h"
#include "resource/constants.h"

#include <vulkan/vulkan.h>
#include <array>

namespace Kita::Pbrv
{
    namespace Render
    {
        /// One render-scene entity: material set (set 2) + per-object set
        /// (set 3) + mesh. The empty state (no mesh, fallback material) is
        /// valid; created by RenderScene::CreateObjectState, updated per frame.
        struct ObjectState
        {
            // Set
            Resource::PerMaterialSet::Handle m_materialSet{};
            Resource::PerObjectSet m_objectSet{};
            std::array<Resource::ResourceId, Resource::kMaterialSlotCount> m_lastTextureIds{};

            // Mesh
            Resource::MeshResource::Handle m_mesh{};
            Resource::ResourceId m_lastMeshId{ Resource::kInvalidId };

            bool HasMesh() const { return m_mesh.IsValid(); }
            VkBuffer GetVertexBuffer() const { return m_mesh->GetVertexBuffer(); }
            VkBuffer GetIndexBuffer() const { return m_mesh->GetIndexBuffer(); }
            uint32_t GetIndexCount() const { return m_mesh->GetIndexCount(); }

            void WriteData(uint32_t frameIndex, const Gpu::PerObject& data) { m_objectSet.WriteData(frameIndex, data); }

            VkDescriptorSetLayout GetMaterialLayout() const { return m_materialSet->GetLayout(); }
            const VkDescriptorSet& GetMaterialSet() const { return m_materialSet->GetSet(); }
            VkDescriptorSetLayout GetObjectLayout() const { return m_objectSet.GetLayout(); }
            const VkDescriptorSet& GetObjectSet(uint32_t frameIndex) const { return m_objectSet.GetSet(frameIndex); }
        };
    }
}
