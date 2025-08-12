#pragma once
#include <httplib.h>
#include "Logger.hpp"
#include "Configurator.hpp"
#include "Http.hpp"
#include "Types.hpp"
#include "RedisManager.hpp"
#include "DBManager.hpp"
#include "AuthService.hpp"

namespace OJApp
{
    using namespace Core::Types;

    class Application
    {
    public:
        static Application& getInstance();
        ~Application() = default;

        template <Core::Http::HttpMethod Method>
        inline void registryRouter(std::string_view path, httplib::Server::Handler handler) {
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

        void init(std::string_view conf_file);
        void run(std::string_view host, uint16_t port);
        
        Core::Configurator& getConfigurator();
        DBManager& getDBManager();
        RedisManager& getRedisManager();
        AuthService& getAuthService();

        void processDbOperateEvent(std::string channel, std::string msg);
    private:
        Application() = default;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_ImplPtr{ nullptr };
        std::unique_ptr<httplib::Server> m_HttpServer{ nullptr };
        
        static std::unique_ptr<Application> s_AppPtr;
    };
}

#define App OJApp::Application::getInstance()