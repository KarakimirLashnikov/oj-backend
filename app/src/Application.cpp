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

namespace OJApp
{
    std::unique_ptr<Application> Application::s_AppPtr{ nullptr };

    using Core::Types::Queue;
    struct Application::Impl
    {
        Queue<std::future<Judge::Submission>> m_FutureQueue;
        std::unique_ptr<Core::ThreadPool> m_JudgerPool;
        std::unique_ptr<std::thread> m_ErrorHandlerThread;
        fs::path m_SubmissionTempDir;
        std::atomic<bool> m_ShouldExit{ false };

        Impl() = default;

        Impl(Core::Configurator& cfg)
        : m_JudgerPool{ nullptr }
        , m_ErrorHandlerThread{ nullptr }
        , m_FutureQueue{}
        , m_SubmissionTempDir{}
        {
            size_t judger_num = cfg.get<int>("application", "JUDGER_NUMBER", std::thread::hardware_concurrency());
            m_JudgerPool = std::make_unique<Core::ThreadPool>(judger_num);
            m_SubmissionTempDir = fs::path{ cfg.get<std::string>("application", "SUBMISSION_TEMP_DIR", "./submission_temp/" ) };
            Exceptions::checkFileExists(m_SubmissionTempDir.c_str());
            Exceptions::checkFileWritable(m_SubmissionTempDir.c_str());
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
        Core::Configurator cfg{ conf_file };
        Judge::CppActuator::initSystem();
        this->m_ImplPtr = std::make_unique<Impl>(cfg);
        this->m_HttpServer = std::make_unique<httplib::Server>();
    }

    void Application::run(std::string_view host, uint16_t port)
    {
        m_ImplPtr->m_ErrorHandlerThread = std::make_unique<std::thread>(
            [this]() -> void {
                while (!m_ImplPtr->m_ShouldExit) {
                    std::future<Judge::Submission> sub{};
                    if (m_ImplPtr->m_FutureQueue.pop(sub)) {
                        Judge::Submission sub_copy{};
                        try {
                            sub_copy = sub.get();
                            // 如果没有异常 , 删除用户临时代码目录
                            fs::remove(std::get<fs::path>(sub_copy.uploade_code_file_path).parent_path());
                        } catch (const Exceptions::FileException& e) {
                            LOG_ERROR("{}\n", e.what());
                            this->submit(std::move(sub_copy)); // try again
                        } catch (const std::exception& e) {
                            LOG_ERROR("Unexpected exception in Judger Thread\n");
                        }
                    } else {
                        std::this_thread::sleep_for(std::chrono::seconds(1));
                    }
                }
            }
        );

        this->m_HttpServer->listen(host.data(), port);
        m_ImplPtr->m_ShouldExit = true;
        m_ImplPtr->m_ErrorHandlerThread->join();
    }

    using namespace Judge::Language;

    void Application::submit(Judge::Submission &&submission)
    {
        auto file_extension{ getFileExtension(submission.language_id) };
        std::string source_path{ m_ImplPtr->m_SubmissionTempDir / (boost::uuids::to_string(submission.submission_id) + file_extension) };
        std::shared_ptr<asio::io_context> p_ioc{};
        try {
            Core::FileWriter writer{ *p_ioc };
            writer.write(source_path.c_str(), std::get<std::string>(submission.uploade_code_file_path));
        } catch (const std::exception &e) {
            LOG_ERROR("Write Code File Error");
        }
        submission.uploade_code_file_path = fs::path(source_path);
        auto error{ m_ImplPtr->m_JudgerPool->enqueue([this, p_ioc, submission]() -> Judge::Submission {
                    auto result = processSubmission(submission, *p_ioc);
                    // TODO: Send Result Back to Client
                    // TODO: Record Result To DB

                    
                    return submission; // 如果出错 , 用来恢复
                }
            )
        };
        m_ImplPtr->m_FutureQueue.bounded_push(std::move(error));
    }
    
    Judge::JudgeResult Application::processSubmission(const Judge::Submission& sub, asio::io_context& ioc)
    {
        // 读数据库 ， 找测试用例 ， 但是先阻塞返回一个生成器 ，
        // 如果编译失败可以直接返回
        // 运行每个测试前才正真读取数据库测试用例

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

        // 1. 从数据库读取题目限制
        // 2. 从题目读取输入输出 , 但是阻塞返回一个生成器
        /*
        while (!gen.done()) {
            auto [stdin, expect_output ] = gen();
            auto test_result = actuator->execute;
            检查是否超时 , 是否超内存 , 是否非0退出码 , 非0信号
            通过检查则对比输出结果， 如果一致 ， 则继续 ， 不一致 ， 立刻退出
        }
        */
        return result;
    }
}