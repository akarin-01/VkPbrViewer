#include "resource_manager.h"

#include "core/log.h"
#include "resource/asset_manager.h"
#include "resource/asset_types.h"
#include "resource/resource_utils.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace
        {
            MeshResource CreateMeshResource(const Rhi::Context& context, const MeshAsset& mesh)
            {
                MeshResource data{};

                size_t vertexDataSize = mesh.GetVertexDataSize();
                data.m_vertexBuffer = ResourceUtils::CreateBufferData(context,
                    static_cast<VkDeviceSize>(vertexDataSize), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    mesh.m_vertices.data(), vertexDataSize);

                size_t indexDataSize = mesh.GetIndexDataSize();
                data.m_indexBuffer = ResourceUtils::CreateBufferData(context,
                    static_cast<VkDeviceSize>(indexDataSize), VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                    mesh.m_indices.data(), indexDataSize);

                size_t indexCount = mesh.GetIndexCount();
                data.m_indexCount = static_cast<uint32_t>(indexCount);
                return data;
            }
        }

        ResourceManager::ResourceManager(const Rhi::Context& context,
            const AssetManager& assetMgr)
            : m_context(context),
            m_assetMgr(assetMgr),
            m_graveyard(context),
            m_meshTable([this](MeshResource&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh resource: vb ",
                        mesh.m_vertexBuffer.m_size, " + ib ", mesh.m_indexBuffer.m_size, " bytes");
                    m_graveyard.PushBuffer(std::move(mesh.m_vertexBuffer));
                    m_graveyard.PushBuffer(std::move(mesh.m_indexBuffer));
                    mesh = {};
                })
        {
        }

        ResourceManager::~ResourceManager() = default;

        MeshResource::Handle ResourceManager::GetOrCreateMeshResource(ResourceId meshId)
        {
            auto it = m_meshIds.find(meshId);
            if (it != m_meshIds.end())
            {
                const ResourceId id = it->second;
                // The mapping can outlive its entry (every handle released):
                // a stale id falls through and is rebuilt below.
                if (m_meshTable.Has(id))
                {
                    // Cache hit: add ref
                    KITA_LOG_DEBUG("[Resource] Reuse mesh resource: mesh asset(", meshId, ")");
                    return m_meshTable.GetShared(id);
                }
            }

            // Cache miss: create
            auto asset = m_assetMgr.GetMesh(meshId);
            if (!asset)
            {
                // Invalid mesh id, return invalid handle
                return MeshResource::Handle();
            }

            MeshResource resource = CreateMeshResource(m_context, *asset);

            KITA_LOG_DEBUG("[Resource] Create mesh resource: mesh asset(", meshId, ")");
            Core::Log::Info("[Resource] Create mesh resource: ", asset->m_name, ", vb ",
                asset->GetVertexDataSize(), " bytes, ib ", asset->GetIndexDataSize(), " bytes");

            MeshResource::Handle handle = m_meshTable.Create(std::move(resource));
            m_meshIds[meshId] = handle.GetId();
            return handle;
        }

        void ResourceManager::FlushGraveyard()
        {
            m_graveyard.Flush();
        }
    }
}
