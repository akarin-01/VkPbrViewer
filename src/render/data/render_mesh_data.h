#pragma once

#include "render/render_resource_types.h"

#include <vector>
#include <cstdint>

namespace Kita::Pbrv
{
    class RenderResources;
    class Mesh;

    class RenderMeshData
    {
    public:
        static VkVertexInputBindingDescription GetVertexBinding();
        static std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();

        explicit RenderMeshData(RenderResources& resources);
        ~RenderMeshData();

        void Update(const Mesh& sceneMesh);

        RenderBufferHandle GetVertexBufferHandle() const { return m_vertexBufferHandle; }
        RenderBufferHandle GetIndexBufferHandle() const { return m_indexBufferHandle; }
        uint32_t GetIndexCount() const { return m_indexCount; }

    private:
        void Create(const Mesh& sceneMesh);
        void Destroy();

    private:
        RenderResources& m_resources;

        RenderBufferHandle m_vertexBufferHandle{ 0 };
        RenderBufferHandle m_indexBufferHandle{ 0 };
        uint32_t m_indexCount{ 0 };

        uint64_t m_lastSyncedRevision{ UINT64_MAX };
    };
}
