#pragma once

#include "rhi/constants.h"
#include "resource/render_texture.h"
#include "resource/resource_id.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <array>
#include <cstdint>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class RenderContext;
    }
    namespace Resource
    {
        class RenderResources;
        class DescriptorAllocator;
        class ComputeConversion;
        struct Texture;
        template<uint32_t, uint32_t> class RenderTextureSet;
    }
    namespace Scene
    {
        class Skybox;
    }

    namespace Render
    {
        class RenderSkyboxData
        {
        public:
            RenderSkyboxData(const Rhi::RenderContext& context,
                Resource::RenderResources& resources,
                const Resource::DescriptorAllocator& descriptorAllocator);
            ~RenderSkyboxData();

            void Update(uint32_t frameIndex, const Scene::Skybox& sceneSkybox);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const;
            Resource::RenderTexture GetCubemap() const;

        private:
            using TextureSet = Resource::RenderTextureSet<1, Rhi::kMaxFramesInFlight>;
            using TextureArray = std::array<Resource::RenderTexture, 1>;

            Resource::RenderTexture CreateCubemap(const Resource::Texture& texture) const;
            Resource::RenderTexture CreateEquirectTexture(const Resource::Texture& texture, VkFormat format) const;

        private:
            const Rhi::RenderContext& m_context;
            Resource::RenderResources& m_resources;
            const Resource::DescriptorAllocator& m_descriptorAllocator;

            Resource::ResourceId m_lastSkyboxId{ Resource::kInvalidId };

            std::unique_ptr<TextureSet> m_textureSet;
            std::unique_ptr<Resource::ComputeConversion> m_conversion;
        };
    }
}
