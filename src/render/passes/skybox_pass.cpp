#include "skybox_pass.h"

#include "core/log.h"
#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"
#include "render/descriptor_allocator.h"
#include "render/data/render_frame_data.h"
#include "render/data/render_skybox_data.h"
#include "render/graphics_pipeline.h"

#include <stdexcept>
#include <array>
#include <cassert>

namespace Kita::Pbrv
{
    SkyboxPass::SkyboxPass(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator,
        const RenderFrameData& frameData,
        const RenderSkyboxData& skybox,
        const RenderTarget& target)
        : RenderPassBase(context, resources, swapChain, descriptorAllocator, target),
        m_frameData(frameData),
        m_skybox(skybox)
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
        assert(m_pipeline && "SkyboxPass: pipeline is null");

        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& frameIndex = frameInfo.m_frameIndex;

        // Begin rendering
        VkExtent2D extent = m_swapChain.Extent();

        RenderImageView* colorImageView = m_resources.GetImageView(m_target.m_colorTex.m_imageViewHandle);
        assert(colorImageView && "SkyboxPass: Color image view handle is invalid");
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorImageView->m_imageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;

        RenderImageView* depthImageView = m_resources.GetImageView(m_target.m_depthTex.m_imageViewHandle);
        assert(depthImageView && "SkyboxPass: Depth image view handle is invalid");
        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depthImageView->m_imageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = 1;
        renderingInfo.pColorAttachments = &colorAttachment;
        renderingInfo.pDepthAttachment = &depthAttachment;

        vkCmdBeginRendering(commandBuffer, &renderingInfo);

        // Viewport and scissor
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(extent.width);
        viewport.height = static_cast<float>(extent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = { 0, 0 };
        scissor.extent = extent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        // Bind pipeline
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

        // Draw
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
            0, 1, &m_frameData.GetSet(frameIndex), 0, nullptr);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
            1, 1, &m_skybox.GetSet(frameIndex), 0, nullptr);
        vkCmdDraw(commandBuffer, 36, 1, 0, 0);

        // End rendering
        vkCmdEndRendering(commandBuffer);
    }

    void SkyboxPass::CreatePipeline()
    {
        GraphicsPipelineBuilder builder(m_context.Device());
        builder.SetShaders("assets/shaders/skybox_vert.spv", "assets/shaders/skybox_frag.spv")
            .SetDepth(true, false, VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetDescriptorSetLayouts({ m_frameData.GetSetLayout(), m_skybox.GetSetLayout(), })
            .SetDynamicRendering({ m_target.m_colorFormat }, m_target.m_depthFormat);
        m_pipeline = builder.Build();
    }
}
