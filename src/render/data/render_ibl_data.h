#pragma once

#include "render/render_constants.h"
#include "render/render_texture.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <array>

namespace Kita::Pbrv
{
    /// STD430 layout
    struct PrefilterPC
    {
        float m_roughness{ 0.0f };  // 0..1, selects the mip level
        float m_mipCount{ 1.0f };
    };

    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;
    class ComputeConversion;
    template<uint32_t, uint32_t> class RenderTextureSet;

    class RenderIblData
    {
    public:
        RenderIblData(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderIblData();

        void Update(uint32_t frameIndex, const RenderTexture& sourceCubemap);

        VkDescriptorSetLayout GetSetLayout() const;
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const;
        VkDescriptorSetLayout GetBrdfLutSetLayout() const;
        const VkDescriptorSet& GetBrdfLutSet(uint32_t frameIndex) const;

    private:
        using BrdfLutSet = RenderTextureSet<1, 1>;
        using BrdfLutArray = std::array<RenderTexture, 1>;
        using TextureSet = RenderTextureSet<2, kMaxFramesInFlight>;
        using TextureArray = std::array<RenderTexture, 2>;

        RenderTexture CreateIrradianceMap(const RenderTexture& sourceCubemap) const;
        RenderTexture CreatePrefilterEnvMap(const RenderTexture& sourceCubemap) const;
        RenderTexture CreateBrdfLut() const;

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        RenderTexture m_lastCubemap{};

        std::unique_ptr<ComputeConversion> m_brdfConversion;
        std::unique_ptr<ComputeConversion> m_irradianceConversion;
        std::unique_ptr<ComputeConversion> m_prefilterConversion;
        std::unique_ptr<BrdfLutSet> m_brdfLutSet;
        std::unique_ptr<TextureSet> m_textureSet;
    };
}
