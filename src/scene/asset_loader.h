#pragma once

#include "scene/mesh.h"
#include "scene/skybox.h"
#include "scene/texture.h"

#include <string>

namespace Kita::Pbrv
{
    namespace Scene
    {
        namespace AssetLoader
        {
            void LoadGltfMesh(const std::string& path, Mesh& mesh);
            void LoadTexture(const std::string& path, TextureType type, Texture& texture);
            void LoadSkybox(const std::string& path, Skybox& skybox);
        }
    }
}
