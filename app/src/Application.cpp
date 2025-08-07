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
        if (!Application::s_AppPtr)
            Application::s_AppPtr.reset(new Application);
        return *s_AppPtr;
    }

    void Application::init(std::string_view conf_file)
    {
        Core::Logger::Init("oj-server", Core::Logger::Level::INFO, "logs/server.log");
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
                            // If no exception, remove user's temp code directory
                            fs::remove(std::get<fs::path>(sub_copy.uploade_code_file_path).parent_path());
                        } catch (const Exceptions::FileException& e) {
                            LOG_ERROR("{}\n", e.what());
                            JudgeDB::JudgeInquirer ji{
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PORT", "3306"),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
                            };
                            JudgeDB::JudgeWriter jw{
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PORT", "3306"),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                                m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
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
                            jw.updateSubmission(sub_copy.submission_id, Judge::SubmissionStatus::IE);
                            // 删除临时文件夹
                            fs::remove(std::get<fs::path>(sub_copy.uploade_code_file_path).parent_path());
                        } catch (const std::exception& e) {
                            LOG_ERROR("Unexpected exception in Judger Thread\n");
                        }
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
        auto file_extension{ getFileExtension(submission.language_id) };
        std::string source_path{ m_ImplPtr->m_SubmissionTempDir / (boost::uuids::to_string(submission.submission_id) + file_extension) };
        std::shared_ptr<asio::io_context> p_ioc{};
        std::string code = std::move(std::get<std::string>(submission.uploade_code_file_path));
        try {
            Core::FileWriter writer{ *p_ioc };
            writer.write(source_path.c_str(), code);
        } catch (const std::exception &e) {
            LOG_ERROR("Write Code File Error");
            return false;
        }
        submission.uploade_code_file_path = fs::path(source_path);
        auto error{ m_ImplPtr->m_JudgerPool->enqueue([this, p_ioc, submission, code]() -> Judge::Submission {
                    // 1. 创建写入器
                    JudgeDB::JudgeWriter jw{
                        m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                        m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PORT", "3306"),
                        m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                        m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
                    };
                    // 2. 先写入提交
                    jw.createSubmission(submission, code);
                    // 2. 评测并返回结果
                    auto result{ processSubmission(submission, *p_ioc) };
                    // 3. 写入评测结果
                    jw.writeJudgeResult(result);
                    // 4. 更新提交状态
                    jw.updateSubmission(result.sub_id, result.status());
                    return submission; // 如果出错 , 用来恢复
                }
            )
        };
        m_ImplPtr->m_FutureQueue.push(std::move(error));
        return true;
    }
    
    Judge::JudgeResult Application::processSubmission(const Judge::Submission& sub, asio::io_context& ioc)
    {
        Judge::JudgeResult result{};

        auto compiler{ getCompiler(sub.language_id) };
        auto actuator{ getActuator(sub.language_id) };

        if (!ioc.stopped()) {
            ioc.run(); // 保证编译文件前 ， 文件已经写到磁盘上
        }
        auto source_path{ std::get<fs::path>(sub.uploade_code_file_path) };
        fs::path exe_path{};

        if (compiler) { // 需要编译器的语言
            exe_path = source_path.parent_path() / "main";
            auto success{ compiler->compile(source_path.c_str(), exe_path.c_str(), sub.compile_options) };
            if (!success) {
                result.compile_msg = compiler->getCompileMessage();
                result.setStatus();
                return result;
            }
        } else {
            exe_path = source_path;
        }
        // 1. 创建一个查询器
        JudgeDB::JudgeInquirer ji{
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PORT", "3306"),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
            m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
        };
        // 2. 从数据库读取题目限制
        Judge::ResourceLimits limit{ ji.getProblemLimits(sub.problem_title) };
        // 3. 从题目读取输入输出 , 但是阻塞返回一个生成器
        auto generator{ ji.getTestCases(sub.problem_title) };
        while (generator.next()) {
            Core::Types::TestCase tc{ generator.getCurrentTestCase() };
            Judge::ExecutionResult exe_result{actuator->execute(exe_path, limit, tc.stdin)};
            if (exe_result.status == Judge::TestStatus::UNKNOWN) {
                if (exe_result.stdout == tc.expected_output)
                    exe_result.status = Judge::TestStatus::ACCEPTED;
                else 
                    exe_result.status = Judge::TestStatus::WRONG_ANSWER;
            }

            result.results.emplace_back(exe_result);
            if (exe_result.status != Judge::TestStatus::ACCEPTED) 
                break;
        }
        result.setStatus();

        return result;
    }

    bool Application::uploadTestCases(std::vector<TestCase>&& test_cases, std::string_view problem_title)
    {
        m_ImplPtr->m_JudgerPool->enqueue(
            [this, test_cases = std::move(test_cases), title = std::string(problem_title)](){
                JudgeDB::JudgeWriter writer{
                    m_ImplPtr->m_Configurator->get<std::string>("judgedb", "HOST", "127.0.0.1"),
                    m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PORT", "3306"),
                    m_ImplPtr->m_Configurator->get<std::string>("judgedb", "USERNAME", "root"),
                    m_ImplPtr->m_Configurator->get<std::string>("judgedb", "PASSWORD", ""),
                };
                writer.insertTestCases(test_cases, title);
            }
        );
        return true;
    }

    Core::Configurator& Application::getConfigurator() {
        return *(m_ImplPtr->m_Configurator);
    }
}