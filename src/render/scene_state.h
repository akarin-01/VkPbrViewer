#pragma once

#include "resource/resource_types.h"
#include "resource/resource_id.h"
#include "resource/set_types.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Render
    {
        /// One render-scene entity: per-object set (set 3) + mesh. The empty
        /// state (no mesh, no set) is valid; created by RenderScene::CreateObjectState.
        struct ObjectState
        {
            // Set
            Resource::PerObjectSet m_set{};

            // Mesh
            Resource::MeshResource::Handle m_mesh{};
            Resource::ResourceId m_lastMeshId{ Resource::kInvalidId };

            bool HasMesh() const { return m_mesh.IsValid(); }
            VkBuffer GetVertexBuffer() const { return m_mesh->GetVertexBuffer(); }
            VkBuffer GetIndexBuffer() const { return m_mesh->GetIndexBuffer(); }
            uint32_t GetIndexCount() const { return m_mesh->GetIndexCount(); }

            void WriteData(uint32_t frameIndex, const Gpu::PerObject& data) { m_set.WriteData(frameIndex, data); }

            VkDescriptorSetLayout GetLayout() const { return m_set.m_layout; }
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_set.GetSet(frameIndex); }
        };
    }
}
