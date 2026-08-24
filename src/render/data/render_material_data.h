#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <memory>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;
    class Material;
    class Texture;
    template<uint32_t, uint32_t> class RenderTextureSet;

    class RenderMaterialData
    {
    public:
        RenderMaterialData(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderMaterialData();

        void Update(uint32_t frameIndex, const Material& sceneMat);

        VkDescriptorSetLayout GetSetLayout() const;
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const;
        const MaterialPC& GetPushConstant() const { return m_pushConstant; }

    private:
        using TextureSet = RenderTextureSet<kMaterialTextureCount, kMaxFramesInFlight>;
        using TextureArray = std::array<RenderTexture, kMaterialTextureCount>;

        TextureArray CreateFallbacks() const;
        RenderTexture CreateTexture(const Texture& sceneTex) const;

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        MaterialPC m_pushConstant{ {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };

        std::array<uint64_t, kMaterialTextureCount> m_lastSyncedRevisions{};

        std::unique_ptr<TextureSet> m_textureSet;
    };
}
