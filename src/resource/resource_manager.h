#pragma once

#include "resource/cache_table.h"
#include "resource/resource_types.h"
#include "resource/resource_id.h"
#include "resource/graveyard.h"

#include <array>
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

            TextureResource CreateTexture(ResourceId textureId,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc);
            TextureResource CreateTexture(const ImageDesc& imageDesc,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc,
                const void* data = nullptr, size_t size = 0);
            TextureResource CreateTexture(ImageRhi::Handle image,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc);
            MeshResource::Handle GetOrCreateMesh(ResourceId meshId);
            TargetResource CreateTarget(const TargetDesc& desc);

            PerObjectSet CreatePerObjectSet();
            PerMaterialSet::Handle GetOrCreatePerMaterialSet(const MaterialDesc& desc);
            PostProcessSet CreatePostProcessSet(const TextureResource& texture);

            void FlushGraveyard();

        private:
            ImageRhi CreateImage(const TextureAsset& asset);

            UboResource CreateUbo(const BufferDesc& desc);
            MeshResource CreateMesh(const MeshAsset& asset);

            PerMaterialSet CreatePerMaterialSet(const MaterialDesc& desc);

        private:
            const Rhi::Context& m_context;
            const AssetManager& m_assetMgr;
            DescriptorManager& m_descriptorMgr;

            Graveyard m_graveyard;      // Graveyard must be destroyed after table

            // ------------- L0: Rhi (vk objects) -------------
            HandleTable<BufferRhi> m_bufferTable;
            CacheTable<ImageRhi, ResourceId> m_imageCache;   // asset path cached, desc path direct
            HandleTable<ImageViewRhi> m_imageViewTable;
            CacheTable<SamplerRhi, SamplerDesc, SamplerDesc::Hash> m_samplerCache;

            // ------------- L1: Resource (render resources) -------------
            CacheTable<MeshResource, ResourceId> m_meshCache;

            // ------------- L2: Set (descriptor sets) -------------
            CacheTable<PerMaterialSet, MaterialDesc, MaterialDesc::Hash> m_materialCache;

            // Fallback images per material slot (1x1), shared by empty slots.
            // Declared last: they release into the tables first on teardown.
            std::array<ImageRhi::Handle, kMaterialSlotCount> m_fallbacks{};
        };
    }
}
