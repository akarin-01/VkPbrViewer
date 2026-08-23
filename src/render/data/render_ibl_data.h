#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <memory>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;
    class ComputeConversion;

    class RenderIblData
    {
    public:
        RenderIblData(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderIblData();

        void Update(uint32_t frameIndex, const RenderTexture& skyboxCubemap);

        VkDescriptorSetLayout GetSetLayout() const { return m_setLayout; }
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

    private:
        void CreateFallback();
        void DestroyFallback();
        void DestroyTexture(RenderTexture& texture) const;
        void WriteSet(VkDescriptorSet set) const;
        RenderTexture CreateIrradianceMap(const RenderTexture& sourceCubemap) const;
        RenderTexture CreateCubemapTexture(uint32_t faceSize, VkFormat format) const;

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        RenderTexture m_irradianceMap{};
        RenderTexture m_fallback{};
        RenderTexture m_lastSkyboxCubemap{};

        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_sets{};
        uint32_t m_setRefreshCount{ 0 };

        std::unique_ptr<ComputeConversion> m_conversion;
    };
}
