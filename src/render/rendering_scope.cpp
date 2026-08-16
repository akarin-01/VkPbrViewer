#include "rendering_scope.h"

#include <optional>

namespace Kita::Pbrv
{
    RenderingScope::RenderingScope(VkCommandBuffer commandBuffer, VkExtent2D extent, const std::vector<RenderingAttachmentDesc>& colorDescs, const RenderingAttachmentDesc* depthDesc)
        : m_commandBuffer(commandBuffer)
    {
        std::vector<VkRenderingAttachmentInfo> colorInfos(colorDescs.size());
        for (size_t i = 0; i < colorInfos.size(); ++i)
        {
            auto& colorInfo = colorInfos[i];
            auto& colorDesc = colorDescs[i];

            colorInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            colorInfo.imageView = colorDesc.m_imageView;
            colorInfo.imageLayout = colorDesc.m_imageLayout;
            colorInfo.loadOp = colorDesc.m_loadOp;
            colorInfo.storeOp = colorDesc.m_storeOp;
            colorInfo.clearValue = colorDesc.m_clearValue;
        }

        std::optional<VkRenderingAttachmentInfo> depthInfo{};
        if (depthDesc)
        {
            VkRenderingAttachmentInfo attachmentInfo{};
            attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
            attachmentInfo.imageView = depthDesc->m_imageView;
            attachmentInfo.imageLayout = depthDesc->m_imageLayout;
            attachmentInfo.loadOp = depthDesc->m_loadOp;
            attachmentInfo.storeOp = depthDesc->m_storeOp;
            attachmentInfo.clearValue = depthDesc->m_clearValue;
            depthInfo = attachmentInfo;
        }

        VkRenderingInfo renderingInfo{};
        renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO_KHR;
        renderingInfo.renderArea.offset = { 0, 0 };
        renderingInfo.renderArea.extent = extent;
        renderingInfo.layerCount = 1;
        renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorInfos.size());
        renderingInfo.pColorAttachments = colorInfos.data();
        renderingInfo.pDepthAttachment = depthInfo ? &(*depthInfo) : nullptr;

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
    }

    RenderingScope::~RenderingScope()
    {
        vkCmdEndRendering(m_commandBuffer);
    }
}