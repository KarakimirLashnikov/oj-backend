#include "Application.hpp"
#include "api/submissions.hpp"
#include "api/Login.hpp"
#include "api/problems.hpp"
#include <sw/redis++/redis++.h>
using Core::Http::POST;
using OJApp::Login::login;
using OJApp::Login::registry;
using OJApp::Submissions::submit;
using OJApp::Problems::create;
using OJApp::Problems::update;
using OJApp::Problems::uploadTestCases;

int main()
{
    try
    {
        App.init("config.ini");
        App.registryRouter<POST>("/api/login/login", &login);
        App.registryRouter<POST>("/api/login/registry", &registry);
        App.registryRouter<POST>("/api/submissions/submit", &submit);
        App.registryRouter<POST>("/api/problems/create",&create);
        App.registryRouter<POST>("/api/problems/update", &update);
        App.registryRouter<POST>("/api/problems/upload_test_cases", &uploadTestCases);
    
        App.run("0.0.0.0", 8000);
    }
    catch (std::exception &e)
    {
        LOG_ERROR("ERROR: {}", e.what());
        exit(EXIT_FAILURE);
    }

    return 0;
}