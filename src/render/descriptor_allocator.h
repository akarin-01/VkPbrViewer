#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderContext;

    class DescriptorAllocator
    {
    public:
        DescriptorAllocator(const RenderContext& context, const VkDescriptorPoolCreateInfo& createInfo);
        ~DescriptorAllocator();

        VkDescriptorSet Allocate(VkDescriptorSetLayout layout) const;

    private:
        const RenderContext& m_context;

        VkDescriptorPool m_pool{ VK_NULL_HANDLE };
    };
}