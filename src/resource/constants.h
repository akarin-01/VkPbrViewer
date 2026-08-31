#pragma once

#include <cstdint>

namespace Kita::Pbrv
{
    namespace Resource
    {
        constexpr uint32_t kCubemapFaceSize = 2048;   // skybox / ibl shared
        constexpr uint32_t kIrradianceSize = 32;
        constexpr uint32_t kPrefilterBaseSize = 128;
        constexpr uint32_t kBrdfLutSize = 512;

        enum class MaterialSlot : uint32_t
        {
            Albedo = 0,
            Normal,
            MetallicRoughness,
            AO,
            Emissive,
            Count
        };

        constexpr uint32_t kMaterialSlotCount = static_cast<uint32_t>(MaterialSlot::Count);
    }
}
