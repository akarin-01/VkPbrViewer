#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        struct FrameInfo
        {
            bool m_swapChainRecreated{ false };
            VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
            uint32_t m_frameIndex{ 0 };
            uint32_t m_imageIndex{ 0 };
        };
    }
}
