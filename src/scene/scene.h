#pragma once

#include "scene/camera.h"
#include "scene/light.h"
#include "scene/material.h"
#include "scene/mesh.h"

namespace Kita::Pbrv
{
    class Mesh;

    /// @brief Only supports one model for now.
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        const Camera& GetCamera() const { return m_camera; }
        Camera& GetCamera() { return m_camera; }

        const Light& GetLight() const { return m_light; }
        Light& GetLight() { return m_light; }

        const Material& GetMaterial() const { return m_material; }
        Material& GetMaterial() { return m_material; }

        const Mesh& GetMesh() const { return m_mesh; }
        Mesh& GetMesh() { return m_mesh; }

    private:
        Camera m_camera;
        Light m_light;
        Material m_material;
        Mesh m_mesh;
    };
}