#pragma once

#include "render/render_resource_types.h"

#include <array>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;
    class Skybox;

    /// GPU resources and descriptor management for the skybox.
    /// Marks the descriptor sets for refresh when the CPU-side Skybox data changes (dirty);
    /// the equirect -> cubemap conversion and texture upload are not implemented yet (see WriteSet).
    class SkyboxEnvironment
    {
    public:
        SkyboxEnvironment(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator);
        ~SkyboxEnvironment();

        void Update(uint32_t frameIndex, const Skybox& sceneSkybox);

        VkDescriptorSetLayout GetSetLayout() const { return m_setLayout; }
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

    private:
        void WriteSet(VkDescriptorSet set);

        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        RenderTexture m_texture{};
        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_sets{};
        uint32_t m_setRefreshCount{ 0 };
    };
}
