#pragma once

#include "resource/types.h"

#include <vulkan/vulkan.h>
#include <map>
#include <vector>

namespace Kita::Pbrv
{
    namespace Resource
    {
        class Resources;
    }

    namespace Rhi
    {
        namespace V2
        {
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
        };

        class DescriptorWriter
        {
        public:
            explicit DescriptorWriter(const Resource::Resources& resources, VkDevice device);
            ~DescriptorWriter();

            DescriptorWriter(const DescriptorWriter&) = delete;
            DescriptorWriter& operator=(const DescriptorWriter&) = delete;
            DescriptorWriter(DescriptorWriter&&) = delete;
            DescriptorWriter& operator=(DescriptorWriter&&) = delete;

            DescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorType type,
                Resource::RenderBufferHandle handle, VkDeviceSize offset, VkDeviceSize range);
            DescriptorWriter& WriteImage(uint32_t binding, VkDescriptorType type,
                VkImageLayout layout, Resource::RenderImageViewHandle imageViewHandle, Resource::RenderSamplerHandle samplerHandle);
            void UpdateSet(VkDescriptorSet set) const;

        private:
            struct BindingEntry
            {
                VkDescriptorType m_type{ VK_DESCRIPTOR_TYPE_MAX_ENUM };
                std::vector<VkDescriptorBufferInfo> m_bufferInfos{};
                std::vector<VkDescriptorImageInfo> m_imageInfos{};
            };

            BindingEntry& GetOrAddEntry(uint32_t binding, VkDescriptorType type);
            VkDescriptorBufferInfo CreateBufferInfo(Resource::RenderBufferHandle handle,
                VkDeviceSize offset,
                VkDeviceSize range) const;
            VkDescriptorImageInfo CreateImageInfo(VkImageLayout layout,
                Resource::RenderImageViewHandle imageViewHandle,
                Resource::RenderSamplerHandle samplerHandle) const;

        private:
            const Resource::Resources& m_resources;
            VkDevice m_device{ VK_NULL_HANDLE };

            std::map<uint32_t, BindingEntry> m_bindingEntries{};
        };
    }
}
