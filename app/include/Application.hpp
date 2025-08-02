#pragma once
#include <httplib.h>
#include "Logger.hpp"
#include "Core.hpp"
#include "HttpMethod.hpp"
#include "JudgeManager.hpp"

namespace OJApp
{
    class Application
    {
    public:
        static Application& getInstance();

        void init(std::string_view host, int port);
        
        template <Core::Http::HttpMethod Method>
        inline void registryRouter(std::string_view path, httplib::Server::Handler&& handler) {
            if constexpr (std::is_same_v<Method, Core::Http::GET>) {
                this->m_HttpServer->Get(path.data(), handler);
            } else if constexpr (std::is_same_v<Method, Core::Http::POST>) {
                this->m_HttpServer->Post(path.data(), handler);
            } else if constexpr (std::is_same_v<Method, Core::Http::PUT>) {
                this->m_HttpServer->Put(path.data(), handler);
            } else {
                LOG_ERROR("Unsupported Http Method type: {}", path.data());
            }
        }
        
        void run();
        void stop();

        ~Application() = default;

    private:
        Application() = default;
        void judgeCallback(Judge::JudgeResult &&result);

    private:
        static std::unique_ptr<Application> s_AppPtr;
        std::atomic<bool> m_IsRunning{ false };
        std::unique_ptr<Judge::JudgeManager> m_JudgeManager{nullptr};
        std::unique_ptr<httplib::Server> m_HttpServer{nullptr};
        std::unique_ptr<std::thread> m_HttpServerThread{nullptr};
        std::string_view m_Host{};
        int m_Port{};
    };
}

#define App OJApp::Application::getInstance()