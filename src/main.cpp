#include <stdexcept>
#include <filesystem>

#include "core/input.h"
#include "core/log.h"
#include "core/time.h"
#include "core/window.h"
#include "render/renderer.h"
#include "scene/scene.h"
#include "scene/asset_loader.h"
#include "scene/camera_controller.h"

using TextureType = Kita::Pbrv::TextureType;
using Key = Kita::Pbrv::Key;
using MouseButton = Kita::Pbrv::MouseButton;

int main()
{
    try
    {
        // Check if the "assets" directory exists
        if (!std::filesystem::is_directory("assets"))
        {
            throw std::runtime_error("assets/ directory not found");
        }

        Kita::Pbrv::Window window(800, 600, "Vk Pbr Viewer");
        Kita::Pbrv::Input input(window);
        Kita::Pbrv::Time time{};

        Kita::Pbrv::Scene scene;
        scene.GetLight()
            .SetDirection(glm::vec3(1.0f, 1.0f, 0.0f));

        Kita::Pbrv::Mesh& mesh = scene.GetMesh();
        Kita::Pbrv::AssetLoader::LoadGltfMesh("assets/models/Cerberus_LP.glb", mesh);

        Kita::Pbrv::Material& mat = scene.GetMaterial();
        Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_A.tga", TextureType::Albedo, mat.GetAlbedoTex());
        Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_N.tga", TextureType::Normal, mat.GetNormalTex());
        Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_M.tga", TextureType::Linear, mat.GetMetallicTex());
        Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_R.tga", TextureType::Linear, mat.GetRoughnessTex());
        Kita::Pbrv::AssetLoader::LoadTexture("assets/textures/Cerberus_AO.tga", TextureType::Linear, mat.GetAOTex());

        Kita::Pbrv::CameraController cameraController(input, scene.GetCamera());

        Kita::Pbrv::Renderer renderer(window);

        while (!window.ShouldClose())
        {
            window.PollEvents();
            input.Update();
            time.Update();

            float deltaTime = time.GetDeltaTime();

            if (input.IsKeyPressed(Key::Escape))
            {
                window.RequestClose();
                continue;
            }

            cameraController.Update(deltaTime);

            renderer.DrawFrame(scene);
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        Kita::Pbrv::Log::Error(e.what());
        return EXIT_FAILURE;
    }
}