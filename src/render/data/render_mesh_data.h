#pragma once

#include "resource/mesh.h"
#include "resource/types.h"

#include <vector>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Resource
    {
        class RenderResources;
    }

    namespace Render
    {
        class RenderMeshData
        {
        public:
            static VkVertexInputBindingDescription GetVertexBinding();
            static std::vector<VkVertexInputAttributeDescription> GetVertexAttributes();

            explicit RenderMeshData(Resource::RenderResources& resources);
            ~RenderMeshData();

            void Update(const Resource::Mesh::Handle& meshHandle);

            VkBuffer GetVertexBuffer() const;
            VkBuffer GetIndexBuffer() const;
            uint32_t GetIndexCount() const { return m_indexCount; }
            bool IsEmpty() const { return m_vertexBufferHandle == 0; }

        private:
            void Create(const Resource::Mesh& mesh);
            void Destroy();

        private:
            Resource::RenderResources& m_resources;

            Resource::RenderBufferHandle m_vertexBufferHandle{ 0 };
            Resource::RenderBufferHandle m_indexBufferHandle{ 0 };
            uint32_t m_indexCount{ 0 };

            Resource::ResourceId m_lastMeshId{ Resource::kInvalidId };
        };
    }
}
