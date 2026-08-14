#include <stdexcept>
#include <filesystem>

#include "core/log.h"
#include "core/window.h"
#include "render/renderer.h"
#include "scene/scene.h"
#include "scene/asset_loader.h"

using namespace Kita;

int main()
{
    try
    {
        // Check if the "assets" directory exists
        if (!std::filesystem::is_directory("assets"))
        {
            throw std::runtime_error("assets/ directory not found");
        }

        Pbrv::Window window(800, 600, "Vk Pbr Viewer");

        Pbrv::Scene scene;
        scene.GetLight()
            .SetDirection(glm::vec3(1.0f, 1.0f, 0.0f));

        Pbrv::Mesh& mesh = scene.GetMesh();
        Pbrv::AssetLoader::LoadGltfMesh("assets/models/Cerberus_LP.glb", mesh);

        Pbrv::Material& mat = scene.GetMaterial()
            .SetMetallic(0.0f);
        Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_A.tga", Pbrv::Texture::Type::Albedo, mat.GetAlbedoTex());
        Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_N.tga", Pbrv::Texture::Type::Normal, mat.GetNormalTex());
        Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_M.tga", Pbrv::Texture::Type::Linear, mat.GetMetallicTex());
        Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_R.tga", Pbrv::Texture::Type::Linear, mat.GetRoughnessTex());
        Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_AO.tga", Pbrv::Texture::Type::Linear, mat.GetAOTex());

        Pbrv::Renderer renderer(window);

        uint32_t frame = 0;
        while (!window.ShouldClose())
        {
            window.PollEvents();

            ++frame;

            // if (frame == 10000)
            // {
            //     mat.GetAlbedoTex().SetEmpty();
            //     mat.GetNormalTex().SetEmpty();
            //     mat.GetMetallicTex().SetEmpty();
            //     mat.GetRoughnessTex().SetEmpty();
            //     mat.GetAOTex().SetEmpty();
            // }
            // else if (frame == 20000)
            // {
            //     scene.GetMesh().SetEmpty();
            // }

            renderer.DrawFrame(scene);
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        Pbrv::Log::Error(e.what());
        return EXIT_FAILURE;
    }
}