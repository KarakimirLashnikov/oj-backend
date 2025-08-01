#include "Logger.hpp"
#include "Configurator.hpp"
#include "JudgeManager.hpp"
#include "Application.hpp"

namespace OJApp
{
    std::unique_ptr<Application> Application::s_App{ };

    Application &Application::getInstance()
    {
        if (!s_App)
            s_App.reset(new Application);
        return *(Application::s_App);
    }

    void Application::init(std::string_view host, int port, std::string_view log_file, std::string_view config_file)
    {
        this->host = std::string(host);
        this->port = port;
        Core::Logger::Init("server", Core::Logger::Level::INFO, log_file.data());
        this->configurator = std::make_shared<Core::Configurator>(config_file);
        this->judge_manager = std::make_unique<Judge::JudgeManager>(
            this->configurator->get<std::string>("judge", "judge0_url", "http://localhost:2358"),
            this->configurator->get<std::string>("judge", "judge0_auth_token")
        );
    }

    void Application::run()
    {
        this->server->listen(this->host, this->port);
    }
}