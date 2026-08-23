#include "skybox_pass.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"
#include "render/render_scene.h"
#include "render/graphics_pipeline.h"
#include "render/rendering_scope.h"
#include "render/data/render_target_data.h"

#include <cassert>

namespace Kita::Pbrv
{
    SkyboxPass::SkyboxPass(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        RenderTargetData& targetData,
        const RenderScene& scene)
        : RenderPassBase(context, resources, swapChain),
        m_targetData(targetData),
        m_frameData(scene.GetFrameData()),
        m_skybox(scene.GetSkyboxData())
    {
        CreatePipeline();
    }

    SkyboxPass::~SkyboxPass() = default;

    void SkyboxPass::RecreateResources()
    {
        /* Empty */
    }

    void SkyboxPass::Draw(const FrameInfo& frameInfo) const
    {
        if (!m_skybox.IsReady())
        {
            return;
        }

        assert(m_pipeline && "SkyboxPass: pipeline is null");

        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& frameIndex = frameInfo.m_frameIndex;

        // Color image -> COLOR_ATTACHMENT_OPTIMAL
        m_targetData.TransitionColorImageLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

        // Depth image -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        m_targetData.TransitionDepthImageLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT);

        // Begin rendering
        {
            VkExtent2D extent = m_swapChain.Extent();

            RenderingAttachmentDesc colorDesc{};
            colorDesc.m_imageView = m_targetData.GetColorImageView();
            colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            RenderingAttachmentDesc depthDesc{};
            depthDesc.m_imageView = m_targetData.GetDepthImageView();
            depthDesc.m_imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            depthDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            RenderingScope scope(commandBuffer, extent, { colorDesc }, &depthDesc);

            // Bind pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

            // Draw
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                0, 1, &m_frameData.GetSet(frameIndex), 0, nullptr);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                1, 1, &m_skybox.GetSet(frameIndex), 0, nullptr);
            vkCmdDraw(commandBuffer, 36, 1, 0, 0);
        }
        // End rendering
    }

    void SkyboxPass::CreatePipeline()
    {
        GraphicsPipelineBuilder builder(m_context.Device());
        builder.SetShaders("assets/shaders/skybox_vert.spv", "assets/shaders/skybox_frag.spv")
            .SetRasterizationSamples(m_context.SampleCount())
            .SetDepth(true, false, VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetDescriptorSetLayouts({ m_frameData.GetSetLayout(), m_skybox.GetSetLayout(), })
            .SetDynamicRendering({ m_targetData.GetColorFormat() }, m_targetData.GetDepthFormat());
        m_pipeline = builder.Build();
    }
}
