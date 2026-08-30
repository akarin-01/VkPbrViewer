#include "resource_manager.h"

#include "core/log.h"
#include "rhi/context.h"
#include "rhi/descriptor_writer.h"
#include "resource/asset_manager.h"
#include "resource/asset_types.h"
#include "resource/resource_utils.h"
#include "resource/gpu_layouts.h"
#include "resource/descriptor_manager.h"

namespace Kita::Pbrv
{
    namespace Resource
    {
        ResourceManager::ResourceManager(const Rhi::Context& context,
            const AssetManager& assetMgr,
            DescriptorManager& descriptorMgr)
            : m_context(context),
            m_assetMgr(assetMgr),
            m_descriptorMgr(descriptorMgr),
            m_graveyard(context),
            m_bufferTable([this](BufferResource&& buffer)
                {
                    m_graveyard.PushBuffer(std::move(buffer));
                }),
            m_imageTable([this](ImageResource&& image)
                {
                    m_graveyard.PushImage(std::move(image));
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

        ImageResource::Handle ResourceManager::CreateImage(const ImageDesc& desc, const void* data, size_t size)
        {
            ImageResource image = ResourceUtils::CreateImageResource(m_context, desc, data, size);
            return m_imageTable.Create(std::move(image));
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

        PerObjectSet ResourceManager::CreatePerObjectSet()
        {
            constexpr DescriptorLayoutType kLayoutType = DescriptorLayoutType::PerObject;

            PerObjectSet perObject{};
            perObject.m_layout = m_descriptorMgr.GetLayout(kLayoutType);

            Resource::BufferDesc desc{};
            desc.m_size = sizeof(Gpu::PerObject);
            desc.m_usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            desc.m_properties =
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            desc.m_mapped = true;

            for (size_t i = 0; i < perObject.m_ubos.size(); ++i)
            {
                perObject.m_ubos[i] = CreateBuffer(desc);
                perObject.m_sets[i] = m_descriptorMgr.Allocate(kLayoutType);

                Rhi::V2::DescriptorWriter writer(m_context.Device());
                writer.WriteBuffer(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    perObject.m_ubos[i]->m_buffer, 0, sizeof(Gpu::PerObject))
                    .UpdateSet(perObject.m_sets[i]);
            }

            return perObject;
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
