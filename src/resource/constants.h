#pragma once

#include <cstdint>

namespace Kita::Pbrv
{
    namespace Resource
    {
        enum MaterialTextureSlot : uint32_t
        {
            Albedo = 0,
            Normal,
            MetallicRoughness,
            AO,
            Emissive,
            kMaterialTextureCount
        };
    }
}
