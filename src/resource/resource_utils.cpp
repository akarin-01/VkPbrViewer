#include "resource_utils.h"

#include "rhi/context.h"
#include "rhi/one_shot_command.h"
#include "rhi/utils.h"
#include "resource/resource_types.h"

#include <cstring>
#include <stdexcept>
#include <cassert>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace
        {
            BufferResource CreateBufferHelper(const Rhi::Context& context, const BufferDesc& desc)
            {
                BufferResource buffer{};
                buffer.m_size = desc.m_size;

                VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
                bufferInfo.size = desc.m_size;
                bufferInfo.usage = desc.m_usage;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                if (vkCreateBuffer(context.Device(), &bufferInfo, nullptr, &buffer.m_buffer) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create buffer!");
                }

                VkMemoryRequirements req{};
                vkGetBufferMemoryRequirements(context.Device(), buffer.m_buffer, &req);

                VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                allocInfo.allocationSize = req.size;
                allocInfo.memoryTypeIndex = Rhi::FindMemoryType(context.PhysicalDevice(), req.memoryTypeBits, desc.m_properties);
                if (vkAllocateMemory(context.Device(), &allocInfo, nullptr, &buffer.m_memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to allocate buffer memory!");
                }

                vkBindBufferMemory(context.Device(), buffer.m_buffer, buffer.m_memory, 0);

                if (desc.m_mapped)
                {
                    vkMapMemory(context.Device(), buffer.m_memory, 0, desc.m_size, 0, &buffer.m_mapped);
                }

                return buffer;
            }
        }

        namespace ResourceUtils
        {
            BufferResource CreateBufferResource(const Rhi::Context& context,
                BufferDesc desc, const void* data, size_t size)
            {
                assert((data == nullptr) == (size == 0) && "CreateBuffer: data and size must agree");

                if (!data)
                {
                    // Non-data buffer
                    return CreateBufferHelper(context, desc);
                }

                const bool hostVisible = (desc.m_properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

                if (hostVisible)
                {
                    // Host-visible: map and write directly into memory
                    assert(desc.m_mapped && "CreateBuffer: host visible must map");

                    BufferResource buffer = CreateBufferHelper(context, desc);
                    std::memcpy(buffer.m_mapped, data, size);
                    return buffer;
                }

                // Device-local: add the transfer flag and upload through a staging buffer.
                desc.m_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                BufferResource buffer = CreateBufferHelper(context, desc);

                BufferDesc stagingDesc{};
                stagingDesc.m_size = size;
                stagingDesc.m_usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                stagingDesc.m_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
                stagingDesc.m_mapped = true;
                BufferResource staging = CreateBufferResource(context, stagingDesc, data, size);

                {
                    Rhi::OneShotCommand cmd(context);
                    VkBufferCopy copy{};
                    copy.size = size;
                    vkCmdCopyBuffer(cmd.Handle(), staging.m_buffer, buffer.m_buffer, 1, &copy);
                }

                DestroyBufferResource(context, staging);

                return buffer;
            }

            void DestroyBufferResource(const Rhi::Context& context, BufferResource& data)
            {
                if (data.m_mapped)
                {
                    vkUnmapMemory(context.Device(), data.m_memory);
                }
                vkDestroyBuffer(context.Device(), data.m_buffer, nullptr);
                vkFreeMemory(context.Device(), data.m_memory, nullptr);

                data = {};
            }
        }
    }
}
