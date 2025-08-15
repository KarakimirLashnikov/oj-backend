#include "Application.hpp"
#include "api/authentication.hpp"
#include "api/problems.hpp"
#include "api/submissions.hpp"

using namespace Core::Http;
using namespace OJApp::Authentication;
using namespace OJApp::Problems;
using namespace OJApp::Submissions;

int main()
{
    try
    {
        App.init("config.ini");

        App.registryRouter<POST>("/api/auth/registry", &registry);
        App.registryRouter<POST>("/api/auth/login", &login);

        App.registryRouter<POST>("/api/problems/create_problem", &createProblem);
        App.registryRouter<POST>("/api/problems/add_problem_limit", &addProblemLimit);
        App.registryRouter<POST>("/api/problems/upload_testcases", &uploadTestCases);

        App.registryRouter<POST>("/api/submissions/submit", &submit);
        App.registryRouter<POST>("/api/submissions/query_submissions", &querySubmissions);

        App.registryRouter<GET>("/api/problems/get_problem_list", &getProblemList);
    
        App.run("0.0.0.0", 8000);
    }
    catch (std::exception &e)
    {
        LOG_ERROR("ERROR: {}", e.what());
        exit(EXIT_FAILURE);
    }

    return 0;
}