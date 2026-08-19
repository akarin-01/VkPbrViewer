#include "render_mesh_data.h"

#include "core/log.h"
#include "render/render_resources.h"
#include "scene/mesh.h"
#include "scene/vertex.h"

#include <cassert>
#include <cstddef>

namespace Kita::Pbrv
{
    VkVertexInputBindingDescription RenderMeshData::GetVertexBinding()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    std::vector<VkVertexInputAttributeDescription> RenderMeshData::GetVertexAttributes()
    {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, tangent);

        return attributeDescriptions;
    }

    RenderMeshData::RenderMeshData(RenderResources& resources)
        : m_resources(resources)
    {
    }

    RenderMeshData::~RenderMeshData()
    {
        Destroy();
    }

    void RenderMeshData::Update(const Mesh& sceneMesh)
    {
        if (sceneMesh.GetRevision() != m_lastSyncedRevision)
        {
            m_lastSyncedRevision = sceneMesh.GetRevision();

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

    VkBuffer RenderMeshData::GetVertexBuffer() const
    {
        RenderBuffer* buffer = m_resources.GetBuffer(m_vertexBufferHandle);
        assert(buffer && "Vertex buffer handle is invalid");

        return buffer->m_buffer;
    }

    VkBuffer RenderMeshData::GetIndexBuffer() const
    {
        RenderBuffer* buffer = m_resources.GetBuffer(m_indexBufferHandle);
        assert(buffer && "Index buffer handle is invalid");

        return buffer->m_buffer;
    }

    void RenderMeshData::Create(const Mesh& sceneMesh)
    {
        assert(!sceneMesh.IsEmpty() && "RenderMeshData::Create requires non-empty mesh");

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

    void RenderMeshData::Destroy()
    {
        m_resources.DestroyBuffer(m_vertexBufferHandle);
        m_resources.DestroyBuffer(m_indexBufferHandle);

        m_vertexBufferHandle = 0;
        m_indexBufferHandle = 0;
        m_indexCount = 0;
    }
}
