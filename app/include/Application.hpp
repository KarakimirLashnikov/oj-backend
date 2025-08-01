#pragma once
#include <httplib.h>
#include "Core.hpp"
#include "HttpMethod.hpp"
#include "JudgeResult.hpp"
#include "Configurator.hpp"
#include "JudgeManager.hpp"

namespace OJApp
{
    class Application
    {
    public:
        static Application& getInstance();

        void init(std::string_view host, int port, std::string_view log_file, std::string_view config_file);

        inline void registryResultHandler(std::function<void(Judge::SubmissionResult&&)>&& result_handler) {
            this->judge_manager->setCallback(std::move(result_handler));
        }

        template <Core::Http::HttpMethod Method>
        inline void registryRouter(std::string_view path, httplib::Server::Handler&& handler) {
            if constexpr (std::is_same_v<Method, Core::Http::GET>) {
                this->server->Get(path.data(), handler);
            } else if constexpr (std::is_same_v<Method, Core::Http::POST>) {
                this->server->Post(path.data(), handler);
            } else if constexpr (std::is_same_v<Method, Core::Http::PUT>) {
                this->server->Put(path.data(), handler);
            } else {
                LOG_ERROR("Unsupported Http Method");
            }
        }

        void run();

        ~Application() = default;
    private:
        std::unique_ptr<httplib::Server> server;
        std::shared_ptr<Core::Configurator> configurator;
        std::unique_ptr<Judge::JudgeManager> judge_manager;
        std::string host{};
        int port{};

        static std::unique_ptr<Application> s_App;

        Application() noexcept = default;
        Application(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;
        Application& operator=(const Application&) = delete;
    };
}

#define App OJApp::Application::getInstance()