#pragma once

#include "resource/mesh.h"
#include "scene/material.h"

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Object
        {
        public:
            using MeshHandle = Resource::Mesh::Handle;

            Object() = default;
            ~Object() = default;

            void SetMesh(MeshHandle mesh)
            {
                m_mesh = std::move(mesh);
            }
            MeshHandle GetMesh() const { return m_mesh; }

            const Material& GetMaterial() const { return m_material; }
            Material& GetMaterial() { return const_cast<Material&>(static_cast<const Object*>(this)->GetMaterial()); }

        private:
            MeshHandle m_mesh{};
            Material m_material{};
        };
    }
}
