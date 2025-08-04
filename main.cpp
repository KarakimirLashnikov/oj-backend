#include "Application.hpp"
#include "api/submissions.hpp"
using Core::Http::POST;

using OJApp::API::dealSubmit;

int main()
{
    try
    {
        App.init("config.ini");

        App.registryRouter<POST>("/api/submit", &dealSubmit);
    
        App.run("0.0.0.0", 8000);
    }
    catch (std::exception &e)
    {
        LOG_ERROR("ERROR: {}", e.what());
        exit(EXIT_FAILURE);
    }

    return 0;
}