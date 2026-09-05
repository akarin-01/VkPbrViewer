#include "render_pipeline.h"

#include "rhi/swap_chain.h"
#include "rhi/utils.h"
#include "render/passes/shadow_pass.h"
#include "render/passes/lit_pass.h"
#include "render/passes/post_process_pass.h"
#include "render/passes/skybox_pass.h"
#include "render/passes/ui_pass.h"
#include "render/render_scene.h"

#include <cassert>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            // Render targets here are all single-mip, single-layer
            VkImageSubresourceRange TargetRange(VkImageAspectFlags aspect)
            {
                VkImageSubresourceRange range{};
                range.aspectMask = aspect;
                range.levelCount = 1;
                range.layerCount = 1;
                return range;
            }
        }

        RenderPipeline::RenderPipeline(const Core::Window& window,
            const Rhi::Context& context,
            const Rhi::SwapChain& swapChain,
            RenderScene& scene)
            : m_swapChain(swapChain),
            m_shadow(scene.GetShadow()),
            m_target(scene.GetTarget())
        {
            CreateRenderPasses(window, context, swapChain, scene);
        }

        RenderPipeline::~RenderPipeline() = default;

        void RenderPipeline::RecreateResources()
        {
            m_shadowPass->RecreateResources();
            m_litPass->RecreateResources();
            m_skyboxPass->RecreateResources();
            m_postProcessPass->RecreateResources();
            m_uiPass->RecreateResources();
        }

        void RenderPipeline::Draw(const Rhi::FrameInfo& frameInfo) const
        {
            auto& commandBuffer = frameInfo.m_commandBuffer;
            auto& imageIndex = frameInfo.m_imageIndex;

            // Shadow map to write
            TransitionShadowMapToWriteLayout(commandBuffer);

            m_shadowPass->Draw(frameInfo);

            // Shadow map to read
            TransitionShadowMapToReadLayout(commandBuffer);

            // Target to write
            TransitionTargetToWriteLayout(commandBuffer);

            m_litPass->Draw(frameInfo);

            // Dynamic rendering scopes get no implicit dependency between each
            // other: make the lit pass's writes visible before the skybox scope
            // reloads the target
            BarrierTargetLitToSkybox(commandBuffer);

            m_skyboxPass->Draw(frameInfo);

            // Target to read
            TransitionTargetToReadLayout(commandBuffer);

            // Swap chain image: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
            TransitionSwapchainToWriteLayout(commandBuffer, imageIndex);

            m_postProcessPass->Draw(frameInfo);

            // Make the post pass's write visible before the UI scope loads it
            BarrierSwapchainPostToUI(commandBuffer, imageIndex);

            // Overlay
            m_uiPass->Draw(frameInfo);

            // Swap chain image: COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
            TransitionSwapchainToPresentLayout(commandBuffer, imageIndex);
        }

        void RenderPipeline::TransitionSwapchainToWriteLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
        {
            Rhi::TransitionImageLayout(commandBuffer,
                m_swapChain.Image(imageIndex),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                TargetRange(VK_IMAGE_ASPECT_COLOR_BIT));
        }

        void RenderPipeline::TransitionSwapchainToPresentLayout(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
        {
            Rhi::TransitionImageLayout(commandBuffer,
                m_swapChain.Image(imageIndex),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                TargetRange(VK_IMAGE_ASPECT_COLOR_BIT));
        }

        void RenderPipeline::TransitionTargetToWriteLayout(VkCommandBuffer commandBuffer) const
        {
            Rhi::TransitionImageLayout(commandBuffer,
                m_target.GetColorImage(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                TargetRange(VK_IMAGE_ASPECT_COLOR_BIT));
            Rhi::TransitionImageLayout(commandBuffer,
                m_target.GetResolveImage(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                TargetRange(VK_IMAGE_ASPECT_COLOR_BIT));
            Rhi::TransitionImageLayout(commandBuffer,
                m_target.GetDepthImage(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                TargetRange(VK_IMAGE_ASPECT_DEPTH_BIT));
        }

        void RenderPipeline::TransitionTargetToReadLayout(VkCommandBuffer commandBuffer) const
        {
            Rhi::TransitionImageLayout(commandBuffer,
                m_target.GetResolveImage(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
                TargetRange(VK_IMAGE_ASPECT_COLOR_BIT));
        }

        void RenderPipeline::CreateRenderPasses(const Core::Window& window,
            const Rhi::Context& context,
            const Rhi::SwapChain& swapChain,
            const RenderScene& scene)
        {
            m_shadowPass = std::make_unique<ShadowPass>(
                context, swapChain, scene);
            m_litPass = std::make_unique<LitPass>(
                context, swapChain, scene);
            m_skyboxPass = std::make_unique<SkyboxPass>(
                context, swapChain, scene);
            m_postProcessPass = std::make_unique<PostProcessPass>(
                context, swapChain, scene);
            m_uiPass = std::make_unique<UIPass>(
                context, swapChain, window);
        }

        void RenderPipeline::TransitionShadowMapToWriteLayout(VkCommandBuffer commandBuffer) const
        {
            Rhi::TransitionImageLayout(commandBuffer, m_shadow.GetImage(),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                TargetRange(VK_IMAGE_ASPECT_DEPTH_BIT));
        }

        void RenderPipeline::TransitionShadowMapToReadLayout(VkCommandBuffer commandBuffer) const
        {
            Rhi::TransitionImageLayout(commandBuffer,
                m_shadow.GetImage(),
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT,
                TargetRange(VK_IMAGE_ASPECT_DEPTH_BIT));
        }

        // Lit writes the whole target; the skybox scope then reloads the MSAA
        // color/depth and re-resolves into the resolve image
        void RenderPipeline::BarrierTargetLitToSkybox(VkCommandBuffer commandBuffer) const
        {
            Rhi::ImageMemoryBarrier(commandBuffer, m_target.GetColorImage(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                TargetRange(VK_IMAGE_ASPECT_COLOR_BIT));

            // Skybox re-resolves into the same resolve image: write-after-write
            Rhi::ImageMemoryBarrier(commandBuffer, m_target.GetResolveImage(),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                TargetRange(VK_IMAGE_ASPECT_COLOR_BIT));

            // Skybox tests against depth without writing it
            Rhi::ImageMemoryBarrier(commandBuffer, m_target.GetDepthImage(),
                VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT,
                VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
                TargetRange(VK_IMAGE_ASPECT_DEPTH_BIT));
        }

        void RenderPipeline::BarrierSwapchainPostToUI(VkCommandBuffer commandBuffer, uint32_t imageIndex) const
        {
            Rhi::ImageMemoryBarrier(commandBuffer, m_swapChain.Image(imageIndex),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                TargetRange(VK_IMAGE_ASPECT_COLOR_BIT));
        }
    }
}
