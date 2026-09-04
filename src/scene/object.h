#pragma once

#include "resource/asset_types.h"
#include "resource/resource_id.h"
#include "scene/material.h"

#include <glm/glm.hpp>

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Object
        {
        public:
            explicit Object(Resource::ResourceId id)
                : m_id(id)
            {
            }

            ~Object() = default;

            /// Writes the per-frame object data and any pending mesh/texture
            /// changes into the SceneProxy
            void Update();

            Resource::ResourceId GetId() const { return m_id; }
            void MarkDelete() { m_deletePending = true; }
            bool IsDeletePending() const { return m_deletePending; }

            Object& SetMesh(Resource::MeshView::Handle mesh);

            Object& SetPosition(const glm::vec3& position);
            Object& SetRotation(const glm::vec3& rotation);
            Object& SetScale(const glm::vec3& scale);
            Object& SetTransform(const Resource::Transform& transform);

            const Resource::MeshView::Handle& GetMesh() const { return m_mesh; }

            glm::vec3 GetPosition() const { return m_transform.m_position; }
            glm::vec3 GetRotation() const { return m_transform.m_rotation; }
            glm::vec3 GetScale() const { return m_transform.m_scale; }
            const Resource::Transform& GetTransform() const { return m_transform; }
            Resource::Transform& GetTransform() { return m_transform; }

            const Material& GetMaterial() const { return m_material; }
            Material& GetMaterial() { return const_cast<Material&>(static_cast<const Object*>(this)->GetMaterial()); }

        private:
            Resource::ResourceId m_id{ Resource::kInvalidId };
            bool m_deletePending{ false };

            Resource::MeshView::Handle m_mesh{};
            bool m_meshDirty{ true };

            Resource::Transform m_transform{};

            Material m_material{};
        };
    }
}
