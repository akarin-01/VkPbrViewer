#include "shadow_pass.h"

#include "rhi/context.h"
#include "rhi/graphics_pipeline.h"
#include "rhi/rendering_scope.h"
#include "rhi/swap_chain.h"
#include "render/render_scene.h"
#include "render/vertex_input.h"

#include <cassert>

namespace Kita::Pbrv
{
    namespace Render
    {
        namespace
        {
            constexpr float kDepthBiasConstant = 1.25f;
            constexpr float kDepthBiasSlope = 1.75f;
        }

        ShadowPass::ShadowPass(const Rhi::Context& context,
            const Rhi::SwapChain& swapChain,
            const RenderScene& scene)
            : RenderPassBase(context, swapChain),
            m_shadow(scene.GetShadow()),
            m_frame(scene.GetFrame()),
            m_objects(scene.GetObjects())
        {
            CreatePipeline(
                {
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::PerFrame),
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::Empty),
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::Empty),
                    scene.GetDescriptorSetLayout(Resource::DescriptorSetRhi::Type::PerObject),
                });
        }

        ShadowPass::~ShadowPass()
        {
            m_pipeline.reset();
        }

        void ShadowPass::RecreateResources()
        {
            /* Empty */
        }

        void ShadowPass::Draw(const Rhi::FrameInfo& frameInfo) const
        {
            assert(m_pipeline && "ShadowPass: pipeline is null");

            auto& commandBuffer = frameInfo.m_commandBuffer;
            auto& frameIndex = frameInfo.m_frameIndex;

            // Begin rendering
            {
                VkClearValue clearValue{};
                clearValue = { 1.0f, 0 };
                VkExtent2D extent{
                    m_shadow.GetExtent().width,
                    m_shadow.GetExtent().height
                };

                Rhi::RenderingAttachmentDesc depthDesc{};
                depthDesc.m_imageView = m_shadow.GetImageView();
                depthDesc.m_imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthDesc.m_clearValue = clearValue;

                Rhi::RenderingScope scope(commandBuffer, extent, {}, &depthDesc);

                // Bind pipeline
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

                // Draw
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    0, 1, &m_frame.GetSet(frameIndex), 0, nullptr);

                for (auto& object : m_objects)
                {
                    if (!object.HasMesh())
                    {
                        continue;
                    }

                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                        3, 1, &object.GetObjectSet(frameIndex), 0, nullptr);

                    VkBuffer buffers[]{ object.GetVertexBuffer() };
                    VkDeviceSize offsets[]{ 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
                    vkCmdBindIndexBuffer(commandBuffer, object.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);
                    vkCmdDrawIndexed(commandBuffer, object.GetIndexCount(), 1, 0, 0, 0);
                }
            }
            // End rendering
        }

        void ShadowPass::CreatePipeline(const std::vector<VkDescriptorSetLayout>& layouts)
        {
            Rhi::GraphicsPipelineBuilder builder(m_context.Device());
            builder.SetShaders("assets/shaders/shadow_vert.spv", "assets/shaders/shadow_frag.spv")
                .SetVertexInput({ VertexInput::Binding() }, VertexInput::Attributes())
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetRasterizationSamples(VK_SAMPLE_COUNT_1_BIT)
                .SetDepth(true, true, VK_COMPARE_OP_LESS)
                .SetDepthBias(true, kDepthBiasConstant, 0.0f, kDepthBiasSlope)
                .SetDescriptorSetLayouts(layouts)
                .SetDynamicRendering({}, m_context.ShadowMapFormat());
            m_pipeline = builder.Build();
        }
    }
}
