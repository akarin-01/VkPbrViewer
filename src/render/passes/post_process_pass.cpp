#include "post_process_pass.h"

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
        PostProcessPass::PostProcessPass(const Rhi::Context& context,
            const Rhi::SwapChain& swapChain,
            const RenderScene& scene)
            : RenderPassBase(context, swapChain),
            m_postProcess(scene.GetPostProcess())
        {
            CreatePipeline(
                {
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::Empty),
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::PostProcess),
                });
        }

        PostProcessPass::~PostProcessPass()
        {
            m_pipeline.reset();
        }

        void PostProcessPass::RecreateResources()
        {
            /* Empty */
        }

        void PostProcessPass::Draw(const Rhi::FrameInfo& frameInfo) const
        {
            assert(m_pipeline && "PostProcessPass: pipeline is null");

            auto& commandBuffer = frameInfo.m_commandBuffer;
            auto& frameIndex = frameInfo.m_frameIndex;
            auto& imageIndex = frameInfo.m_imageIndex;

            // Begin rendering
            {
                VkExtent2D extent = m_swapChain.Extent();

                Rhi::RenderingAttachmentDesc colorDesc{};
                colorDesc.m_imageView = m_swapChain.ImageView(imageIndex);
                colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
                colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;

                Rhi::RenderingScope scope(commandBuffer, extent, { colorDesc });

                // Bind pipeline
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

                // Draw
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    1, 1, &m_postProcess.GetSet(frameIndex), 0, nullptr);

                vkCmdDraw(commandBuffer, 3, 1, 0, 0);
            }
            // End rendering
        }

        void PostProcessPass::CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts)
        {
            Rhi::GraphicsPipelineBuilder builder(m_context.Device());
            builder.SetShaders("assets/shaders/post_process_vert.spv", "assets/shaders/post_process_frag.spv")
                .SetDescriptorSetLayouts(layouts)
                .SetDynamicRendering({ m_swapChain.Format() }, VK_FORMAT_UNDEFINED);
            m_pipeline = builder.Build();
        }
    }
}
