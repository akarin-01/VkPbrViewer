#include "lit_pass.h"

#include "core/log.h"

#include "render/render_context.h"
#include "render/swap_chain.h"

#include "render/data/render_target_data.h"
#include "render/data/render_frame_data.h"
#include "render/data/render_material_data.h"
#include "render/data/render_mesh_data.h"
#include "render/data/render_ibl_data.h"

#include "render/graphics_pipeline.h"
#include "render/rendering_scope.h"

#include <cassert>

namespace Kita::Pbrv
{
    LitPass::LitPass(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        RenderTargetData& targetData,
        const RenderFrameData& frameData,
        const RenderMaterialData& materialData,
        const RenderMeshData& meshData,
        const RenderIblData& iblData)
        : RenderPassBase(context, resources, swapChain),
        m_targetData(targetData),
        m_frameData(frameData),
        m_materialData(materialData),
        m_meshData(meshData),
        m_iblData(iblData)
    {
        CreatePipeline();
    }

    LitPass::~LitPass() = default;

    void LitPass::RecreateResources()
    {
        /* Empty */
    }

    void LitPass::Draw(const FrameInfo& frameInfo) const
    {
        assert(m_pipeline && "LitPass: pipeline is null");

        auto& commandBuffer = frameInfo.m_commandBuffer;
        auto& frameIndex = frameInfo.m_frameIndex;

        // Color image -> COLOR_ATTACHMENT_OPTIMAL
        m_targetData.TransitionColorImageLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT);

        // Depth image -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        m_targetData.TransitionDepthImageLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);

        // Begin rendering
        {
            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
            clearValues[1].depthStencil = { 1.0f, 0 };
            VkExtent2D extent = m_swapChain.Extent();

            RenderingAttachmentDesc colorDesc{};
            colorDesc.m_imageView = m_targetData.GetColorImageView();
            colorDesc.m_imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            colorDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            colorDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorDesc.m_clearValue = clearValues[0];

            RenderingAttachmentDesc depthDesc{};
            depthDesc.m_imageView = m_targetData.GetDepthImageView();
            depthDesc.m_imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depthDesc.m_loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthDesc.m_storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            depthDesc.m_clearValue = clearValues[1];

            RenderingScope scope(commandBuffer, extent, { colorDesc }, &depthDesc);

            // Bind pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Handle());

            // Draw
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                0, 1, &m_frameData.GetSet(frameIndex), 0, nullptr);
            {
                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    1, 1, &m_materialData.GetSet(frameIndex), 0, nullptr);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                    2, 1, &m_iblData.GetSet(frameIndex), 0, nullptr);

                auto& pushConstant = m_materialData.GetPushConstant();
                vkCmdPushConstants(commandBuffer, m_pipeline->Layout(),
                    VK_SHADER_STAGE_FRAGMENT_BIT,
                    0, sizeof(pushConstant), &pushConstant);

                if (!m_meshData.IsEmpty())
                {
                    VkBuffer buffers[]{ m_meshData.GetVertexBuffer() };
                    VkDeviceSize offsets[]{ 0 };
                    vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
                    vkCmdBindIndexBuffer(commandBuffer, m_meshData.GetIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

                    vkCmdDrawIndexed(commandBuffer, m_meshData.GetIndexCount(), 1, 0, 0, 0);
                }
            }
        }
        // End rendering
    }

    void LitPass::CreatePipeline()
    {
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(MaterialPC);

        GraphicsPipelineBuilder builder(m_context.Device());
        builder.SetShaders("assets/shaders/lit_vert.spv", "assets/shaders/lit_frag.spv")
            .SetVertexInput({ RenderMeshData::GetVertexBinding() }, RenderMeshData::GetVertexAttributes())
            .SetCullMode(VK_CULL_MODE_BACK_BIT)
            .SetRasterizationSamples(m_context.SampleCount())
            .SetDepth(true, true, VK_COMPARE_OP_LESS)
            .SetDescriptorSetLayouts({ m_frameData.GetSetLayout(), m_materialData.GetSetLayout(), m_iblData.GetSetLayout() })
            .SetPushConstants({ pushConstant })
            .SetDynamicRendering({ m_targetData.GetColorFormat() }, m_targetData.GetDepthFormat());
        m_pipeline = builder.Build();
    }
}
