#include "frame_sync.h"

#include "rhi/constants.h"
#include "rhi/context.h"
#include "rhi/swap_chain.h"
#include "resource/constants.h"

#include <stdexcept>
#include <array>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        FrameSync::FrameSync(const RenderContext& context, SwapChain& swapChain)
            : m_context(context), m_swapChain(swapChain)
        {
            CreateCommandBuffers();
            CreateFrameSyncObjects();
            CreatePresentSyncObjects();
        }

        FrameSync::~FrameSync()
        {
            CleanupFrameSyncObjects();
            CleanupPresentSyncObjects();
        }

        FrameInfo FrameSync::BeginFrame()
        {
            FrameInfo frameInfo{};

            vkWaitForFences(m_context.Device(), 1, &m_inFlights[m_frameIndex], VK_TRUE, UINT64_MAX);

            bool swapChainRecreated = m_swapChain.AcquireNextImage(UINT64_MAX, m_imageAvailables[m_frameIndex], VK_NULL_HANDLE, &m_imageIndex);
            if (swapChainRecreated)
            {
                RecreatePresentSyncObjects();
                frameInfo.m_swapChainRecreated = true;
                return frameInfo;
            }

            vkResetFences(m_context.Device(), 1, &m_inFlights[m_frameIndex]);

            VkCommandBuffer& commandBuffer = m_commandBuffers[m_frameIndex];
            vkResetCommandBuffer(commandBuffer, 0);

            // Begin recording commands
            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            frameInfo.m_swapChainRecreated = false;
            frameInfo.m_commandBuffer = commandBuffer;
            frameInfo.m_frameIndex = m_frameIndex;
            frameInfo.m_imageIndex = m_imageIndex;
            return frameInfo;
        }

        bool FrameSync::EndFrame()
        {
            VkCommandBuffer& commandBuffer = m_commandBuffers[m_frameIndex];

            // End recording commands
            vkEndCommandBuffer(commandBuffer);

            // Submit commands
            std::array<VkSemaphore, 1> waitSemaphores = { m_imageAvailables[m_frameIndex] };
            std::array<VkPipelineStageFlags, 1> waitStages = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            std::array<VkSemaphore, 1> signalSemaphores = { m_presentSemaphores[m_imageIndex] };
            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.waitSemaphoreCount = static_cast<uint32_t>(waitSemaphores.size());
            submitInfo.pWaitSemaphores = waitSemaphores.data();
            submitInfo.pWaitDstStageMask = waitStages.data();
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;
            submitInfo.signalSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
            submitInfo.pSignalSemaphores = signalSemaphores.data();

            if (vkQueueSubmit(m_context.GraphicsQueue(), 1, &submitInfo, m_inFlights[m_frameIndex]) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to submit draw command buffer!");
            }

            // Present the image
            std::array<VkSwapchainKHR, 1> swapChains = { m_swapChain.Handle() };
            std::array<uint32_t, 1> imageIndices = { m_imageIndex };
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = static_cast<uint32_t>(signalSemaphores.size());
            presentInfo.pWaitSemaphores = signalSemaphores.data();
            presentInfo.swapchainCount = static_cast<uint32_t>(swapChains.size());
            presentInfo.pSwapchains = swapChains.data();
            presentInfo.pImageIndices = imageIndices.data();

            bool swapChainRecreated = m_swapChain.QueuePresent(presentInfo);
            if (swapChainRecreated)
            {
                RecreatePresentSyncObjects();
            }

            m_frameIndex = (m_frameIndex + 1) % kMaxFramesInFlight;

            return swapChainRecreated;
        }

        void FrameSync::CreateCommandBuffers()
        {
            m_commandBuffers.resize(kMaxFramesInFlight);

            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = m_context.CommandPool();
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = kMaxFramesInFlight;

            if (vkAllocateCommandBuffers(m_context.Device(), &allocInfo, m_commandBuffers.data()) != VK_SUCCESS)
            {
                throw std::runtime_error("Failed to allocate command buffers!");
            }
        }

        void FrameSync::CreateFrameSyncObjects()
        {
            // Image available semaphores
            m_imageAvailables.resize(kMaxFramesInFlight);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
            {
                if (vkCreateSemaphore(m_context.Device(), &semaphoreInfo, nullptr, &m_imageAvailables[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create semaphore!");
                }
            }

            // In-flight fences
            m_inFlights.resize(kMaxFramesInFlight);

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
            {
                if (vkCreateFence(m_context.Device(), &fenceInfo, nullptr, &m_inFlights[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create fence!");
                }
            }
        }

        void FrameSync::CleanupFrameSyncObjects()
        {
            for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
            {
                vkDestroyFence(m_context.Device(), m_inFlights[i], nullptr);
            }

            for (uint32_t i = 0; i < kMaxFramesInFlight; ++i)
            {
                vkDestroySemaphore(m_context.Device(), m_imageAvailables[i], nullptr);
            }
        }

        void FrameSync::CreatePresentSyncObjects()
        {
            // Present semaphores: one per swapchain image, signals "slot ready" for present.
            m_presentSemaphores.resize(m_swapChain.ImageCount());

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            for (size_t i = 0; i < m_presentSemaphores.size(); ++i)
            {
                if (vkCreateSemaphore(m_context.Device(), &semaphoreInfo, nullptr, &m_presentSemaphores[i]) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create semaphore!");
                }
            }
        }

        void FrameSync::CleanupPresentSyncObjects()
        {
            for (size_t i = 0; i < m_presentSemaphores.size(); ++i)
            {
                vkDestroySemaphore(m_context.Device(), m_presentSemaphores[i], nullptr);
            }
        }

        void FrameSync::RecreatePresentSyncObjects()
        {
            CleanupPresentSyncObjects();
            CreatePresentSyncObjects();
        }
    }
}
