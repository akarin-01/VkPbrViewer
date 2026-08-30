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
        struct TextureAsset;

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
            ImageResource::Handle GetOrCreateImage(ResourceId textureId);
            ImageViewResource::Handle CreateImageView(const ImageViewDesc& desc,
                const ImageResource::Handle& image);
            SamplerResource::Handle GetOrCreateSampler(const SamplerDesc& desc);

            MeshResource::Handle GetOrCreateMesh(ResourceId meshId);

            PerObjectSet CreatePerObjectSet();

            void FlushGraveyard();

        private:
            MeshResource::Handle CreateMesh(const MeshAsset& asset);
            ImageResource::Handle CreateImage(const TextureAsset& asset);

        private:
            const Rhi::Context& m_context;
            const AssetManager& m_assetMgr;
            DescriptorManager& m_descriptorMgr;

            Graveyard m_graveyard;      // Graveyard must be destroyed after table

            // ------------- Unique resources (no dedup) ----------------
            HandleTable<BufferResource> m_bufferTable;
            HandleTable<ImageResource> m_imageTable;
            HandleTable<ImageViewResource> m_imageViewTable;

            // ------------- Cached resources (dedup) ----------------
            HandleTable<MeshResource> m_meshTable;
            std::unordered_map<ResourceId, ResourceId> m_meshIds;
            HandleTable<SamplerResource> m_samplerTable;
            std::unordered_map<SamplerDesc, ResourceId, SamplerDesc::Hash> m_samplerIds;
            std::unordered_map<ResourceId, ResourceId> m_imageIds;
        };
    }
}
