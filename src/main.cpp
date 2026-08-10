#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "core/window.h"
#include "render/renderer.h"
#include "scene/scene.h"
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
        scene.GetMaterial()
            .SetAlbedo(glm::vec4(0.0f, 1.0f, 1.0f, 1.0f))
            .SetMetallic(0.5f)
            .SetRoughness(0.5f)
            .SetAO(1.0f);
        scene.GetMesh().LoadFromObj("assets/models/Cerberus_LP.obj");

        Pbrv::Renderer renderer(window);

        while (!window.ShouldClose())
        {
            window.PollEvents();

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