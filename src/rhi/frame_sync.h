#pragma once

#include "rhi/frame_info.h"
#include "resource/types.h"

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
        class SwapChain;

        class FrameSync
        {
        public:
            FrameSync(const Context& context, SwapChain& swapChain);
            ~FrameSync();

            FrameInfo BeginFrame();
            bool EndFrame();

        private:
            void CreateCommandBuffers();
            void CreateFrameSyncObjects();
            void CleanupFrameSyncObjects();
            void CreatePresentSyncObjects();
            void CleanupPresentSyncObjects();
            void RecreatePresentSyncObjects();

        private:
            const Context& m_context;
            SwapChain& m_swapChain;

            std::vector<VkCommandBuffer> m_commandBuffers;

            /// Per-frame semaphore: signaled by the swapchain when an image is available,
            /// waited on by the graphics queue at submit time.
            std::vector<VkSemaphore> m_imageAvailables;

            /// Per-frame fence: the CPU waits on it before reusing the frame's command buffer.
            std::vector<VkFence> m_inFlights;

            /// Per-image semaphore: "slot ready" signal in the producer-consumer model.
            /// The graphics queue produces the rendered content and signals it, and the
            /// present queue consumes it. Indexed by image index rather than frame index:
            /// the swapchain only re-acquires an image after its previous present completes,
            /// so each semaphore never receives a second signal while a present still waits.
            std::vector<VkSemaphore> m_presentSemaphores;
            uint32_t m_frameIndex{ 0 };
            uint32_t m_imageIndex{ 0 };
        };
    }
}
