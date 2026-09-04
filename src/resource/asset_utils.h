#pragma once

#include "resource/asset_types.h"

#include <optional>
#include <string>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace AssetUtils
        {
            /// Loads a glTF/GLB into actual model data. Throws on failure.
            /// Each glTF primitive becomes one ModelAsset::Part.
            ModelAsset LoadGltf(const std::string& path);

            /// Loads an image. Returns nullopt on missing/decode failure.
            std::optional<TextureAsset> LoadTexture(const std::string& path, TextureAsset::Type type);
        }
    }
}
