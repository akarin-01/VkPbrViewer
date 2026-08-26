#pragma once

#include "rhi/constants.h"
#include "resource/texture.h"

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

            Resource::RenderTexture CreateCubemap(const Scene::Skybox& sceneSkybox) const;
            Resource::RenderTexture CreateEquirectTexture(const Scene::Skybox& sceneSkybox, VkFormat format) const;

        private:
            const Rhi::RenderContext& m_context;
            Resource::RenderResources& m_resources;
            const Resource::DescriptorAllocator& m_descriptorAllocator;

            uint64_t m_lastSyncedRevision{ UINT64_MAX };

            std::unique_ptr<TextureSet> m_textureSet;
            std::unique_ptr<Resource::ComputeConversion> m_conversion;
        };
    }
}
