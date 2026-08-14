#pragma once

#include "scene/mesh.h"
#include "scene/texture.h"

namespace Kita::Pbrv
{
    namespace AssetLoader
    {
        void LoadGltfMesh(const std::string& path, Mesh& mesh);
        void LoadTexture(const std::string& path, Texture::Type type, Texture& texture);
    }
}