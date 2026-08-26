#pragma once

#include "resource/mesh.h"
#include "resource/texture.h"

#include <string>

namespace Kita::Pbrv
{
    namespace Resource
    {
        namespace AssetUtils
        {
            Mesh LoadGltfMesh(const std::string& path);
            Texture LoadTexture(const std::string& path, TextureType type);
        }
    }
}
