#pragma once

#include <vulkan/vulkan.h>
#include <vector>

namespace Kita::Pbrv
{
    struct RenderingAttachmentDesc
    {
        VkImageView m_imageView{ VK_NULL_HANDLE };
        VkImageLayout m_imageLayout{ VK_IMAGE_LAYOUT_UNDEFINED };
        VkAttachmentLoadOp m_loadOp{ VK_ATTACHMENT_LOAD_OP_LOAD };
        VkAttachmentStoreOp m_storeOp{ VK_ATTACHMENT_STORE_OP_STORE };
        VkClearValue m_clearValue{};
    };

    class RenderingScope
    {
    public:
        RenderingScope(VkCommandBuffer commandBuffer,
            VkExtent2D extent,
            const std::vector<RenderingAttachmentDesc>& colorDescs,
            const RenderingAttachmentDesc* depthDesc = nullptr);
        ~RenderingScope();

        RenderingScope(const RenderingScope& other) = delete;
        RenderingScope& operator=(const RenderingScope& other) = delete;
        RenderingScope(RenderingScope&& other) = delete;
        RenderingScope& operator=(RenderingScope&& other) = delete;

    private:
        VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
    };
}