#pragma once

#include "scene/camera.h"
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

        const Mesh& GetMesh() const { return m_mesh; }
        Mesh& GetMesh() { return m_mesh; }

    private:
        Camera m_camera;
        Mesh m_mesh;
    };
}