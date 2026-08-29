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
        struct BufferResource;
        struct BufferDesc;

        namespace ResourceUtils
        {
            BufferResource CreateBufferResource(const Rhi::Context& context,
                BufferDesc desc, const void* data = nullptr, size_t size = 0);

            void DestroyBufferResource(const Rhi::Context& context, BufferResource& data);
        };
    }
}
