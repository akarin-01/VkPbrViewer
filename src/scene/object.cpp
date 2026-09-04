#include "object.h"

#include "core/math.h"
#include "resource/constants.h"
#include "render/scene_proxy.h"

#include <array>
#include <utility>

namespace Kita::Pbrv
{
    namespace Scene
    {
        void Object::Update()
        {
            Render::SceneProxy& proxy = Render::SceneProxy::Get();

            // Change records: written once when a handle changes
            if (m_meshDirty)
            {
                m_meshDirty = false;
                proxy.UpdateMesh(m_id, m_mesh.GetId());   // invalid view -> kInvalidId = remove mesh
            }
            if (m_material.ConsumeTexturesDirty())
            {
                std::array<Resource::ResourceId, Resource::kMaterialSlotCount> textureIds{};
                for (size_t i = 0; i < textureIds.size(); ++i)
                {
                    textureIds[i] = m_material.GetTexture(
                        static_cast<Resource::MaterialSlot>(i)).GetId();
                }
                proxy.UpdateMaterial(m_id, textureIds);
            }

            // Per-frame data
            proxy.WriteObjectData(m_id, m_transform.m_position, m_transform.m_rotation, m_transform.m_scale);
            proxy.WriteObjectMaterial(m_id, m_material.GetAlbedo(), m_material.GetMetallic(),
                m_material.GetRoughness(), m_material.GetAO(), m_material.GetEmissive(),
                m_material.GetEmissiveIntensity());
        }

        Object& Object::SetMesh(Resource::MeshView::Handle mesh)
        {
            if (mesh.GetId() == m_mesh.GetId())
            {
                return *this;   // duplicate set
            }
            m_mesh = std::move(mesh);
            m_meshDirty = true;
            return *this;
        }

        Object& Object::SetName(std::string name)
        {
            m_name = std::move(name);
            return *this;
        }

        Object& Object::SetPosition(const glm::vec3& position)
        {
            m_transform.m_position = position;
            return *this;
        }

        Object& Object::SetRotation(const glm::vec3& rotation)
        {
            m_transform.m_rotation.x = Core::WrapDegrees(rotation.x);
            m_transform.m_rotation.y = Core::WrapDegrees(rotation.y);
            m_transform.m_rotation.z = Core::WrapDegrees(rotation.z);
            return *this;
        }

        Object& Object::SetScale(const glm::vec3& scale)
        {
            m_transform.m_scale = scale;
            return *this;
        }

        Object& Object::SetTransform(const Resource::Transform& transform)
        {
            m_transform = transform;
            return *this;
        }
    }
}
