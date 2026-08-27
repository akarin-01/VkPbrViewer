#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;

        class OneShotCommand
        {
        public:
            OneShotCommand(const Context& context);
            ~OneShotCommand();

            OneShotCommand(const OneShotCommand&) = delete;
            OneShotCommand& operator=(const OneShotCommand&) = delete;
            OneShotCommand(OneShotCommand&&) = delete;
            OneShotCommand& operator=(OneShotCommand&&) = delete;

            VkCommandBuffer Handle() const { return m_commandBuffer; }

        private:
            const Context& m_context;

            VkCommandBuffer m_commandBuffer{ VK_NULL_HANDLE };
        };
    }
}
