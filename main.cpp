#include "Application.hpp"
#include "api/authentication.hpp"

using namespace Core::Http;
using namespace OJApp::Authentication;

int main()
{
    try
    {
        App.init("config.ini");

        App.registryRouter<POST>("/api/auth/registry", &registry);
    
        App.run("0.0.0.0", 8000);
    }
    catch (std::exception &e)
    {
        LOG_ERROR("ERROR: {}", e.what());
        exit(EXIT_FAILURE);
    }

    return 0;
}