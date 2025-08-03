#pragma once
#include <httplib.h>
#include "Logger.hpp"
#include "Core.hpp"
#include "HttpMethod.hpp"
#include "JudgeManager.hpp"

namespace OJApp
{
    using JudgeCacheContainer = std::map<Core::Types::SubID, std::future<Judge::JudgeResult>>;

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

        void init(std::string_view host, int port);
        void startServer();
        void run();
        void shutdownServer();
        void stop();

        void processSubmission(Judge::Submission sub);

        void WriteSubmissionToDB(const Judge::JudgeResult& result);

    private:
        Application() = default;
        void judgeCallback(Core::Types::SubID id, std::future<Judge::JudgeResult>&& result);

    private:
        static std::unique_ptr<Application> s_AppPtr;
        std::atomic<bool> m_IsRunning{false};
        std::atomic<bool> m_ShutdownRequested{false};  // 关闭请求标志
        
        std::unique_ptr<Judge::JudgeManager> m_JudgeManager{nullptr};
        std::unique_ptr<httplib::Server> m_HttpServer{nullptr};
        std::unique_ptr<std::thread> m_HttpServerThread{nullptr};
        
        // 同步原语
        std::mutex m_ShutdownMutex;
        std::condition_variable m_ShutdownCV;

        // 提交 ID 和 评测句柄
        std::mutex m_JudgeCacheMutex;
        JudgeCacheContainer m_JudgeCache;
        Core::Types::TimeStamp m_LastFlushTime;
        
        std::string_view m_Host{};
        int m_Port{};
    };
}

#define App OJApp::Application::getInstance()