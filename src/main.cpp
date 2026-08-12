#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "core/window.h"
#include "render/renderer.h"
#include "scene/scene.h"
#include "scene/material.h"
#include "scene/texture.h"
#include "scene/mesh.h"

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
        Pbrv::Material& mat = scene.GetMaterial()
            .SetMetallic(0.0f);
        // mat.GetAlbedoTex().LoadFromFile("assets/textures/Cerberus_A.tga", Pbrv::Texture::Type::Albedo);
        mat.GetNormalTex().LoadFromFile("assets/textures/Cerberus_N.tga", Pbrv::Texture::Type::Normal);
        // mat.GetMetallicTex().LoadFromFile("assets/textures/Cerberus_M.tga", Pbrv::Texture::Type::Linear);
        mat.GetRoughnessTex().LoadFromFile("assets/textures/Cerberus_R.tga", Pbrv::Texture::Type::Linear);
        mat.GetAOTex().LoadFromFile("assets/textures/Cerberus_AO.tga", Pbrv::Texture::Type::Linear);
        // scene.GetMesh().LoadFromObj("assets/models/Cerberus_LP.obj");

        Pbrv::Renderer renderer(window);

        uint32_t frame = 0;
        while (!window.ShouldClose())
        {
            window.PollEvents();

            ++frame;
            if (frame == 60)
            {
                scene.GetMesh().SetEmpty();
                mat.GetAlbedoTex().SetEmpty();
            }
            else if (frame == 120)
            {
                scene.GetMesh().LoadFromObj("assets/models/Cerberus_LP.obj");
            }
            else if (frame == 240)
            {
                mat.GetAlbedoTex().LoadFromFile("assets/textures/Cerberus_A.tga", Pbrv::Texture::Type::Albedo);
            }
            else if (frame == 241)
            {
                mat.GetMetallicTex().LoadFromFile("assets/textures/Cerberus_M.tga", Pbrv::Texture::Type::Linear);
            }
            else if (frame == 480)
            {
                mat.GetAlbedoTex().SetEmpty();
            }
            else if (frame == 600)
            {
                scene.GetMesh().SetEmpty();
            }

            renderer.DrawFrame(scene);
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}