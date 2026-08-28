#include "render_mesh_data.h"
#include "core/log.h"
#include "resource/handle.h"
#include "resource/asset_types.h"
#include "resource/resources.h"
#include "resource/vertex.h"

#include <cassert>
#include <cstddef>

namespace Kita::Pbrv
{
    namespace Render
    {
        VkVertexInputBindingDescription RenderMeshData::GetVertexBinding()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(Resource::Vertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        std::vector<VkVertexInputAttributeDescription> RenderMeshData::GetVertexAttributes()
        {
            std::vector<VkVertexInputAttributeDescription> attributeDescriptions(4);

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(Resource::Vertex, position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(Resource::Vertex, normal);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(Resource::Vertex, texCoord);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
            attributeDescriptions[3].offset = offsetof(Resource::Vertex, tangent);

            return attributeDescriptions;
        }

        RenderMeshData::RenderMeshData(Resource::Resources& resources)
            : m_resources(resources)
        {
        }

        RenderMeshData::~RenderMeshData()
        {
            Destroy();
        }

        void RenderMeshData::Update(const Resource::MeshAsset::Handle& meshHandle)
        {
            if (meshHandle.GetId() != m_lastMeshId)
            {
                m_lastMeshId = meshHandle.GetId();

                // Destroy old mesh
                Destroy();

                if (meshHandle.IsValid())
                {
                    // Create new mesh
                    Create(*meshHandle);

                    Core::Log::Info("[Renderer] Upload mesh: ", meshHandle->m_name, ", ",
                        meshHandle->GetIndexCount(), " indices");
                }
            }
        }

        VkBuffer RenderMeshData::GetVertexBuffer() const
        {
            Resource::RenderBuffer* buffer = m_resources.GetBuffer(m_vertexBufferHandle);
            assert(buffer && "Vertex buffer handle is invalid");

            return buffer->m_buffer;
        }

        VkBuffer RenderMeshData::GetIndexBuffer() const
        {
            Resource::RenderBuffer* buffer = m_resources.GetBuffer(m_indexBufferHandle);
            assert(buffer && "Index buffer handle is invalid");

            return buffer->m_buffer;
        }

        void RenderMeshData::Create(const Resource::MeshAsset& mesh)
        {
            const auto& vertices = mesh.m_vertices;
            const auto& indices = mesh.m_indices;

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
}
