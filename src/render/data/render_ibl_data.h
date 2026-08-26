#pragma once

#include "rhi/constants.h"
#include "resource/constants.h"
#include "resource/texture.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <array>

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

    namespace Render
    {
        struct PrefilterPC
        {
            float m_roughness{ 0.0f };  // 0..1, selects the mip level
            float m_mipCount{ 1.0f };
        };

        class RenderIblData
        {
        public:
            RenderIblData(const Rhi::RenderContext& context,
                Resource::RenderResources& resources,
                const Resource::DescriptorAllocator& descriptorAllocator);
            ~RenderIblData();

            void Update(uint32_t frameIndex, const Resource::RenderTexture& sourceCubemap);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const;
            VkDescriptorSetLayout GetBrdfLutSetLayout() const;
            const VkDescriptorSet& GetBrdfLutSet(uint32_t frameIndex) const;

        private:
            using BrdfLutSet = Resource::RenderTextureSet<1, 1>;
            using BrdfLutArray = std::array<Resource::RenderTexture, 1>;
            using TextureSet = Resource::RenderTextureSet<2, Rhi::kMaxFramesInFlight>;
            using TextureArray = std::array<Resource::RenderTexture, 2>;

            Resource::RenderTexture CreateIrradianceMap(const Resource::RenderTexture& sourceCubemap) const;
            Resource::RenderTexture CreatePrefilterEnvMap(const Resource::RenderTexture& sourceCubemap) const;
            Resource::RenderTexture CreateBrdfLut() const;

        private:
            const Rhi::RenderContext& m_context;
            Resource::RenderResources& m_resources;
            const Resource::DescriptorAllocator& m_descriptorAllocator;

            Resource::RenderTexture m_lastCubemap{};

            std::unique_ptr<Resource::ComputeConversion> m_brdfConversion;
            std::unique_ptr<Resource::ComputeConversion> m_irradianceConversion;
            std::unique_ptr<Resource::ComputeConversion> m_prefilterConversion;
            std::unique_ptr<BrdfLutSet> m_brdfLutSet;
            std::unique_ptr<TextureSet> m_textureSet;
        };
    }
}
