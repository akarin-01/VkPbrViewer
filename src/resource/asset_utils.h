#pragma once

#include "resource/asset_types.h"

#include <string>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace AssetUtils
        {
            /// Loads a glTF mesh; throws on failure (missing file, parse error,
            /// no valid triangle data). A successful return is guaranteed
            /// non-empty and buildable into a MeshResource.
            MeshAsset LoadGltfMesh(const std::string& path);

            /// Loads an image; throws on failure (missing file, decode error).
            /// A successful return is guaranteed to have valid dimensions
            /// and non-empty bytes.
            TextureAsset LoadTexture(const std::string& path, TextureAsset::Type type);
        }
    }
}
