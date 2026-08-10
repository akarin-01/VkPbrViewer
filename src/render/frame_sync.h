#pragma once

#include "render/render_resource_types.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Kita::Pbrv
{
    class RenderContext;
    class SwapChain;

    class FrameSync
    {
    public:
        FrameSync(const RenderContext& context, SwapChain& swapChain);
        ~FrameSync();

        FrameInfo BeginFrame();
        bool EndFrame();

    private:
        void CreateCommandBuffers();
        void CreateFrameSyncObjects();
        void CleanupFrameSyncObjects();
        void CreateRenderSyncObjects();
        void CleanupRenderSyncObjects();
        void RecreateRenderSyncObjects();

    private:
        const RenderContext& m_context;
        SwapChain& m_swapChain;

        std::vector<VkCommandBuffer> m_commandBuffers;
        std::vector<VkSemaphore> m_imageAvailables;
        std::vector<VkFence> m_inFlights;
        std::vector<VkSemaphore> m_renderFinisheds;
        uint32_t m_frameIndex{ 0 };
        uint32_t m_imageIndex{ 0 };
    };
}