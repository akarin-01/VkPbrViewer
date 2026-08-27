#pragma once

#include "rhi/constants.h"
#include "resource/render_texture.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <array>

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
        class ComputeConversion;
        template<uint32_t, uint32_t> class TextureSet;
    }

    namespace Render
    {
        class RenderIblData
        {
        public:
            RenderIblData(const Rhi::Context& context,
                Resource::Resources& resources,
                Resource::DescriptorManager& descriptorMgr);
            ~RenderIblData();

            void Update(uint32_t frameIndex, const Resource::RenderTexture& sourceCubemap);

            VkDescriptorSetLayout GetSetLayout() const;
            const VkDescriptorSet& GetSet(uint32_t frameIndex) const;
            VkDescriptorSetLayout GetBrdfLutSetLayout() const;
            const VkDescriptorSet& GetBrdfLutSet(uint32_t frameIndex) const;

        private:
            using BrdfLutSet = Resource::TextureSet<1, 1>;
            using BrdfLutArray = std::array<Resource::RenderTexture, 1>;
            using IblTextureSet = Resource::TextureSet<2, Rhi::kMaxFramesInFlight>;
            using IblTextureArray = std::array<Resource::RenderTexture, 2>;

            Resource::RenderTexture CreateIrradianceMap(const Resource::RenderTexture& sourceCubemap) const;
            Resource::RenderTexture CreatePrefilterEnvMap(const Resource::RenderTexture& sourceCubemap) const;
            Resource::RenderTexture CreateBrdfLut() const;

        private:
            const Rhi::Context& m_context;
            Resource::Resources& m_resources;
            Resource::DescriptorManager& m_descriptorMgr;

            Resource::RenderTexture m_lastCubemap{};

            std::unique_ptr<Resource::ComputeConversion> m_brdfConversion;
            std::unique_ptr<Resource::ComputeConversion> m_irradianceConversion;
            std::unique_ptr<Resource::ComputeConversion> m_prefilterConversion;
            std::unique_ptr<BrdfLutSet> m_brdfLutSet;
            std::unique_ptr<IblTextureSet> m_iblTextureSet;
        };
    }
}
