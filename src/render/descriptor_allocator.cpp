#include "descriptor_allocator.h"

#include "render/render_context.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    DescriptorAllocator::DescriptorAllocator(const RenderContext& context, const VkDescriptorPoolCreateInfo& createInfo)
        : m_context(context)
    {
        if (vkCreateDescriptorPool(m_context.Device(), &createInfo, nullptr, &m_pool) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create descriptor pool!");
        }
    }

    DescriptorAllocator::~DescriptorAllocator()
    {
        vkDestroyDescriptorPool(m_context.Device(), m_pool, nullptr);
    }

    VkDescriptorSet DescriptorAllocator::Allocate(VkDescriptorSetLayout layout) const
    {
        VkDescriptorSet set{};

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        if (vkAllocateDescriptorSets(m_context.Device(), &allocInfo, &set) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to allocate descriptor sets!");
        }

        return set;
    }
}