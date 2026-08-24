#pragma once

#include <cstdint>

#define STD140_ASSERT(T, SIZE)\
    static_assert(sizeof(T) == SIZE, #T " std140 mismatch (expected " #SIZE")")

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
