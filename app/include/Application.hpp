#pragma once
#include <httplib.h>
#include "Logger.hpp"
#include "Configurator.hpp"
#include "Core.hpp"
#include "Http.hpp"
#include "utilities/Submission.hpp"
#include "Types.hpp"

namespace OJApp
{
    using Core::Types::TestCase;

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
        bool submit(Judge::Submission&& submission);
        bool uploadTestCases(std::vector<TestCase>&& test_cases, std::string_view problem_title);

        Core::Configurator& getConfigurator();
    private:
        Application() = default;

        Judge::JudgeResult processSubmission(const Judge::Submission& sub);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_ImplPtr{ nullptr };
        std::unique_ptr<httplib::Server> m_HttpServer{ nullptr };
        
        static std::unique_ptr<Application> s_AppPtr;
    };
}

#define App OJApp::Application::getInstance()