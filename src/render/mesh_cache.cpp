#include "mesh_cache.h"

#include "core/log.h"
#include "render/render_resources.h"
#include "scene/mesh.h"
#include "scene/vertex.h"

#include <cassert>

namespace Kita::Pbrv
{
    MeshCache::MeshCache(RenderResources& resources)
        : m_resources(resources)
    {
    }

    MeshCache::~MeshCache()
    {
        Destroy();
    }

    void MeshCache::Update(const Mesh& sceneMesh)
    {
        if (sceneMesh.IsDirty())
        {
            sceneMesh.ClearDirty();

            // Destroy old mesh
            Destroy();

            if (!sceneMesh.IsEmpty())
            {
                // Create new mesh
                Create(sceneMesh);

                Log::Info("[Renderer] Upload mesh: ", sceneMesh.GetName(), ", ",
                    sceneMesh.GetIndexCount(), " indices");
            }
        }
    }

    void MeshCache::Create(const Mesh& sceneMesh)
    {
        assert(!sceneMesh.IsEmpty() && "MeshCache::Create requires non-empty mesh");

        auto& vertices = sceneMesh.GetVertices();
        auto& indices = sceneMesh.GetIndices();

        // Vertex buffer
        {
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            m_vertexBufferHandle = m_resources.CreateBufferWithData(bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                vertices.data(), static_cast<size_t>(bufferSize));
        }

        // Index buffer
        {
            VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = bufferSize;
            bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            m_indexBufferHandle = m_resources.CreateBufferWithData(bufferInfo, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                indices.data(), static_cast<size_t>(bufferSize));
        }

        m_indexCount = static_cast<uint32_t>(indices.size());
    }

    void MeshCache::Destroy()
    {
        m_resources.DestroyBuffer(m_vertexBufferHandle);
        m_resources.DestroyBuffer(m_indexBufferHandle);

        m_vertexBufferHandle = 0;
        m_indexBufferHandle = 0;
        m_indexCount = 0;
    }
}
