#pragma once

#include "render/render_resource_types.h"

#include <vulkan/vulkan.h>

namespace Kita::Pbrv
{
    class RenderResources;

    RenderTexture CreateCubemapFallback(RenderResources& resources, VkFormat format);

    void DestroyTexture(RenderResources& resources, RenderTexture& texture);
}
