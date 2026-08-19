#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderContext;

    class OneShotCommand
    {
    public:
        OneShotCommand(const RenderContext& context);
        ~OneShotCommand();

        OneShotCommand(const OneShotCommand&) = delete;
        OneShotCommand& operator=(const OneShotCommand&) = delete;
        OneShotCommand(OneShotCommand&&) = delete;
        OneShotCommand& operator=(OneShotCommand&&) = delete;

        VkCommandBuffer Handle() const { return m_commandBuffer; }

    private:
        const RenderContext& m_context;

        VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
    };
}
