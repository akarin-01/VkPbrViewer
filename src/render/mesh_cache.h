#pragma once

#include "render/render_resource_types.h"

namespace Kita::Pbrv
{
    class RenderResources;
    class Mesh;

    /// GPU resources for a mesh (vertex/index buffers).
    /// Rebuilds the buffers when the CPU-side Mesh data changes (dirty).
    class MeshCache
    {
    public:
        explicit MeshCache(RenderResources& resources);
        ~MeshCache();

        void Update(const Mesh& sceneMesh);

        RenderBufferHandle GetVertexBufferHandle() const { return m_vertexBufferHandle; }
        RenderBufferHandle GetIndexBufferHandle() const { return m_indexBufferHandle; }
        uint32_t GetIndexCount() const { return m_indexCount; }

    private:
        void Create(const Mesh& sceneMesh);
        void Destroy();

        RenderResources& m_resources;

        RenderBufferHandle m_vertexBufferHandle{ 0 };
        RenderBufferHandle m_indexBufferHandle{ 0 };
        uint32_t m_indexCount{ 0 };
    };
}
