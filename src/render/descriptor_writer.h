#pragma once

#include "render/render_resource_types.h"

#include <vulkan/vulkan.h>
#include <map>
#include <vector>

namespace Kita::Pbrv
{
    class RenderResources;

    class DescriptorWriter
    {
    public:
        explicit DescriptorWriter(const RenderResources& resources, VkDevice device);
        ~DescriptorWriter();

        DescriptorWriter(const DescriptorWriter&) = delete;
        DescriptorWriter& operator=(const DescriptorWriter&) = delete;
        DescriptorWriter(DescriptorWriter&&) = delete;
        DescriptorWriter& operator=(DescriptorWriter&&) = delete;

        DescriptorWriter& WriteBuffer(uint32_t binding, VkDescriptorType type,
            RenderBufferHandle handle, VkDeviceSize offset, VkDeviceSize range);
        DescriptorWriter& WriteImage(uint32_t binding, VkDescriptorType type,
            VkImageLayout layout, RenderImageViewHandle imageViewHandle, RenderSamplerHandle samplerHandle);
        void UpdateSet(VkDescriptorSet set) const;

    private:
        struct BindingEntry
        {
            VkDescriptorType m_type{ VK_DESCRIPTOR_TYPE_MAX_ENUM };
            std::vector<VkDescriptorBufferInfo> m_bufferInfos{};
            std::vector<VkDescriptorImageInfo> m_imageInfos{};
        };

        BindingEntry& GetOrAddEntry(uint32_t binding, VkDescriptorType type);
        VkDescriptorBufferInfo CreateBufferInfo(RenderBufferHandle handle,
            VkDeviceSize offset,
            VkDeviceSize range) const;
        VkDescriptorImageInfo CreateImageInfo(VkImageLayout layout,
            RenderImageViewHandle imageViewHandle,
            RenderSamplerHandle samplerHandle) const;

    private:
        const RenderResources& m_resources;
        VkDevice m_device{ VK_NULL_HANDLE };

        std::map<uint32_t, BindingEntry> m_bindingEntries{};
    };
}
