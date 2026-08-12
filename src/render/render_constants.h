#pragma once

#include <cstdint>
#include <string>

namespace Kita::Pbrv
{
    constexpr uint32_t kMaxFramesInFlight = 2;

    enum MaterialTextureSlot : uint32_t
    {
        Albedo = 0,
        Normal,
        Metallic,
        Roughness,
        AO,
        kMaterialTextureCount
    };

    inline const char* ToString(MaterialTextureSlot tex)
    {
        switch (tex)
        {
        case Albedo:
            return "Albedo";
        case Normal:
            return "Normal";
        case Metallic:
            return "Metallic";
        case Roughness:
            return "Roughness";
        case AO:
            return "AO";
        default:
            return "Unknown";
        }
    }
}