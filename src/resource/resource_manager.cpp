#include "resource_manager.h"

#include "core/log.h"
#include "resource/asset_manager.h"
#include "resource/asset_types.h"
#include "resource/resource_utils.h"

#include <stdexcept>

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

            void DestroyMeshResource(const Rhi::Context& context, MeshResource& mesh)
            {
                ResourceUtils::DestroyBufferData(context, mesh.m_vertexBuffer);
                ResourceUtils::DestroyBufferData(context, mesh.m_indexBuffer);
                mesh = {};
            }
        }

        ResourceManager::ResourceManager(const Rhi::Context& context,
            AssetManager& assetMgr)
            : m_context(context),
            m_assetMgr(assetMgr),
            m_meshTable([this](const ResourceId& id)
                {
                    const MeshAsset* mesh = m_assetMgr.GetMesh(id);
                    if (!mesh)
                    {
                        throw std::runtime_error("MeshAsset asset not found: " + std::to_string(id));
                    }

                    MeshResource data = CreateMeshResource(m_context, *mesh);
                    Core::Log::Info("[Resource] Create mesh resource: ", mesh->m_name, ", vb ",
                        mesh->GetVertexDataSize(), " bytes, ib ", mesh->GetIndexDataSize(), " bytes");
                    return data;
                },
                [this](const ResourceId& id)
                {
                    KITA_LOG_DEBUG("[Resource] Reuse mesh resource: ", m_assetMgr.GetMesh(id)->m_name);
                },
                [this](MeshResource& mesh)
                {
                    Core::Log::Info("[Resource] Release mesh resource: ",
                        mesh.m_vertexBuffer.m_size, " + ", mesh.m_indexBuffer.m_size, " bytes");
                    DestroyMeshResource(m_context, mesh);
                })
        {
        }

        ResourceManager::~ResourceManager() = default;

        MeshResource::Handle ResourceManager::GetOrCreateMeshResource(ResourceId meshId)
        {
            return m_meshTable.GetOrCreate(meshId);
        }

        void ResourceManager::FlushDeferred(uint32_t frameIndex)
        {
            m_meshTable.FlushDeferred(frameIndex);
        }
    }
}
