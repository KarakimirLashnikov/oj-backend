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
        m_ImplPtr->m_ErrorHandlerThread = std::make_unique<std::thread>(
            [this]() -> void {
                while (!m_ImplPtr->m_ShouldExit) {
                    std::future<Judge::Submission> sub{};
                    if (m_ImplPtr->m_FutureQueue.pop(sub, std::chrono::seconds(1))) {
                        Judge::Submission sub_copy{};
                        try {
                            sub_copy = sub.get();
                        } catch (const std::exception& e) {
                            LOG_ERROR("{}\n", e.what());
                            JudgeDB::JudgeInquirer ji{
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                                m_ImplPtr->m_Configurator->get<uint16_t>("judgedb", "PORT", 3306),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "DATABASE", "judgedb")
                            };
                            JudgeDB::JudgeWriter jw{
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                                m_ImplPtr->m_Configurator->get<uint16_t>("judgedb", "PORT", 3306),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "DATABASE", "judgedb")
                            };
                            // 先查数据库提交有没有记录 , 如果有写入状态为 Internal Error
                            // 如果没有则要从用户提交的临时文件夹写入提交 , 设置状态为 Internal Error
                            if (!ji.isExists(sub_copy.submission_id)) {
                                std::ifstream f{ std::get<fs::path>(sub_copy.uploade_code_file_path).string() };
                                if (!f.is_open()) {
                                    LOG_CRITICAL("Cannot read code from temporary directory. Submission id: {}.", boost::uuids::to_string(sub_copy.submission_id));
                                    continue; // 无法再处理
                                }
                                std::stringstream code{};
                                code << f.rdbuf();
                                jw.createSubmission(sub_copy, code.str());
                            }
                            std::string uuid_str{ boost::uuids::to_string(sub_copy.submission_id) };
                            jw.updateSubmission(uuid_str, Judge::SubmissionStatus::IE);
                        }
                        // 删除临时文件夹
                        fs::remove_all(std::get<fs::path>(sub_copy.uploade_code_file_path).parent_path());
                    }
                }
            }
        );

        this->m_HttpServer->listen(host.data(), port);
        m_ImplPtr->m_ShouldExit = true;
        m_ImplPtr->m_ErrorHandlerThread->join();
    }

    using namespace Judge::Language;

    bool Application::submit(Judge::Submission &&submission)
    {
        auto start_at{ std::chrono::steady_clock::now() };
        // 读取源码
        std::string code{ std::get<std::string>(submission.uploade_code_file_path) };
        // 设置源码存储路径 , 创建临时文件夹
        std::string file_extension{ getFileExtension(submission.language_id) };
        fs::path source_path{ m_ImplPtr->m_SubmissionTempDir / (boost::uuids::to_string(submission.submission_id) + "/main" + file_extension) };
        fs::create_directory(source_path.parent_path());
        // 转储源码路径
        submission.uploade_code_file_path = source_path;
        // 源码写入临时文件
        std::shared_ptr<asio::io_context> p_ioc{ std::make_shared<asio::io_context>() };
        try {
            Core::FileWriter writer{ *p_ioc };
            writer.write(source_path.c_str(), code);
        } catch (const std::exception& e) {
            LOG_ERROR("Cannot write file. Submission id: {}.", boost::uuids::to_string(submission.submission_id));
            return false;
        }
        auto error{ m_ImplPtr->m_JudgerPool->enqueue([this, sub = submission, p_ioc, code_copy = std::move(code), start_at]() -> Judge::Submission {
                    // 1. 创建写入器
                    LOG_INFO("A new task in judger thread. Submission id: {}.", boost::uuids::to_string(sub.submission_id));
                    JudgeDB::JudgeWriter jw{
                        m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                        m_ImplPtr->m_Configurator->get<uint16_t>("judgedb", "PORT", 3306),
                        m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                        m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
                        m_ImplPtr->m_Configurator->get<std::string>("judgedb", "DATABASE", "judgedb")
                    };
                    // 2. 先写入提交
                    jw.createSubmission(sub, code_copy);
                    // 2. 评测并返回结果
                    auto result{ processSubmission(sub, *p_ioc) };
                    // 3. 写入评测结果
                    jw.writeJudgeResult(result);
                    // 4. 更新提交状态
                    jw.updateSubmission(result.sub_id, result.status());
                    auto duration{ std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_at) };
                    LOG_INFO("End of this task. Submission id: {}. Judging duration: {} ms.", result.sub_id, duration.count());
                    return sub; // 如果出错 , 用来恢复
                }
            )
        };
        m_ImplPtr->m_FutureQueue.push(std::move(error));
        return true;
    }
    
    Judge::JudgeResult Application::processSubmission(const Judge::Submission& sub, asio::io_context& ioc)
    {
        Judge::JudgeResult judge_result{};
        judge_result.sub_id = boost::uuids::to_string(sub.submission_id);
        judge_result.problem_title = sub.problem_title;

        auto compiler{ getCompiler(sub.language_id) };
        auto actuator{ getActuator(sub.language_id) };

        if (!ioc.stopped()) {
            ioc.run(); // 保证编译文件前 ， 文件已经写到磁盘上
        }
        fs::path source_path{ std::get<fs::path>(sub.uploade_code_file_path) };
        fs::path exe_path{};

        if (compiler) { // 需要编译器的语言
            exe_path = source_path.parent_path() / "main";
            auto success{ compiler->compile(source_path.c_str(), exe_path.c_str(), sub.compile_options) };
            if (!success) {
                judge_result.compile_msg = compiler->getCompileMessage();
                judge_result.setStatus();
                return judge_result;
            }
        } else {
            exe_path = source_path;
        }
        // 1. 创建一个查询器
        JudgeDB::JudgeInquirer ji{
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
            m_ImplPtr->m_Configurator->get<uint16_t>("judgedb", "PORT", 3306),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "DATABASE", "judgedb")
        };
        // 2. 从数据库读取题目限制
        Judge::ResourceLimits limit{ ji.getProblemLimits(sub.problem_title) };
        // 3. 从题目读取输入输出 , 但是阻塞返回一个生成器
        auto generator{ ji.getTestCases(sub.problem_title) };
        while (generator.next()) {
            Core::Types::TestCase tc{ generator.getCurrentTestCase() };
            Judge::ExecutionResult execute_result{actuator->execute(exe_path, limit, tc.stdin)};
            if (execute_result.test_result.status == Judge::TestStatus::UNKNOWN) {
                if (execute_result.stdout == tc.expected_output)
                    execute_result.test_result.status = Judge::TestStatus::ACCEPTED;
                else 
                    execute_result.test_result.status = Judge::TestStatus::WRONG_ANSWER;
            }

            judge_result.results.emplace_back(execute_result.test_result);
            if (execute_result.test_result.status != Judge::TestStatus::ACCEPTED) 
                break;
        }
        judge_result.setStatus();

        return judge_result;
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