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
#include "judgedb/JudgeInquirer.hpp"
#include "judgedb/JudgeWriter.hpp"
#include "Judger.hpp"

namespace OJApp
{
    std::unique_ptr<Application> Application::s_AppPtr{ nullptr };

    using Core::Queue;
    struct Application::Impl
    {
        Queue<std::future<Judge::Submission>> m_FutureQueue;
        std::unique_ptr<Core::ThreadPool> m_JudgerPool;
        std::unique_ptr<std::thread> m_ErrorHandlerThread;
        std::unique_ptr<Core::Configurator> m_Configurator{};
        fs::path m_SubmissionTempDir;
        std::atomic<bool> m_ShouldExit{ false };

        Impl() = default;

        Impl(std::string_view conf_file_path)
        : m_JudgerPool{ nullptr }
        , m_ErrorHandlerThread{ nullptr }
        , m_FutureQueue{}
        , m_SubmissionTempDir{}
        , m_Configurator{ std::make_unique<Core::Configurator>(conf_file_path.data()) }
        {
            size_t judger_num = m_Configurator->get<int>("application", "JUDGER_NUMBER", std::thread::hardware_concurrency());
            m_JudgerPool = std::make_unique<Core::ThreadPool>(judger_num);
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

            // submission 写入数据库
        });
        return true;
    }
    
    Judge::JudgeResult Application::processSubmission(const Judge::Submission& sub)
    {
        Judge::JudgeResult judger_result{sub.problem_title, boost::uuids::to_string(sub.submission_id)};

        JudgeDB::JudgeInquirer ji{
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
            m_ImplPtr->m_Configurator->get<uint16_t>("judgedb", "PORT", 3306),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "DATABASE", "judgedb")
        };

        Judge::ResourceLimits limits{ ji.queryProblemLimits(sub.problem_title, sub.language_id) };

        Judge::Judger judger{ sub.language_id, sub.source_code, limits};
        auto compile_error{ judger.getCompileError() };
        if (compile_error.has_value()) {
            judger_result.compile_msg = compile_error.value();
            LOG_INFO("id: {} compile failed: {}", judger_result.sub_id, judger_result.compile_msg);
            judger_result.setStatus();
            return judger_result;
        }

        auto gen{ ji.getTestCases(sub.problem_title) };
        while (gen.next()) {
            auto tc{ gen.getCurrentTestCase() };
            auto execute_result{ judger.judge(tc.stdin, tc.expected_output) };
            judger_result.results.push_back(execute_result);
            if (execute_result.status != Judge::TestStatus::ACCEPTED)
                break;
        }
        judger_result.setStatus();
        return judger_result;
    }

    bool Application::uploadTestCases(std::vector<TestCase>&& test_cases, std::string_view problem_title)
    {
        try {
            JudgeDB::JudgeWriter writer{
                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                m_ImplPtr->m_Configurator->get<uint16_t>("judgedb", "PORT", 3306),
                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "DATABASE", "judgedb")
            };
            writer.insertTestCases(test_cases, std::string(problem_title));
        } catch (std::exception& e) {
            LOG_ERROR("insert test case error: {}", e.what());
            return false;
        }
        return true;
    }

    Core::Configurator& Application::getConfigurator() {
        return *(m_ImplPtr->m_Configurator);
    }
}