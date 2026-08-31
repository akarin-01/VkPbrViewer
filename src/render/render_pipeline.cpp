#include "render_pipeline.h"

#include "rhi/swap_chain.h"
#include "rhi/utils.h"
#include "render/passes/lit_pass.h"
#include "render/passes/post_process_pass.h"
#include "render/passes/skybox_pass.h"
#include "render/passes/ui_pass.h"
#include "render/render_scene.h"

#include <vulkan/vulkan.h>
#include <cassert>

namespace Kita::Pbrv
{
    namespace Render
    {
        RenderPipeline::RenderPipeline(const Core::Window& window,
            const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            RenderScene& scene)
            : m_swapChain(swapChain),
            m_target(scene.GetGlobal().m_target)
        {
            CreateRenderPasses(window, context, resources, swapChain, scene);
        }

        RenderPipeline::~RenderPipeline() = default;

        void RenderPipeline::RecreateResources()
        {
            m_litPass->RecreateResources();
            m_skyboxPass->RecreateResources();
            m_postProcessPass->RecreateResources();
            m_uiPass->RecreateResources();
        }

        void RenderPipeline::Draw(const Rhi::FrameInfo& frameInfo) const
        {
            auto& commandBuffer = frameInfo.m_commandBuffer;
            auto& imageIndex = frameInfo.m_imageIndex;

            // Swap chain image: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
            TransitionSwapchainToWriteLayout(commandBuffer, imageIndex);

            // Target to write
            TransitionTargetToWriteLayout(commandBuffer);

            m_litPass->Draw(frameInfo);
            m_skyboxPass->Draw(frameInfo);

            // Target to read
            TransitionTargetToReadLayout(commandBuffer);

            m_postProcessPass->Draw(frameInfo);

            // Overlay
            m_uiPass->Draw(frameInfo);

            // Swap chain image: COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
            TransitionSwapchainToPresentLayout(commandBuffer, imageIndex);
        }

        void RenderPipeline::TransitionSwapchainToWriteLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
        {
            VkImageSubresourceRange colorRange{};
            colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorRange.baseMipLevel = 0;
            colorRange.levelCount = 1;
            colorRange.baseArrayLayer = 0;
            colorRange.layerCount = 1;

            Rhi::TransitionImageLayout(commandBuffer,
                m_swapChain.Image(imageIndex),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                colorRange);
        }

        void RenderPipeline::TransitionSwapchainToPresentLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
        {
            VkImageSubresourceRange colorRange{};
            colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorRange.baseMipLevel = 0;
            colorRange.levelCount = 1;
            colorRange.baseArrayLayer = 0;
            colorRange.layerCount = 1;

            Rhi::TransitionImageLayout(commandBuffer,
                m_swapChain.Image(imageIndex),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                colorRange);
        }

        void RenderPipeline::TransitionTargetToWriteLayout(VkCommandBuffer commandBuffer) const
        {
            VkImageSubresourceRange colorRange{};
            colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorRange.baseMipLevel = 0;
            colorRange.levelCount = 1;
            colorRange.baseArrayLayer = 0;
            colorRange.layerCount = 1;

            VkImageSubresourceRange depthRange{};
            depthRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            depthRange.baseMipLevel = 0;
            depthRange.levelCount = 1;
            depthRange.baseArrayLayer = 0;
            depthRange.layerCount = 1;

            Rhi::TransitionImageLayout(commandBuffer,
                m_target.GetColorImage(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                colorRange);
            Rhi::TransitionImageLayout(commandBuffer,
                m_target.GetResolveImage(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                colorRange);
            Rhi::TransitionImageLayout(commandBuffer,
                m_target.GetDepthImage(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                depthRange);
        }

        void RenderPipeline::TransitionTargetToReadLayout(VkCommandBuffer commandBuffer) const
        {
            VkImageSubresourceRange colorRange{};
            colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorRange.baseMipLevel = 0;
            colorRange.levelCount = 1;
            colorRange.baseArrayLayer = 0;
            colorRange.layerCount = 1;

            Rhi::TransitionImageLayout(commandBuffer,
                m_target.GetResolveImage(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
                colorRange);
        }

        void RenderPipeline::CreateRenderPasses(const Core::Window& window,
            const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            const RenderScene& scene)
        {
            m_litPass = std::make_unique<LitPass>(
                context, resources, swapChain, scene);
            m_skyboxPass = std::make_unique<SkyboxPass>(
                context, resources, swapChain, scene);
            m_postProcessPass = std::make_unique<PostProcessPass>(
                context, resources, swapChain, scene);
            m_uiPass = std::make_unique<UIPass>(
                context, resources, swapChain, window);
        }
    }
}
