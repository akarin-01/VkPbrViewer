#include "skybox_pass.h"
#include "rhi/context.h"
#include "rhi/graphics_pipeline.h"
#include "rhi/rendering_scope.h"
#include "rhi/swap_chain.h"
#include "render/render_scene.h"

#include <cassert>

namespace Kita::Pbrv
{
    namespace Render
    {
        SkyboxPass::SkyboxPass(const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            const RenderScene& scene)
            : RenderPassBase(context, resources, swapChain),
            m_target(scene.GetTarget()),
            m_frameData(scene.GetFrameData())
        {
            CreatePipeline();
        }

        SkyboxPass::~SkyboxPass() = default;

        void SkyboxPass::RecreateResources()
        {
            /* Empty */
        }

        void SkyboxPass::Draw(const Rhi::FrameInfo& frameInfo) const
        {
            assert(m_pipeline && "SkyboxPass: pipeline is null");

            auto& commandBuffer = frameInfo.m_commandBuffer;
            auto& frameIndex = frameInfo.m_frameIndex;

            // Begin rendering
            {
                VkExtent2D extent = m_swapChain.Extent();

                Rhi::RenderingAttachmentDesc colorDesc{};
                colorDesc.m_imageView = m_target.GetColorImageView();
                colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorDesc.m_resolveImageView = m_target.GetResolveImageView();
                colorDesc.m_resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;

                Rhi::RenderingAttachmentDesc depthDesc{};
                depthDesc.m_imageView = m_target.GetDepthImageView();
                depthDesc.m_imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
                depthDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

                Rhi::RenderingScope scope(commandBuffer, extent, { colorDesc }, &depthDesc);

                // Bind pipeline
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

                // Draw
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    0, 1, &m_frameData.GetSet(frameIndex), 0, nullptr);
                vkCmdDraw(commandBuffer, 36, 1, 0, 0);
            }
            // End rendering
        }

        void SkyboxPass::CreatePipeline()
        {
            Rhi::GraphicsPipelineBuilder builder(m_context.Device());
            builder.SetShaders("assets/shaders/skybox_vert.spv", "assets/shaders/skybox_frag.spv")
                .SetRasterizationSamples(m_context.SampleCount())
                .SetDepth(true, false, VK_COMPARE_OP_LESS_OR_EQUAL)
                .SetDescriptorSetLayouts(
                    {
                        m_frameData.GetSetLayout()
                    })
                .SetDynamicRendering({ m_target.GetColorFormat() }, m_target.GetDepthFormat());
            m_pipeline = builder.Build();
        }
    }
}
