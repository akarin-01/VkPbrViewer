#pragma once

#include "resource/asset_types.h"

#include <string>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace AssetUtils
        {
            MeshAsset LoadGltfMesh(const std::string& path);
            TextureAsset LoadTexture(const std::string& path, TextureAsset::Type type);
        }
    }
}
