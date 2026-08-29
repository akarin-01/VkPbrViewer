#include "descriptor_writer.h"

#include "resource/resources.h"

#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        namespace V2
        {
            DescriptorWriter::DescriptorWriter(VkDevice device)
                : m_device(device)
            {
            }

            DescriptorWriter::~DescriptorWriter() = default;

            DescriptorWriter& DescriptorWriter::WriteBuffer(uint32_t binding, VkDescriptorType type,
                VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range)
            {
                BindingEntry& entry = GetOrAddEntry(binding, type);

                if (!entry.m_imageInfos.empty())
                {
                    throw std::runtime_error("DescriptorWriter: binding already wrote image descriptors");
                }

                VkDescriptorBufferInfo info{};
                info.buffer = buffer;
                info.offset = offset;
                info.range = range;
                entry.m_bufferInfos.push_back(std::move(info));

                return *this;
            }

            DescriptorWriter& DescriptorWriter::WriteImage(uint32_t binding, VkDescriptorType type,
                VkImageLayout layout, VkImageView imageView, VkSampler sampler)
            {
                BindingEntry& entry = GetOrAddEntry(binding, type);

                if (!entry.m_bufferInfos.empty())
                {
                    throw std::runtime_error("DescriptorWriter: binding already wrote buffer descriptors");
                }

                VkDescriptorImageInfo info{};
                info.imageLayout = layout;
                info.imageView = imageView;
                info.sampler = sampler;
                entry.m_imageInfos.push_back(std::move(info));

                return *this;
            }

            void DescriptorWriter::UpdateSet(VkDescriptorSet set) const
            {
                std::vector<VkWriteDescriptorSet> writes{};
                writes.reserve(m_bindingEntries.size());

                for (const auto& [binding, entry] : m_bindingEntries)
                {
                    VkWriteDescriptorSet write{};
                    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    write.dstSet = set;
                    write.dstBinding = binding;
                    write.dstArrayElement = 0;
                    write.descriptorType = entry.m_type;

                    if (!entry.m_bufferInfos.empty())
                    {
                        write.descriptorCount = static_cast<uint32_t>(entry.m_bufferInfos.size());
                        write.pBufferInfo = entry.m_bufferInfos.data();
                    }
                    else if (!entry.m_imageInfos.empty())
                    {
                        write.descriptorCount = static_cast<uint32_t>(entry.m_imageInfos.size());
                        write.pImageInfo = entry.m_imageInfos.data();
                    }

                    writes.push_back(write);
                }

                vkUpdateDescriptorSets(m_device,
                    static_cast<uint32_t>(writes.size()), writes.data(),
                    0, nullptr);
            }

            DescriptorWriter::BindingEntry& DescriptorWriter::GetOrAddEntry(uint32_t binding, VkDescriptorType type)
            {
                auto [it, inserted] = m_bindingEntries.try_emplace(binding);
                BindingEntry& entry = it->second;

                if (inserted)
                {
                    entry.m_type = type;
                }
                else if (entry.m_type != type)
                {
                    throw std::runtime_error("DescriptorWriter: binding already wrote with a different type");
                }

                return entry;
            }
        }

        DescriptorWriter::DescriptorWriter(const Resource::Resources& resources, VkDevice device)
            : m_resources(resources), m_device(device)
        {
        }

        DescriptorWriter::~DescriptorWriter() = default;


        DescriptorWriter& DescriptorWriter::WriteBuffer(uint32_t binding, VkDescriptorType type, Resource::RenderBufferHandle handle, VkDeviceSize offset, VkDeviceSize range)
        {
            BindingEntry& entry = GetOrAddEntry(binding, type);

            if (!entry.m_imageInfos.empty())
            {
                throw std::runtime_error("DescriptorWriter: binding already wrote image descriptors");
            }

            entry.m_bufferInfos.push_back(CreateBufferInfo(handle, offset, range));

            return *this;
        }

        DescriptorWriter& DescriptorWriter::WriteImage(uint32_t binding, VkDescriptorType type, VkImageLayout layout, Resource::RenderImageViewHandle imageViewHandle, Resource::RenderSamplerHandle samplerHandle)
        {
            BindingEntry& entry = GetOrAddEntry(binding, type);

            if (!entry.m_bufferInfos.empty())
            {
                throw std::runtime_error("DescriptorWriter: binding already wrote buffer descriptors");
            }

            entry.m_imageInfos.push_back(CreateImageInfo(layout, imageViewHandle, samplerHandle));

            return *this;
        }

        void DescriptorWriter::UpdateSet(VkDescriptorSet set) const
        {
            std::vector<VkWriteDescriptorSet> writes{};
            writes.reserve(m_bindingEntries.size());

            for (const auto& [binding, entry] : m_bindingEntries)
            {
                VkWriteDescriptorSet write{};
                write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                write.dstSet = set;
                write.dstBinding = binding;
                write.dstArrayElement = 0;
                write.descriptorType = entry.m_type;

                if (!entry.m_bufferInfos.empty())
                {
                    write.descriptorCount = static_cast<uint32_t>(entry.m_bufferInfos.size());
                    write.pBufferInfo = entry.m_bufferInfos.data();
                }
                else if (!entry.m_imageInfos.empty())
                {
                    write.descriptorCount = static_cast<uint32_t>(entry.m_imageInfos.size());
                    write.pImageInfo = entry.m_imageInfos.data();
                }

                writes.push_back(write);
            }

            vkUpdateDescriptorSets(m_device,
                static_cast<uint32_t>(writes.size()), writes.data(),
                0, nullptr);
        }

        DescriptorWriter::BindingEntry& DescriptorWriter::GetOrAddEntry(uint32_t binding, VkDescriptorType type)
        {
            auto [it, inserted] = m_bindingEntries.try_emplace(binding);
            BindingEntry& entry = it->second;

            if (inserted)
            {
                entry.m_type = type;
            }
            else if (entry.m_type != type)
            {
                throw std::runtime_error("DescriptorWriter: binding already wrote with a different type");
            }

            return entry;
        }

        VkDescriptorBufferInfo DescriptorWriter::CreateBufferInfo(Resource::RenderBufferHandle handle, VkDeviceSize offset, VkDeviceSize range) const
        {
            Resource::RenderBuffer* buffer = m_resources.GetBuffer(handle);
            if (!buffer)
            {
                throw std::runtime_error("DescriptorWriter: invalid buffer handle");
            }

            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = buffer->m_buffer;
            bufferInfo.offset = offset;
            bufferInfo.range = range;

            return bufferInfo;
        }

        VkDescriptorImageInfo DescriptorWriter::CreateImageInfo(VkImageLayout layout, Resource::RenderImageViewHandle imageViewHandle, Resource::RenderSamplerHandle samplerHandle) const
        {
            Resource::RenderImageView* imageView = m_resources.GetImageView(imageViewHandle);
            if (!imageView)
            {
                throw std::runtime_error("DescriptorWriter: invalid image view handle");
            }

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = layout;
            imageInfo.imageView = imageView->m_imageView;

            if (samplerHandle != 0)
            {
                Resource::RenderSampler* sampler = m_resources.GetSampler(samplerHandle);
                if (!sampler)
                {
                    throw std::runtime_error("DescriptorWriter: invalid sampler handle");
                }
                imageInfo.sampler = sampler->m_sampler;
            }

            return imageInfo;
        }
    }
}
