#include "Application.hpp"

namespace OJApp
{
    std::unique_ptr<Application> Application::s_AppPtr{ nullptr };

    constexpr static int MaxCacheResultNum{ 128 };
    constexpr static float CacheTimeout_s { 2.0f };
    constexpr const char* SubmissionTempRootDir{ "SubmissionTempRootDir" };

    Application &Application::getInstance()
    {
        if (!Application::s_AppPtr)
            Application::s_AppPtr.reset(new Application);
        return *s_AppPtr;
    }

    void Application::init(std::string_view host, int port)
    {
        Core::Logger::Init("oj-server", Core::Logger::Level::INFO, "logs/server.log", true, 1024, 2);
        this->m_Host = host;
        this->m_Port = port;
        this->m_HttpServer = std::make_unique<httplib::Server>();
        this->m_JudgeManager = std::make_unique<Judge::JudgeManager>(
            std::bind(&Application::judgeCallback, this, std::placeholders::_1, std::placeholders::_2));
        this->m_ShutdownRequested = false;
        this->m_IsRunning = false;
        if (!fs::exists(SubmissionTempRootDir) && !fs::create_directory(SubmissionTempRootDir)) {
            LOG_ERROR("Failed to create submission root directory: {}", SubmissionTempRootDir);
            return;
        }
    }

    void Application::startServer()
    {
        this->m_HttpServerThread = std::make_unique<std::thread>([this]() {
            LOG_INFO("HTTP Server Start");
            this->m_HttpServer->listen(this->m_Host.data(), this->m_Port);
            
            // 服务器停止后的清理
            if (this->m_ShutdownRequested) {
                LOG_INFO("HTTP Server stopped");
            } else {
                LOG_WARN("HTTP Server stopped unexpectedly");
            }
        });
    }

    void Application::run()
    {
        this->m_IsRunning = true;
        
        // 等待关闭信号
        {
            std::unique_lock<std::mutex> lock(m_ShutdownMutex);
            this->m_ShutdownCV.wait(lock, [this] { 
                return this->m_ShutdownRequested.load(); 
            });
        }
        
        LOG_INFO("Main thread preparing to exit");
    }
    void Application::stop()
    {
        if (!m_ShutdownRequested) {
            shutdownServer();
        }

        // 等待HTTP线程结束
        if (m_HttpServerThread && m_HttpServerThread->joinable()) {
            m_HttpServerThread->join();
            m_HttpServerThread.reset();
        }

        // 清理资源
        m_JudgeManager.reset();
        m_HttpServer.reset();
        
        
        LOG_INFO("All resources cleaned up");
    }

    void Application::processSubmission(Judge::Submission sub)
    {
        fs::path root_dir{ SubmissionTempRootDir };
        root_dir /= boost::uuids::to_string(sub.submission_id);
        if (fs::create_directory(root_dir)) {
            // 写入源码
            {
                fs::path code_file{ root_dir / ("main" + Judge::Language::getFileExtension(sub.language_id)) };
                std::ofstream file{ code_file, std::ios::trunc };
                if (!file.is_open()) {
                    LOG_ERROR("Failed to write source file: {}", (root_dir / sub.source_code_path).string());
                    return;
                }
                file << sub.source_code_path << std::endl;
                sub.source_code_path = code_file.generic_string();
            }
            LOG_INFO("process submission: {}", __FILE__);
            this->m_JudgeManager->submit(std::forward<Judge::Submission>(sub));
        }
    }

    void Application::WriteSubmissionToDB(const Judge::JudgeResult& result)
    {
        LOG_INFO("Submission Completed: {}", result.toString());
    }

    void Application::shutdownServer()
    {
        if (m_ShutdownRequested.exchange(true)) {
            LOG_WARN("Shutdown already requested");
            return;
        }

        LOG_INFO("shutdown...");
        
        // 1. 停止接受新请求
        m_HttpServer->stop();
        
        // 2. 通知工作线程（如果有任务队列）
        if (m_JudgeManager) {

        }

        // 3. 唤醒主线程
        m_ShutdownCV.notify_one();
    }

    void Application::judgeCallback(Core::Types::SubID sub_id ,std::future<Judge::JudgeResult> &&result)
    {
        LOG_INFO("Get future of submission: {}", boost::uuids::to_string(sub_id));
        result.get();
        std::lock_guard<std::mutex> lock{ this->m_JudgeCacheMutex };
        this->m_JudgeCache.emplace(sub_id, std::move(result));

        if (this->m_JudgeCache.empty())
            this->m_LastFlushTime = std::chrono::steady_clock::now();
            return;

        auto flushResultToDB{
            [this] () -> void {
                std::vector<std::future<void>> f{};
                for (auto& [_, future] : m_JudgeCache) {
                    try {
                        auto val{ future.get() };
                        f.emplace_back(std::async(std::launch::async, 
                            std::bind(&Application::WriteSubmissionToDB, this, std::placeholders::_1), future.get()));
                    } catch (const std::exception& e) {
                        // TODO: 任务评测过程中出现的异常处理
                        continue;
                    }
                }
                for (auto&& fu : f) {
                    try {
                        fu.get();
                    } catch (const std::exception& e) {
                        // TODO: 写入数据库过程中发生的异常
                    }
                }
            }
        };

        if (this->m_JudgeCache.size() == MaxCacheResultNum ||
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - this->m_LastFlushTime
            ).count() >= CacheTimeout_s) {
            flushResultToDB();
            this->m_JudgeCache.clear();
            this->m_LastFlushTime = std::chrono::steady_clock::now();
        }
    }
}