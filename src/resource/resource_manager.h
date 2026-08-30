#pragma once

#include "resource/handle_table.h"
#include "resource/resource_types.h"
#include "resource/resource_id.h"
#include "resource/set_types.h"
#include "resource/graveyard.h"

#include <unordered_map>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        class AssetManager;
        class DescriptorManager;

        struct MeshAsset;

        class ResourceManager
        {
        public:
            ResourceManager(const Rhi::Context& context,
                const AssetManager& assetMgr,
                DescriptorManager& descriptorMgr);
            ~ResourceManager();

            BufferResource::Handle CreateBuffer(const BufferDesc& desc,
                const void* data = nullptr, size_t size = 0);
            ImageResource::Handle CreateImage(const ImageDesc& desc,
                const void* data = nullptr, size_t size = 0);
            ImageViewResource::Handle CreateImageView(const ImageViewDesc& desc,
                const ImageResource::Handle& image);

            MeshResource::Handle GetOrCreateMesh(ResourceId meshId);

            PerObjectSet CreatePerObjectSet();

            void FlushGraveyard();

        private:
            MeshResource CreateMeshResource(const MeshAsset& asset);

        private:
            const Rhi::Context& m_context;
            const AssetManager& m_assetMgr;
            DescriptorManager& m_descriptorMgr;

            Graveyard m_graveyard;      // Graveyard must be destroyed after table

            // ------------- Vk handle ----------------
            HandleTable<BufferResource> m_bufferTable;
            HandleTable<ImageResource> m_imageTable;
            HandleTable<ImageViewResource> m_imageViewTable;

            // ------------- Cache --------------------
            HandleTable<MeshResource> m_meshTable;
            std::unordered_map<ResourceId, ResourceId> m_meshIds;
        };
    }
}
