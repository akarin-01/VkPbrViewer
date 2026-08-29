#pragma once

#include "core/macro.h"
#include "rhi/constants.h"
#include "resource/constants.h"
#include "resource/resource_id.h"
#include "resource/render_texture.h"

#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Scene
    {
        class Material;
    }
    namespace Rhi
    {
        class Context;
    }
    namespace Resource
    {
        class Resources;
        class DescriptorManager;
        struct TextureAsset;
    }

    namespace Render
    {
        /// Set 2 — per-material textures (named bindings), one instance per
        /// material. Textures rotate across the K set instances on change.
        class RenderMaterialData
        {
        public:
            RenderMaterialData(const Rhi::Context& context,
                Resource::Resources& resources,
                Resource::DescriptorManager& descriptorMgr);
            ~RenderMaterialData();

            void UpdateTextures(const Scene::Material& sceneMat);
            void RefreshSet(uint32_t frameIndex);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

        private:
            Resource::RenderTexture CreateFallback(Resource::MaterialTextureSlot slot) const;
            Resource::RenderTexture CreateTexture(const Resource::TextureAsset& texture) const;
            void DestroyTextureSafe(uint32_t slot);
            void WriteSet(uint32_t frameIndex) const;

        private:
            const Rhi::Context& m_context;
            Resource::Resources& m_resources;
            Resource::DescriptorManager& m_descriptorMgr;

            std::array<Resource::RenderTexture, Resource::kMaterialTextureCount> m_textures{};
            std::array<Resource::RenderTexture, Resource::kMaterialTextureCount> m_fallbacks{};
            std::array<Resource::ResourceId, Resource::kMaterialTextureCount> m_lastSyncedTextureIds{};
            std::array<VkDescriptorSet, Rhi::kMaxFramesInFlight> m_sets{};

            uint32_t m_setDirtyCount{ 0 };
        };
    }
}
