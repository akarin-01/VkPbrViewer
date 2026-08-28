#pragma once

#include "resource/asset_types.h"
#include "scene/material.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Object
        {
        public:
            Object() = default;
            ~Object() = default;

            void SetMesh(Resource::MeshAsset::Handle mesh)
            {
                m_mesh = std::move(mesh);
            }
            Resource::MeshAsset::Handle GetMesh() const { return m_mesh; }

            const Material& GetMaterial() const { return m_material; }
            Material& GetMaterial() { return const_cast<Material&>(static_cast<const Object*>(this)->GetMaterial()); }

        private:
            Resource::MeshAsset::Handle m_mesh{};
            Material m_material{};
        };
    }
}
