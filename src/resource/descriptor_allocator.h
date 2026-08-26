#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class RenderContext;
    }

    namespace Resource
    {
        class DescriptorAllocator
        {
        public:
            DescriptorAllocator(const Rhi::RenderContext& context, const VkDescriptorPoolCreateInfo& createInfo);
            ~DescriptorAllocator();

            VkDescriptorSet Allocate(VkDescriptorSetLayout layout, const char* debugName) const;

        private:
            const Rhi::RenderContext& m_context;

            VkDescriptorPool m_pool{ VK_NULL_HANDLE };
            uint32_t m_maxSetCount{ 0 };
            mutable uint32_t m_usedSetCount{ 0 };
        };
    }
}
