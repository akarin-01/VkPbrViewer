#pragma once

#include "resource/handle_table.h"
#include "resource/resource_types.h"
#include "resource/resource_id.h"
#include "resource/graveyard.h"

#include <array>
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

            BufferRhi::Handle CreateBuffer(const BufferDesc& desc,
                const void* data = nullptr, size_t size = 0);
            ImageRhi::Handle CreateImage(const ImageDesc& desc,
                const void* data = nullptr, size_t size = 0);
            ImageRhi::Handle GetOrCreateImage(ResourceId textureId);
            ImageViewRhi::Handle CreateImageView(const ImageViewDesc& desc,
                ImageRhi::Handle image);
            SamplerRhi::Handle GetOrCreateSampler(const SamplerDesc& desc);

            MeshResource::Handle GetOrCreateMesh(ResourceId meshId);
            TextureResource CreateTexture(ResourceId textureId,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc);
            TextureResource CreateTexture(const ImageDesc& imageDesc,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc,
                const void* data = nullptr, size_t size = 0);
            TextureResource CreateTexture(ImageRhi::Handle image,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc);

            PerObjectSet CreatePerObjectSet();
            PerMaterialSet::Handle GetOrCreatePerMaterialSet(const MaterialDesc& desc);

            void FlushGraveyard();

        private:
            MeshResource::Handle CreateMesh(const MeshAsset& asset);
            ImageRhi::Handle CreateImage(const TextureAsset& asset);
            UboResource CreateUbo(const BufferDesc& desc);
            PerMaterialSet::Handle CreatePerMaterialSet(const MaterialDesc& desc);

        private:
            const Rhi::Context& m_context;
            const AssetManager& m_assetMgr;
            DescriptorManager& m_descriptorMgr;

            Graveyard m_graveyard;      // Graveyard must be destroyed after table

            // ------------- Unique resources (no dedup) ----------------
            HandleTable<BufferRhi> m_bufferTable;
            HandleTable<ImageRhi> m_imageTable;
            HandleTable<ImageViewRhi> m_imageViewTable;

            // ------------- Cached resources (dedup) ----------------
            std::unordered_map<ResourceId, ResourceId> m_imageIds;      // Part of image table

            HandleTable<SamplerRhi> m_samplerTable;
            std::unordered_map<SamplerDesc, ResourceId, SamplerDesc::Hash> m_samplerIds;

            HandleTable<MeshResource> m_meshTable;
            std::unordered_map<ResourceId, ResourceId> m_meshIds;

            HandleTable<PerMaterialSet> m_materialTable;
            std::unordered_map<MaterialDesc, ResourceId, MaterialDesc::Hash> m_materialIds;

            // Fallback images per material slot (1x1); empty slots assemble a
            // fresh view + shared sampler over these.
            std::array<ImageRhi::Handle, kMaterialSlotCount> m_fallbacks{};
        };
    }
}
