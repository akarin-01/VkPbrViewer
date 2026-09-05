#pragma once

#include <string>

namespace Kita::Pbrv
{
    namespace Resource
    {
        class AssetManager;
    }

    namespace Scene
    {
        class Scene;

        /// Scene assembly helpers on top of the resource layer's asset module
        namespace Utils
        {
            /// Loads a model and spawns one Object per part with its material,
            /// transform and textures. Throws on load failure.
            void SpawnModel(Scene& scene, Resource::AssetManager& assets, const std::string& path);
        }
    }
}
