#include "resource_manager.h"

#include "core/log.h"
#include "resource/asset_manager.h"
#include "resource/asset_types.h"
#include "resource/resource_utils.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        ResourceManager::ResourceManager(const Rhi::Context& context,
            const AssetManager& assetMgr)
            : m_context(context),
            m_assetMgr(assetMgr),
            m_graveyard(context),
            m_bufferTable([this](BufferResource&& buffer)
                {
                    m_graveyard.PushBuffer(std::move(buffer));
                }),
            m_meshTable([](MeshResource&& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh resource: vb ",
                        mesh.m_vertexBuffer->m_size, " + ib ", mesh.m_indexBuffer->m_size, " bytes");
                })
        {
        }

        ResourceManager::~ResourceManager() = default;

        BufferResource::Handle ResourceManager::CreateBuffer(const BufferDesc& desc, const void* data, size_t size)
        {
            BufferResource buffer = ResourceUtils::CreateBufferResource(m_context, desc, data, size);
            return m_bufferTable.Create(std::move(buffer));
        }

        MeshResource::Handle ResourceManager::GetOrCreateMesh(ResourceId meshId)
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

            MeshResource resource = CreateMeshResource(*asset);

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

        MeshResource ResourceManager::CreateMeshResource(const MeshAsset& asset)
        {
            MeshResource mesh{};

            // Vertex
            {
                BufferDesc desc{};
                desc.m_size = asset.GetVertexDataSize();
                desc.m_usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
                desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                desc.m_mapped = false;

                mesh.m_vertexBuffer = CreateBuffer(desc, asset.GetVertexData(), asset.GetVertexDataSize());
            }

            // Index
            {
                BufferDesc desc{};
                desc.m_size = asset.GetIndexDataSize();
                desc.m_usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
                desc.m_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
                desc.m_mapped = false;

                mesh.m_indexBuffer = CreateBuffer(desc, asset.GetIndexData(), asset.GetIndexDataSize());
            }

            mesh.m_indexCount = static_cast<uint32_t>(asset.GetIndexCount());

            return mesh;
        }
    }
}
