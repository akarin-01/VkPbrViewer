#pragma once

#include <map>
#include <vector>
#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Resource
    {
        /// Batches descriptor writes for one descriptor set, then updates it.
        /// Takes raw Vk handles: resources are already resolved by the caller.
        class DescriptorWriter
        {
        public:
            explicit DescriptorWriter(VkDevice device);
            ~DescriptorWriter();

            DescriptorWriter(const DescriptorWriter&) = delete;
            DescriptorWriter& operator=(const DescriptorWriter&) = delete;
            DescriptorWriter(DescriptorWriter&&) = delete;
            DescriptorWriter& operator=(DescriptorWriter&&) = delete;

            DescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorType type,
                VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range);
            DescriptorWriter& WriteImage(uint32_t binding, VkDescriptorType type,
                VkImageLayout layout, VkImageView imageView, VkSampler sampler);
            void UpdateSet(VkDescriptorSet set) const;

        private:
            struct BindingEntry
            {
                VkDescriptorType m_type{ VK_DESCRIPTOR_TYPE_MAX_ENUM };
                std::vector<VkDescriptorBufferInfo> m_bufferInfos{};
                std::vector<VkDescriptorImageInfo> m_imageInfos{};
            };

            BindingEntry& GetOrAddEntry(uint32_t binding, VkDescriptorType type);

        private:
            VkDevice m_device{ VK_NULL_HANDLE };

            std::map<uint32_t, BindingEntry> m_bindingEntries{};
        };
    }
}
