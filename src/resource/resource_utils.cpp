#include "resource_utils.h"

#include "resource_types.h"
#include "rhi/context.h"
#include "rhi/one_shot_command.h"
#include "rhi/utils.h"

#include <cstring>
#include <stdexcept>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace ResourceUtils
        {
            BufferResource CreateBufferData(const Rhi::Context& context,
                VkDeviceSize size, VkBufferUsageFlags usages, VkMemoryPropertyFlags properties,
                bool mapped)
            {
                BufferResource buffer{};

                VkBufferCreateInfo bufferInfo{ VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
                bufferInfo.size = size;
                bufferInfo.usage = usages;
                bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
                if (vkCreateBuffer(context.Device(), &bufferInfo, nullptr, &buffer.m_buffer) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to create buffer!");
                }

                VkMemoryRequirements req{};
                vkGetBufferMemoryRequirements(context.Device(), buffer.m_buffer, &req);

                VkMemoryAllocateInfo allocInfo{ VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
                allocInfo.allocationSize = req.size;
                allocInfo.memoryTypeIndex = Rhi::FindMemoryType(context.PhysicalDevice(), req.memoryTypeBits, properties);
                if (vkAllocateMemory(context.Device(), &allocInfo, nullptr, &buffer.m_memory) != VK_SUCCESS)
                {
                    throw std::runtime_error("Failed to allocate buffer memory!");
                }

                vkBindBufferMemory(context.Device(), buffer.m_buffer, buffer.m_memory, 0);

                if (mapped)
                {
                    vkMapMemory(context.Device(), buffer.m_memory, 0, size, 0, &buffer.m_mapped);
                }

                buffer.m_size = size;
                return buffer;
            }

            BufferResource CreateBufferData(const Rhi::Context& context,
                VkDeviceSize size, VkBufferUsageFlags usages, VkMemoryPropertyFlags properties,
                const void* data, size_t dataSize)
            {
                const bool hostVisible = (properties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);

                // Host-visible: map and write directly into memory.
                if (hostVisible)
                {
                    BufferResource buffer = CreateBufferData(context, size, usages, properties, true);
                    std::memcpy(buffer.m_mapped, data, dataSize);
                    return buffer;
                }

                // Device-local: add the transfer flag and upload through a staging buffer.
                BufferResource buffer = CreateBufferData(context, size,
                    usages | VK_BUFFER_USAGE_TRANSFER_DST_BIT, properties, false);

                BufferResource staging = CreateBufferData(context, dataSize,
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                    data, dataSize);

                {
                    Rhi::OneShotCommand cmd(context);
                    VkBufferCopy copy{};
                    copy.size = dataSize;
                    vkCmdCopyBuffer(cmd.Handle(), staging.m_buffer, buffer.m_buffer, 1, &copy);
                }

                DestroyBufferData(context, staging);   // synchronous: staging is no longer needed

                return buffer;
            }

            void DestroyBufferData(const Rhi::Context& context, BufferResource& data)
            {
                if (data.m_mapped)
                {
                    vkUnmapMemory(context.Device(), data.m_memory);
                }
                vkDestroyBuffer(context.Device(), data.m_buffer, nullptr);
                vkFreeMemory(context.Device(), data.m_memory, nullptr);

                data = BufferResource{};
            }
        }
    }
}
