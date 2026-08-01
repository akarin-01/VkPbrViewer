#include <iostream>
#include <stdexcept>
#include <filesystem>

#include "core/window.h"
#include "render/renderer.h"

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
        Pbrv::Renderer renderer(window);

        while (!window.ShouldClose())
        {
            window.PollEvents();

            renderer.DrawFrame();
        }

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}