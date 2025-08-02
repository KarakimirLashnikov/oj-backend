#include "Application.hpp"

namespace OJApp
{
    std::unique_ptr<Application> Application::s_AppPtr{ nullptr };

    Application &Application::getInstance()
    {
        if (!Application::s_AppPtr)
            Application::s_AppPtr.reset(new Application);
        return *s_AppPtr;
    }

    void Application::init(std::string_view host, int port)
    {
        this->m_Host = host;
        this->m_Port = port;
        this->m_HttpServer = std::make_unique<httplib::Server>();
        this->m_JudgeManager = std::make_unique<Judge::JudgeManager>(
            std::bind(&Application::judgeCallback, this, std::placeholders::_1));
        this->m_IsRunning = false;
    }

    void Application::run()
    {
        this->m_IsRunning = true;
        this->m_HttpServerThread = std::make_unique<std::thread>([this]() {
                LOG_INFO("HTTP Server Start");
                this->m_HttpServer->listen(this->m_Host.data(), this->m_Port);
                LOG_INFO("HTTP Server Ready");
            }
        );
        while (this->m_IsRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(2));
            LOG_WARN("main thread is idling ...");
        }
    }

    void Application::stop() 
    {
        this->m_IsRunning = false;
        this->m_HttpServer->stop();
        this->m_HttpServerThread->join();
    }

    void Application::judgeCallback(Judge::JudgeResult &&result)
    {
        LOG_INFO("{}", result.toString());
    }
}