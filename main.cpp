#include "Logger.hpp"
#include "Application.hpp"

void callback(Judge::SubmissionResult&& result)
{
    LOG_INFO("Callback Received:\n {}\n", result.toString());
}

int main()
{
    using Core::Http::PUT;
    using Core::Http::POST;
    try
    {
        App.init("127.0.0.1", 5000, "logs/server.log", "config.ini");
        App.registryResultHandler(callback);
        App.registryRouter<POST>("/submit", [&](const httplib::Request &req, httplib::Response &res) {

        });
        App.registryRouter<PUT>("/callback", [&](const httplib::Request &req, httplib::Response &res) {
            
        });
        App.run();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR(e.what());
        exit(EXIT_FAILURE);
    }
    return 0;
}