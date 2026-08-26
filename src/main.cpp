#include "application/application.h"
#include "core/log.h"

int main()
{
    try
    {
        Kita::Pbrv::Application::Application app{};
        app.Run();

        return EXIT_SUCCESS;
    }
    catch (const std::exception& e)
    {
        Kita::Pbrv::Core::Log::Error(e.what());
        return EXIT_FAILURE;
    }
}
