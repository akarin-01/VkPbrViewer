#pragma once

#include "render/render_texture.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <array>
#include <cstdint>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;
    class Skybox;
    class ComputeConversion;
    template<uint32_t, uint32_t> class RenderTextureSet;

    class RenderSkyboxData
    {
    public:
        RenderSkyboxData(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderSkyboxData();

        void Update(uint32_t frameIndex, const Skybox& sceneSkybox);

        VkDescriptorSetLayout GetSetLayout() const;
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const;
        RenderTexture GetCubemap() const;

    private:
        using TextureSet = RenderTextureSet<1, kMaxFramesInFlight>;
        using TextureArray = std::array<RenderTexture, 1>;

        RenderTexture CreateCubemap(const Skybox& sceneSkybox) const;
        RenderTexture CreateEquirectTexture(const Skybox& sceneSkybox, VkFormat format) const;

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        uint64_t m_lastSyncedRevision{ UINT64_MAX };

        std::unique_ptr<TextureSet> m_textureSet;
        std::unique_ptr<ComputeConversion> m_conversion;
    };
}
