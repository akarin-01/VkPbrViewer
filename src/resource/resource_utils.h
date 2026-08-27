#pragma once

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    namespace Rhi
    {
        class Context;
    }

    namespace Resource
    {
        struct BufferData;

        namespace ResourceUtils
        {
            /// Create an empty buffer; `mapped` maps host-visible memory for direct writes.
            BufferData CreateBufferData(const Rhi::Context& context,
                VkDeviceSize size, VkBufferUsageFlags usages, VkMemoryPropertyFlags properties,
                bool mapped = false);

            /// Create a buffer and fill it with `data` (synchronous).
            /// Host-visible: direct memcpy; device-local: TRANSFER_DST is added automatically,
            /// upload goes through a staging buffer that is destroyed here.
            BufferData CreateBufferData(const Rhi::Context& context,
                VkDeviceSize size, VkBufferUsageFlags usages, VkMemoryPropertyFlags properties,
                const void* data, size_t dataSize);

            /// Destroy immediately; only call when the GPU is done using the buffer
            /// (blocks should go through the ResourceManager's deferred path).
            void DestroyBufferData(const Rhi::Context& context, BufferData& data);
        };
    }
}
