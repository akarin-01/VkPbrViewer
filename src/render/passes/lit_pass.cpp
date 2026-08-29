#include "lit_pass.h"
#include "rhi/context.h"
#include "rhi/graphics_pipeline.h"
#include "rhi/rendering_scope.h"
#include "rhi/swap_chain.h"
#include "render/vertex_input.h"
#include "render/render_scene.h"

#include <cassert>

namespace Kita::Pbrv
{
    namespace Render
    {
        LitPass::LitPass(const Rhi::Context& context,
            Resource::Resources& resources,
            const Rhi::SwapChain& swapChain,
            const RenderScene& scene)
            : RenderPassBase(context, resources, swapChain),
            m_target(scene.GetTarget()),
            m_frameData(scene.GetFrameData()),
            m_materialData(scene.GetMaterialData()),
            m_objectData(scene.GetObjectData()),
            m_meshData(scene.GetMeshData())
        {
            CreatePipeline(scene.GetEmptyLayout());
        }

        LitPass::~LitPass() = default;

        void LitPass::RecreateResources()
        {
            /* Empty */
        }

        void LitPass::Draw(const Rhi::FrameInfo& frameInfo) const
        {
            assert(m_pipeline && "LitPass: pipeline is null");

            auto& commandBuffer = frameInfo.m_commandBuffer;
            auto& frameIndex = frameInfo.m_frameIndex;

            // Begin rendering
            {
                std::array<VkClearValue, 2> clearValues{};
                clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
                clearValues[1].depthStencil = { 1.0f, 0 };
                VkExtent2D extent = m_swapChain.Extent();

                Rhi::RenderingAttachmentDesc colorDesc{};
                colorDesc.m_imageView = m_target.GetColorImageView();
                colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorDesc.m_resolveImageView = m_target.GetResolveImageView();
                colorDesc.m_resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                colorDesc.m_clearValue = clearValues[0];

                Rhi::RenderingAttachmentDesc depthDesc{};
                depthDesc.m_imageView = m_target.GetDepthImageView();
                depthDesc.m_imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                depthDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                depthDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                depthDesc.m_clearValue = clearValues[1];

                Rhi::RenderingScope scope(commandBuffer, extent, { colorDesc }, &depthDesc);

                // Bind pipeline
                vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

                // Draw
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    0, 1, &m_frameData.GetSet(frameIndex), 0, nullptr);
                {
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                        2, 1, &m_materialData.GetSet(frameIndex), 0, nullptr);
                    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                        3, 1, &m_objectData.GetSet(frameIndex), 0, nullptr);

                    auto meshHandle = m_meshData.m_mesh;
                    if (meshHandle)
                    {
                        VkBuffer buffers[]{ meshHandle->GetVertexBuffer() };
                        VkDeviceSize offsets[]{ 0 };
                        vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
                        vkCmdBindIndexBuffer(commandBuffer, meshHandle->GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                        vkCmdDrawIndexed(commandBuffer, meshHandle->GetIndexCount(), 1, 0, 0, 0);
                    }
                }
            }
            // End rendering
        }

        void LitPass::CreatePipeline(VkDescriptorSetLayout emptyLayout)
        {
            Rhi::GraphicsPipelineBuilder builder(m_context.Device());
            builder.SetShaders("assets/shaders/lit_vert.spv", "assets/shaders/lit_frag.spv")
                .SetVertexInput({ VertexInput::Binding() }, VertexInput::Attributes())
                .SetCullMode(VK_CULL_MODE_BACK_BIT)
                .SetRasterizationSamples(m_context.SampleCount())
                .SetDepth(true, true, VK_COMPARE_OP_LESS)
                .SetDescriptorSetLayouts(
                    {
                        m_frameData.GetSetLayout(),
                        emptyLayout,
                        m_materialData.GetSetLayout(),
                        m_objectData.GetSetLayout(),
                    })
                    .SetDynamicRendering({ m_target.GetColorFormat() }, m_target.GetDepthFormat());
            m_pipeline = builder.Build();
        }
    }
}
