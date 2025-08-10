#include "Application.hpp"
#include "Types.hpp"
#include "Configurator.hpp"
#include "ThreadPool.hpp"
#include "FileWriter.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/Language.hpp"
#include "compilers/CppCompiler.hpp"
#include "actuators/CppActuator.hpp"
#include "FileException.hpp"
#include "Queue.hpp"
#include "Judger.hpp"
#include "DBManager.hpp"
#include "RedisCache.hpp"
#include "authentication/AuthService.hpp"

namespace OJApp
{
    std::unique_ptr<Application> Application::s_AppPtr{ nullptr };

    using Core::Queue;
    struct Application::Impl
    {
        std::unique_ptr<Core::ThreadPool> m_JudgerPool;
        std::unique_ptr<Core::Configurator> m_Configurator;
        std::unique_ptr<Database::DBManager> m_DBManager;
        std::unique_ptr<Database::RedisCache> m_RedisCache;
        std::unique_ptr<AuthService> m_AuthService;
        std::unique_ptr<Core::Queue<Database::DBTask>> m_TaskQueue;
        fs::path m_SubmissionTempDir;
        std::atomic<bool> m_ShouldExit{ false };

        Impl() = default;

        Impl(std::string_view conf_file_path)
        : m_JudgerPool{ nullptr }
        , m_SubmissionTempDir{}
        , m_DBManager{}
        , m_RedisCache{}
        , m_AuthService{}
        , m_TaskQueue{}
        , m_Configurator{ std::make_unique<Core::Configurator>(conf_file_path.data()) }
        {
            size_t judger_num = m_Configurator->get<int>("application", "JUDGER_NUMBER", std::thread::hardware_concurrency());
            m_JudgerPool = std::make_unique<Core::ThreadPool>(judger_num);
            
            m_DBManager = std::make_unique<Database::DBManager>(
                m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                m_Configurator->get<uint16_t>("judgedb", "PORT", 3306),
                m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
                m_Configurator->get<std::string>("judgedb", "DATABASE", "judgedb")
            );
            
            m_RedisCache = std::make_unique<Database::RedisCache>();
            m_RedisCache->init(*m_Configurator);
            
            size_t queue_size = m_Configurator->get<size_t>("application", "DB_QUEUE_SIZE", 1024);
            m_TaskQueue = std::make_unique<Queue<Database::DBTask>>(queue_size);
            
            std::string secret_key{ m_Configurator->get<std::string>("application", "SECRET_KEY", "") };
            uint32_t token_expiry{ static_cast<uint32_t>(m_Configurator->get<double>("application", "TOKEN_EXPIRY", 300.0)) };
            m_AuthService = std::make_unique<AuthService>(m_DBManager.get(), m_RedisCache.get(), secret_key, std::chrono::seconds(token_expiry));
            
            m_SubmissionTempDir = fs::path{ m_Configurator->get<std::string>("application", "SUBMISSION_TEMP_DIR", "./submission_temp/" ) };
            if (!fs::is_directory(m_SubmissionTempDir))
                fs::create_directories(m_SubmissionTempDir);
        }

        ~Impl() = default;
    };


    Application &Application::getInstance()
    {
        if (!Application::s_AppPtr) {
            Core::Logger::Init("oj-server", Core::Logger::Level::INFO, "logs/server.log", true);
            Application::s_AppPtr.reset(new Application);
        }
        return *s_AppPtr;
    }

    void Application::init(std::string_view conf_file)
    {
        this->m_ImplPtr = std::make_unique<Impl>(conf_file);
        this->m_HttpServer = std::make_unique<httplib::Server>();
        Judge::CppActuator::initSystem(this->getConfigurator());
    }

    void Application::run(std::string_view host, uint16_t port)
    {
        m_ImplPtr->m_JudgerPool->enqueue([this](){
            while (!m_ImplPtr->m_ShouldExit) {
                Database::DBTask task{};
                if (m_ImplPtr->m_TaskQueue->pop(task, std::chrono::milliseconds(10000))) {
                    task.resume();
                }
            }
        }); // 启动后台工作线程进行数据库操作

        this->m_HttpServer->listen(host.data(), port);
        m_ImplPtr->m_ShouldExit = true;
    }

    using namespace Judge::Language;

    bool Application::submit(Judge::Submission &&submission)
    {
        fs::path source_file{ m_ImplPtr->m_SubmissionTempDir / submission.submission_id / ("main" + getFileExtension(submission.language_id))};
        std::unique_ptr<asio::io_context> pioc{ std::make_unique<asio::io_context>() }; 
        Core::FileWriter writer{ *pioc };
        try {
            writer.write(source_file, submission.source_code);
        } catch (std::exception& e) {
            LOG_ERROR(e.what());
            return false;
        }
        submission.source_code = source_file.string();
        m_ImplPtr->m_JudgerPool->enqueue([this, ioc = std::move(pioc), sub = std::move(submission)]() ->void {
            if (!ioc->stopped()) {
                ioc->run();
            }
            auto judge_result{ this->processSubmission(sub) };

            auto db_task{ m_ImplPtr->m_DBManager->insertJudgeResult(judge_result) };
        });
        return true;
    }
    
    Judge::JudgeResult Application::processSubmission(const Judge::Submission& sub)
    {
        Judge::JudgeResult judger_result{sub.problem_title, sub.submission_id, sub.language_id};

        Judge::ResourceLimits limits{ m_ImplPtr->m_DBManager->queryProblemLimits(sub.problem_title, sub.language_id) };

        Judge::Judger judger{ sub.language_id, sub.source_code, limits};
        auto compile_error{ judger.getCompileError() };
        if (compile_error.has_value()) {
            judger_result.compile_msg = compile_error.value();
            LOG_INFO("id: {} compile failed: {}", judger_result.sub_id, judger_result.compile_msg);
            judger_result.setStatus();
            return judger_result;
        }

        auto gen{ m_ImplPtr->m_DBManager->queryTestCases(sub.problem_title) };
        while (gen.next()) {
            auto tc{ gen.getCurrentTestCase() };
            auto execute_result{ judger.judge(tc.stdin_data, tc.expected_output) };
            judger_result.results.push_back(execute_result);
            if (execute_result.status != Judge::TestStatus::ACCEPTED)
                break;
        }
        judger_result.setStatus();
        return judger_result;
    }

    bool Application::uploadTestCases(std::vector<TestCase>&& test_cases, std::string_view problem_title)
    {
        m_ImplPtr->m_TaskQueue->push(m_ImplPtr->m_DBManager->insertTestCases(std::move(test_cases), problem_title.data()));
        return true;
    }

    void Application::addDBTask(Database::DBTask &&task)
    {
        m_ImplPtr->m_TaskQueue->push(std::move(task));
    }

    Core::Configurator& Application::getConfigurator() {
        return *(m_ImplPtr->m_Configurator);
    }

    Database::DBManager& Application::getDBManager() {
        return *(m_ImplPtr->m_DBManager);
    }

    AuthService& Application::getAuthService() {
        return *(m_ImplPtr->m_AuthService);
    }
}