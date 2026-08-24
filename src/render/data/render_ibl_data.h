#pragma once

#include "render/render_constants.h"
#include "render/render_texture.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <array>

namespace Kita::Pbrv
{
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

    private:
        using TextureSet = RenderTextureSet<1, kMaxFramesInFlight>;
        using TextureArray = std::array<RenderTexture, 1>;

        RenderTexture CreateIrradianceMap(const RenderTexture& sourceCubemap) const;

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        RenderTexture m_lastCubemap{};

        std::unique_ptr<TextureSet> m_textureSet;
        std::unique_ptr<ComputeConversion> m_conversion;
    };
}
