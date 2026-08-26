#pragma once

#include "resource/types.h"

#include <vector>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Resource
    {
        class RenderResources;
    }
    namespace Scene
    {
        class Mesh;
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

            void Update(const Scene::Mesh& sceneMesh);

            VkBuffer GetVertexBuffer() const;
            VkBuffer GetIndexBuffer() const;
            uint32_t GetIndexCount() const { return m_indexCount; }
            bool IsEmpty() const { return m_vertexBufferHandle == 0; }

        private:
            void Create(const Scene::Mesh& sceneMesh);
            void Destroy();

        private:
            Resource::RenderResources& m_resources;

            Resource::RenderBufferHandle m_vertexBufferHandle{ 0 };
            Resource::RenderBufferHandle m_indexBufferHandle{ 0 };
            uint32_t m_indexCount{ 0 };

            uint64_t m_lastSyncedRevision{ UINT64_MAX };
        };
    }
}
