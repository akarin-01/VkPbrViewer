#include <iostream>
#include <filesystem>

int main()
{
    // Check if the "assets" directory exists
    if (!std::filesystem::is_directory("assets"))
    {
        std::cerr << "Error: 'assets' directory not found." << std::endl;
        exit(-1);
    }

    std::cout << "Hello, Vulkan PBR Viewer!" << std::endl;
    return 0;
}