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
            const Rhi::RenderContext& context,
            Resource::RenderResources& resources,
            const Rhi::SwapChain& swapChain,
            Resource::DescriptorManager& descriptorMgr,
            const RenderScene& scene)
            : m_swapChain(swapChain),
            m_targetData(context, resources, descriptorMgr, m_swapChain.Extent())
        {
            CreateRenderPasses(window, context, resources, swapChain, scene);
        }

        RenderPipeline::~RenderPipeline()
        {
            DestroyRenderPasses();
        }

        void RenderPipeline::RecreateResources()
        {
            m_targetData.Recreate(m_swapChain.Extent());

            for (auto& pass : m_passes)
            {
                pass->RecreateResources();
            }
        }

        void RenderPipeline::Draw(const Rhi::FrameInfo& frameInfo) const
        {
            auto& commandBuffer = frameInfo.m_commandBuffer;
            auto& imageIndex = frameInfo.m_imageIndex;

            VkImageSubresourceRange colorRange{};
            colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            colorRange.baseMipLevel = 0;
            colorRange.levelCount = 1;
            colorRange.baseArrayLayer = 0;
            colorRange.layerCount = 1;

            // Swap chain image: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
            Rhi::TransitionImageLayout(commandBuffer,
                m_swapChain.Image(imageIndex),
                VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                colorRange);

            for (auto& pass : m_passes)
            {
                pass->Draw(frameInfo);
            }

            // Swap chain image: COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC_KHR
            Rhi::TransitionImageLayout(commandBuffer,
                m_swapChain.Image(imageIndex),
                VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
                VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE,
                colorRange);
        }

        void RenderPipeline::CreateRenderPasses(const Core::Window& window,
            const Rhi::RenderContext& context,
            Resource::RenderResources& resources,
            const Rhi::SwapChain& swapChain,
            const RenderScene& scene)
        {
            m_passes.push_back(std::make_unique<LitPass>(
                context, resources, swapChain,
                m_targetData, scene));
            m_passes.push_back(std::make_unique<SkyboxPass>(
                context, resources, swapChain,
                m_targetData, scene));
            m_passes.push_back(std::make_unique<PostProcessPass>(
                context, resources, swapChain,
                m_targetData, scene));
            m_passes.push_back(std::make_unique<UIPass>(
                context, resources, swapChain,
                window));
        }

        void RenderPipeline::DestroyRenderPasses()
        {
            m_passes.clear();
        }
    }
}
