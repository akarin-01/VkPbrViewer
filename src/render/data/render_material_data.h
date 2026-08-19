#pragma once

#include "render/render_resource_types.h"

#include <array>
#include <vulkan/vulkan.h>
#include <cstdint>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;
    class Material;
    class Texture;

    class RenderMaterialData
    {
    public:
        RenderMaterialData(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderMaterialData();

        void Update(uint32_t frameIndex, const Material& sceneMat);

        VkDescriptorSetLayout GetSetLayout() const { return m_setLayout; }
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }
        const MaterialPC& GetPushConstant() const { return m_pushConstant; }

    private:
        void CreateFallbacks();
        void DestroyFallbacks();

        bool UpdateTextureSlot(uint32_t slot, const Texture& sceneTex);
        RenderTexture CreateTexture(const Texture& sceneTex) const;
        void DestroyTexture(RenderTexture& texture) const;
        void WriteSet(VkDescriptorSet set);

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        std::array<RenderTexture, kMaterialTextureCount> m_fallbackTextures{};
        std::array<RenderTexture, kMaterialTextureCount> m_textures{};
        MaterialPC m_pushConstant{ {1.0f, 1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f, 1.0f} };
        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_sets{};

        uint32_t m_setRefreshCount{ 0 };
        std::array<uint64_t, kMaterialTextureCount> m_lastSyncedRevisions{};
    };
}
