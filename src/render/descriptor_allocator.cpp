#include "descriptor_allocator.h"

#include "core/log.h"
#include "render/render_context.h"

#include <stdexcept>
#include <string>

namespace Kita::Pbrv
{
    namespace
    {
        const char* ToString(VkResult result)
        {
            switch (result)
            {
            case VK_ERROR_OUT_OF_POOL_MEMORY:   return "OUT_OF_POOL_MEMORY";
            case VK_ERROR_OUT_OF_HOST_MEMORY:   return "OUT_OF_HOST_MEMORY";
            case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "OUT_OF_DEVICE_MEMORY";
            case VK_ERROR_FRAGMENTED_POOL:      return "FRAGMENTED_POOL";
            default:                            return "UNKNOWN";
            }
        }
    }

    DescriptorAllocator::DescriptorAllocator(const RenderContext& context, const VkDescriptorPoolCreateInfo& createInfo)
        : m_context(context), m_maxSetCount(createInfo.maxSets)
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

    VkDescriptorSet DescriptorAllocator::Allocate(VkDescriptorSetLayout layout, const char* debugName) const
    {
        if (m_usedSetCount >= m_maxSetCount)
        {
            Log::Error("[Resources] Descriptor pool exhausted: allocated ", m_usedSetCount, " / ",
                m_maxSetCount, " sets. -> Increase budget in Renderer::CreateDescriptorAllocator");
            throw std::runtime_error("Descriptor pool exhausted (maxSets = " + std::to_string(m_maxSetCount) + ")");
        }

        VkDescriptorSet set{};

        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkResult result = vkAllocateDescriptorSets(m_context.Device(), &allocInfo, &set);
        if (result != VK_SUCCESS)
        {
            Log::Error("Failed to allocate descriptor set: ", debugName, " (", ToString(result), ")");
            throw std::runtime_error("Failed to allocate descriptor set: " + std::string(debugName));
        }

        ++m_usedSetCount;

        KITA_LOG_DEBUG("[Resources] Allocate descriptor set: ", debugName);
        return set;
    }
}