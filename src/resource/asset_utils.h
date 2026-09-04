#pragma once

#include "resource/asset_types.h"

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

            /// Loads an image; throws on failure (missing file, decode error).
            /// A successful return is guaranteed to have valid dimensions
            /// and non-empty bytes.
            TextureAsset LoadTexture(const std::string& path, TextureAsset::Type type);
        }
    }
}
