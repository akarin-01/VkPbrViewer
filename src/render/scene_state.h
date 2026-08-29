#pragma once

#include "core/macro.h"
#include "rhi/constants.h"
#include "resource/resource_types.h"
#include "resource/resource_id.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>
#include <cstring>

namespace Kita::Pbrv
{
    namespace Render
    {
        /// std140 ABI mirror of the per-object block in per_object.glsl.
        struct PerObjectUbo
        {
            struct Transform
            {
                alignas(16) glm::mat4 m_model{ 1.0f };
                alignas(16) glm::mat4 m_normal{ 1.0f };
            };

            struct Material
            {
                alignas(16) glm::vec4 m_albedo{ 1.0f, 1.0f, 1.0f, 1.0f };
                alignas(16) glm::vec4 m_pbrParams{ 0.0f, 0.0f, 0.0f, 0.0f };   // x - metallic, y - roughness, z - ao, w - padding
                alignas(16) glm::vec4 m_emissive{ 0.0f, 0.0f, 0.0f, 1.0f };    // rgb - color, a - intensity
            };

            Transform m_transform{};
            Material m_material{};
        };
        STD140_ASSERT(PerObjectUbo, 176);

        /// One render-scene entity: per-object set (set 3) + mesh. The empty
        /// state (no mesh, no set) is valid; created by RenderScene::CreateObjectState.
        struct ObjectState
        {
            // Set 3: K UBO slots + K descriptor sets, bound once at creation.
            std::array<Resource::BufferResource::Handle, Rhi::kMaxFramesInFlight> m_ubos{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};
            VkDescriptorSetLayout m_layout{ VK_NULL_HANDLE };

            // Mesh
            Resource::MeshResource::Handle m_mesh{};
            Resource::ResourceId m_lastMeshId{ Resource::kInvalidId };

            bool HasMesh() const { return m_mesh.IsValid(); }
            VkBuffer GetVertexBuffer() const { return m_mesh->GetVertexBuffer(); }
            VkBuffer GetIndexBuffer() const { return m_mesh->GetIndexBuffer(); }
            uint32_t GetIndexCount() const { return m_mesh->GetIndexCount(); }

            void WriteUbo(uint32_t frameIndex, const PerObjectUbo& data)
            {
                std::memcpy(m_ubos[frameIndex]->m_mapped, &data, sizeof(data));
            }

            VkDescriptorSetLayout GetLayout() const { return m_layout; }
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }
        };
    }
}
