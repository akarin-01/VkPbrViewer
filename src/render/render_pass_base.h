#pragma once

#include "render/render_resource_types.h"

namespace Kita::Pbrv
{
    struct RenderTarget
    {
        RenderTexture m_colorTex{};
        RenderTexture m_depthTex{};
        VkFormat m_colorFormat{ VK_FORMAT_UNDEFINED };
        VkFormat m_depthFormat{ VK_FORMAT_UNDEFINED };
    };

    class RenderContext;
    class RenderResources;
    class SwapChain;
    class DescriptorAllocator;

    class RenderPassBase
    {
    public:
        RenderPassBase(const RenderContext& context,
            RenderResources& resources,
            const SwapChain& swapChain,
            const DescriptorAllocator& descriptorAllocator,
            const RenderTarget& target)
            : m_context(context),
            m_resources(resources),
            m_swapChain(swapChain),
            m_descriptorAllocator(descriptorAllocator),
            m_target(target)
        {
        }
        virtual ~RenderPassBase() = default;

        virtual void RecreateResources() = 0;
        virtual void Draw(const FrameInfo& frameInfo) const = 0;

    protected:
        const RenderContext& m_context;
        RenderResources& m_resources;
        const SwapChain& m_swapChain;
        const DescriptorAllocator& m_descriptorAllocator;
        const RenderTarget& m_target;
    };
}