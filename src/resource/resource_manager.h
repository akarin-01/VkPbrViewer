#pragma once

#include "resource/cache_table.h"
#include "resource/resource_id.h"
#include "resource/resource_types.h"

#include <array>
#include <cstdint>
#include <memory>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        class AssetManager;
        struct BakedTexture;
        class DescriptorManager;
        class EnvironmentBaker;
        class Graveyard;
        struct MeshAsset;
        struct TextureAsset;

        class ResourceManager
        {
        public:
            ResourceManager(const Rhi::Context& context,
                const AssetManager& assetMgr);
            ~ResourceManager();

            VkDescriptorSetLayout GetDescriptorSetLayout(DescriptorSetRhi::Type type) const
            {
                return m_descriptorMgr->GetLayout(type);
            }

            // ---- Rhi resources ----
            BufferRhi::Handle CreateBuffer(const BufferDesc& desc,
                const void* data = nullptr, size_t size = 0);
            ImageRhi::Handle CreateImage(const ImageDesc& desc,
                const void* data = nullptr, size_t size = 0);
            ImageRhi::Handle GetOrCreateImage(ResourceId textureId);
            ImageViewRhi::Handle CreateImageView(const ImageViewDesc& desc,
                ImageRhi::Handle image);
            SamplerRhi::Handle GetOrCreateSampler(const SamplerDesc& desc);
            DescriptorSetRhi::Handle CreateDescriptorSet(DescriptorSetRhi::Type type);

            // ---- Render resources ----
            TextureResource CreateTexture(ResourceId textureId,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc);
            TextureResource CreateTexture(const ImageDesc& imageDesc,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc,
                const void* data = nullptr, size_t size = 0);
            TextureResource CreateTexture(ImageRhi::Handle image,
                const ImageViewDesc& imageViewDesc, const SamplerDesc& samplerDesc);
            MeshResource::Handle GetOrCreateMesh(ResourceId meshId);
            TargetResource CreateTarget(const TargetDesc& desc);

            // ---- Sets ----
            PerObjectSet CreatePerObjectSet();
            PerMaterialSet::Handle GetOrCreatePerMaterialSet(const MaterialDesc& desc);
            PostProcessSet CreatePostProcessSet(const TextureResource& texture);
            PerFrameSet CreatePerFrameSet(ResourceId equirectId);

            void FlushGraveyard();

        private:
            ImageRhi CreateImage(const TextureAsset& asset);
            std::array<ImageRhi::Handle, kMaterialSlotCount> CreateMaterialFallbacks();
            ImageRhi::Handle CreateCubemapFallback();

            UboResource CreateUbo(const BufferDesc& desc);
            MeshResource CreateMesh(const MeshAsset& asset);
            TextureResource CreateEquirect(const TextureAsset& asset);
            TextureResource CreateTexture(BakedTexture baked);

            PerMaterialSet CreatePerMaterialSet(const MaterialDesc& desc);

        private:
            const Rhi::Context& m_context;
            const AssetManager& m_assetMgr;

            // The graveyard outlives the tables (their destroyers push into it),
            // and the descriptor manager outlives the graveyard (its flush recycles).
            std::unique_ptr<DescriptorManager> m_descriptorMgr{};
            std::unique_ptr<Graveyard> m_graveyard{};

            // ------------- L0: Rhi (vk objects) -------------
            HandleTable<BufferRhi> m_bufferTable;
            HandleTable<DescriptorSetRhi> m_descriptorSetTable;
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
            ImageRhi::Handle m_cubemapFallback{};
            TextureResource m_brdfLut{};

            // IBL conversions: cubemap / irradiance / prefilter baking and the brdf lut.
            std::unique_ptr<EnvironmentBaker> m_environmentBaker{};
        };
    }
}
