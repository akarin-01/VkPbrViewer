#pragma once

#include "core/macro.h"
#include "rhi/constants.h"
#include "resource/constants.h"
#include "resource/resource_id.h"
#include "resource/render_texture.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
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
        class Resources;
        class DescriptorManager;
        struct TextureAsset;
        template<uint32_t, uint32_t> class TextureSet;
    }
    namespace Scene
    {
        class Material;
    }

    namespace Render
    {
        struct MaterialPC
        {
            alignas(16) glm::vec4 m_albedo;
            alignas(16) glm::vec4 m_params;             // x - metallic, y - roughness, z - ao, w - padding
            alignas(16) glm::vec4 m_emissive;           // xyz - emissive, w - padding
        };
        STD140_ASSERT(MaterialPC, 48);

        class RenderMaterialData
        {
        public:
            RenderMaterialData(const Rhi::Context& context,
                Resource::Resources& resources,
                Resource::DescriptorManager& descriptorMgr);
            ~RenderMaterialData();

            void Update(uint32_t frameIndex, const Scene::Material& sceneMat);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const;
            const MaterialPC& GetPushConstant() const { return m_pushConstant; }

        private:
            using TextureSet = Resource::TextureSet<Resource::kMaterialTextureCount, Rhi::kMaxFramesInFlight>;
            using TextureArray = std::array<Resource::RenderTexture, Resource::kMaterialTextureCount>;

            TextureArray CreateFallbacks() const;
            Resource::RenderTexture CreateTexture(const Resource::TextureAsset& texture) const;

        private:
            const Rhi::Context& m_context;
            Resource::Resources& m_resources;
            Resource::DescriptorManager& m_descriptorMgr;

            MaterialPC m_pushConstant{ {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };

            std::array<Resource::ResourceId, Resource::kMaterialTextureCount> m_lastSyncedTextureIds{};

            std::unique_ptr<TextureSet> m_textureSet;
        };
    }
}
