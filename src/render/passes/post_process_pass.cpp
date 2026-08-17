#include "post_process_pass.h"

#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"
#include "render/descriptor_allocator.h"
#include "render/graphics_pipeline.h"
#include "render/rendering_scope.h"

#include "render/data/render_target_data.h"

#include <cassert>

namespace Kita::Pbrv
{
    PostProcessPass::PostProcessPass(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        RenderTargetData& targetData)
        : RenderPassBase(context, resources, swapChain),
        m_targetData(targetData)
    {
        CreatePipeline();
    }

    PostProcessPass::~PostProcessPass()
    {
        m_pipeline.reset();
    }

    void PostProcessPass::RecreateResources()
    {
        /* Empty */
    }

    void PostProcessPass::Draw(const FrameInfo& frameInfo) const
    {
        assert(m_pipeline && "PostProcessPass: pipeline is null");

        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& imageIndex = frameInfo.m_imageIndex;

        // Color image: COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
        m_targetData.TransitionColorImageLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT, VK_ACCESS_2_SHADER_READ_BIT);

        VkImageSubresourceRange colorRange{};
        colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorRange.baseMipLevel = 0;
        colorRange.levelCount = 1;
        colorRange.baseArrayLayer = 0;
        colorRange.layerCount = 1;

        // Swap chain image: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
        TransitionImageLayout(commandBuffer,
            m_swapChain.Image(imageIndex),
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            colorRange);


        // Begin rendering
        {
            VkExtent2D extent = m_swapChain.Extent();

            RenderingAttachmentDesc colorDesc{};
            colorDesc.m_imageView = m_swapChain.ImageView(imageIndex);
            colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            RenderingScope scope(commandBuffer, extent, { colorDesc });

            // Bind pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

            // Draw
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                0, 1, &m_targetData.GetSet(), 0, nullptr);
            vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        }
        // End rendering

        // Transition the image layout to present
        TransitionImageLayout(commandBuffer,
            m_swapChain.Image(imageIndex),
            VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_BOTTOM_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            colorRange);
    }

    void PostProcessPass::CreatePipeline()
    {
        GraphicsPipelineBuilder builder(m_context.Device());
        builder.SetShaders("assets/shaders/post_process_vert.spv", "assets/shaders/post_process_frag.spv")
            .SetDescriptorSetLayouts({ m_targetData.GetSetLayout() })
            .SetDynamicRendering({ m_swapChain.Format() }, VK_FORMAT_UNDEFINED);
        m_pipeline = builder.Build();
    }
}
