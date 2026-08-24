#pragma once

#include "render/render_resource_types.h"
#include "render/render_constants.h"

#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <array>

namespace Kita::Pbrv
{
    class Camera;
    class Light;
    class RenderContext;
    class RenderResources;
    class SwapChain;
    class DescriptorAllocator;

    struct FrameUbo
    {
        alignas(16) glm::mat4 m_viewProj;
        alignas(16) glm::mat4 m_skyboxViewProj;
        alignas(16) glm::vec4 m_viewPos;            // xyz - pos, w - 1 always
        alignas(16) glm::vec4 m_lightPos;           // xyz - pos, w - 0(directional light)
        alignas(16) glm::vec4 m_lightColor;         // xyz - rgb, w - intensity
    };
    STD140_ASSERT(FrameUbo, 176);

    class RenderFrameData
    {
    public:
        RenderFrameData(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator);
        ~RenderFrameData();

        void Update(uint32_t frameIndex, const Camera& camera, const Light& light);

        VkDescriptorSetLayout GetSetLayout() const { return m_setLayout; }
        const VkDescriptorSet& GetSet(uint32_t frameIndex) const { return m_sets[frameIndex]; }

    private:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const SwapChain& m_swapChain;
        const DescriptorAllocator& m_descriptorAllocator;

        std::array<RenderBufferHandle, kMaxFramesInFlight> m_uboHandles{};
        VkDescriptorSetLayout m_setLayout{ VK_NULL_HANDLE };
        std::array<VkDescriptorSet, kMaxFramesInFlight> m_sets{};
    };
}
