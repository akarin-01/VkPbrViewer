#include "one_shot_command.h"

#include "core/log.h"
#include "render/render_context.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    OneShotCommand::OneShotCommand(const RenderContext& context)
        : m_context(context)
    {
        // Allocate command buffer
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_context.CommandPool();
        allocInfo.commandBufferCount = 1;

        if (vkAllocateCommandBuffers(m_context.Device(), &allocInfo, &m_commandBuffer))
        {
            throw std::runtime_error("Failed to allocate one-shot command buffer!");
        }

        // Begin command buffer
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        if (vkBeginCommandBuffer(m_commandBuffer, &beginInfo))
        {
            throw std::runtime_error("Failed to begin one-shot command buffer!");
        }
    }

    OneShotCommand::~OneShotCommand()
    {
        // End command buffer
        if (vkEndCommandBuffer(m_commandBuffer))
        {
            Log::Error("[Resources] End one-shot command buffer failed");
            return;
        }

        // Submit commands
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffer;

        if (vkQueueSubmit(m_context.GraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE))
        {
            Log::Error("[Resources] Submit one-shot command buffer failed");
            return;
        }

        vkQueueWaitIdle(m_context.GraphicsQueue());

        // Free command buffer
        vkFreeCommandBuffers(m_context.Device(), m_context.CommandPool(), 1, &m_commandBuffer);
    }
}
