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
#include "render/rendering_scope.h"

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
        RenderImageView* depthImageView = m_resources.GetImageView(m_target.m_depthTex.m_imageViewHandle);
        assert(depthImageView && "SkyboxPass: Depth image view handle is invalid");

        // Begin rendering
        {
            RenderingAttachmentDesc colorDesc{};
            colorDesc.m_imageView = colorImageView->m_imageView;
            colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;

            RenderingAttachmentDesc depthDesc{};
            depthDesc.m_imageView = depthImageView->m_imageView;
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
            .SetDepth(true, false, VK_COMPARE_OP_LESS_OR_EQUAL)
            .SetDescriptorSetLayouts({ m_frameData.GetSetLayout(), m_skybox.GetSetLayout(), })
            .SetDynamicRendering({ m_target.m_colorFormat }, m_target.m_depthFormat);
        m_pipeline = builder.Build();
    }
}
