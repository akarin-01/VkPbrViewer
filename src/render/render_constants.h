#pragma once

#include <cstdint>

namespace Kita::Pbrv
{
    constexpr uint32_t kMaxFramesInFlight = 2;

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
