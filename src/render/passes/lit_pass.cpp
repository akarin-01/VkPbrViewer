#include "lit_pass.h"

#include "core/log.h"

#include "scene/vertex.h"

#include "render/render_context.h"
#include "render/render_utils.h"
#include "render/render_resources.h"
#include "render/swap_chain.h"
#include "render/descriptor_allocator.h"
#include "render/data/render_frame_data.h"
#include "render/data/render_material_data.h"
#include "render/data/render_mesh_data.h"
#include "render/graphics_pipeline.h"

#include <stdexcept>
#include <array>
#include <cassert>

namespace Kita::Pbrv
{
    LitPass::LitPass(const RenderContext& context,
        RenderResources& resources,
        const SwapChain& swapChain,
        const DescriptorAllocator& descriptorAllocator,
        const RenderFrameData& frameData,
        const RenderMaterialData& materialCache,
        const RenderMeshData& meshCache,
        const RenderTarget& target)
        : RenderPassBase(context, resources, swapChain, descriptorAllocator, target),
        m_frameData(frameData),
        m_materialData(materialCache),
        m_meshData(meshCache)
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

        VkImageSubresourceRange colorRange{};
        colorRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        colorRange.baseMipLevel = 0;
        colorRange.levelCount = 1;
        colorRange.baseArrayLayer = 0;
        colorRange.layerCount = 1;

        // Color image: UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL
        RenderImage* colorImage = m_resources.GetImage(m_target.m_colorTex.m_imageHandle);
        assert(colorImage && "LitPass: Color image handle is invalid");
        TransitionImageLayout(commandBuffer,
            colorImage->m_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT,
            colorRange);

        VkImageSubresourceRange depthRange{};
        depthRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        depthRange.baseMipLevel = 0;
        depthRange.levelCount = 1;
        depthRange.baseArrayLayer = 0;
        depthRange.layerCount = 1;

        // Depth image: UNDEFINED -> DEPTH_STENCIL_ATTACHMENT_OPTIMAL
        auto depthImage = m_resources.GetImage(m_target.m_depthTex.m_imageHandle);
        assert(depthImage && "LitPass: Depth image handle is invalid");
        TransitionImageLayout(commandBuffer,
            depthImage->m_image,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
            VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT, VK_ACCESS_2_NONE,
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
            depthRange);

        // Begin rendering
        std::array<VkClearValue, 2> clearValues{};
        clearValues[0].color = { 0.0f, 0.0f, 0.0f, 1.0f };
        clearValues[1].depthStencil = { 1.0f, 0 };
        VkExtent2D extent = m_swapChain.Extent();

        RenderImageView* colorImageView = m_resources.GetImageView(m_target.m_colorTex.m_imageViewHandle);
        assert(colorImageView && "LitPass: Color image view handle is invalid");
        VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.imageView = colorImageView->m_imageView;
        colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
        colorAttachment.resolveImageView = VK_NULL_HANDLE;
        colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.clearValue = clearValues[0];

        RenderImageView* depthImageView = m_resources.GetImageView(m_target.m_depthTex.m_imageViewHandle);
        assert(depthImageView && "LitPass: Depth image view handle is invalid");
        VkRenderingAttachmentInfo depthAttachment{};
        depthAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        depthAttachment.imageView = depthImageView->m_imageView;
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue = clearValues[1];

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
        {
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline->Layout(),
                1, 1, &m_materialData.GetSet(frameIndex), 0, nullptr);

            auto& pushConstant = m_materialData.GetPushConstant();
            vkCmdPushConstants(commandBuffer, m_pipeline->Layout(),
                VK_SHADER_STAGE_FRAGMENT_BIT,
                0, sizeof(pushConstant), &pushConstant);

            if (m_meshData.GetIndexCount() != 0)
            {
                RenderBuffer* vertexBuffer = m_resources.GetBuffer(m_meshData.GetVertexBufferHandle());
                assert(vertexBuffer && "Vertex buffer handle is invalid");
                VkBuffer buffers[]{ vertexBuffer->m_buffer };
                VkDeviceSize offsets[]{ 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

                RenderBuffer* indexBuffer = m_resources.GetBuffer(m_meshData.GetIndexBufferHandle());
                assert(indexBuffer && "Index buffer handle is invalid");
                vkCmdBindIndexBuffer(commandBuffer, indexBuffer->m_buffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdDrawIndexed(commandBuffer, m_meshData.GetIndexCount(), 1, 0, 0, 0);
            }
        }

        // End rendering
        vkCmdEndRendering(commandBuffer);
    }

    void LitPass::CreatePipeline()
    {
        VkPushConstantRange pushConstant{};
        pushConstant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstant.offset = 0;
        pushConstant.size = sizeof(MaterialPC);

        GraphicsPipelineBuilder builder(m_context.Device());
        builder.SetShaders("assets/shaders/lit_vert.spv", "assets/shaders/lit_frag.spv")
            .SetVertexInput({ Vertex::GetBindingDescription() }, Vertex::GetAttributeDescriptions())
            .SetCullMode(VK_CULL_MODE_BACK_BIT)
            .SetDepth(true, true, VK_COMPARE_OP_LESS)
            .SetDescriptorSetLayouts({ m_frameData.GetSetLayout(), m_materialData.GetSetLayout(), })
            .SetPushConstants({ pushConstant })
            .SetDynamicRendering({ m_target.m_colorFormat }, m_target.m_depthFormat);
        m_pipeline = builder.Build();
    }
}
