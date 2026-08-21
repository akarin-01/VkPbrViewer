#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <array>

namespace Kita::Pbrv
{
    class PostProcess;
    class RenderContext;
    class RenderResources;
    class DescriptorAllocator;

    class RenderPostProcessData
    {
    public:
        RenderPostProcessData(const RenderContext& context,
            RenderResources& resources,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderPostProcessData();

        void Update(uint32_t frameIndex, const PostProcess& postProcess);

        VkDescriptorSetLayout GetSetLayout() const { return m_setLayout; }
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const DescriptorAllocator& m_descriptorAllocator;

        std::array<RenderBufferHandle, kMaxFramesInFlight> m_uboHandles{};
        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_sets{};
    };
}
