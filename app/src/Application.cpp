#include "Application.hpp"
#include "Types.hpp"
#include "Configurator.hpp"
#include "ThreadPool.hpp"
#include "FileWriter.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/Language.hpp"
#include "compilers/CppCompiler.hpp"
#include "actuators/CppActuator.hpp"

namespace OJApp
{
    std::unique_ptr<Application> Application::s_AppPtr{ nullptr };

    struct Application::Impl
    {
        Core::Queue<Judge::JudgeResult> m_CompletedQueue;
        std::unique_ptr<Core::ThreadPool> m_JudgerPool;
        fs::path m_SubmissionTempDir;

        Impl() = default;

        Impl(Core::Configurator& cfg)
        : m_JudgerPool{nullptr}
        , m_SubmissionTempDir{}
        {
            size_t judger_num = cfg.get<int>("application", "JUDGER_NUMBER", std::thread::hardware_concurrency());
            m_JudgerPool = std::make_unique<Core::ThreadPool>(judger_num);
            m_SubmissionTempDir = fs::path{ cfg.get<std::string>("application", "SUBMISSION_TEMP_DIR", "./submission_temp/" ) };
            if (!fs::exists(m_SubmissionTempDir)) {
                fs::create_directories(m_SubmissionTempDir);
                LOG_WARN("Creating Submission temp directory: {}", m_SubmissionTempDir.string());
            }
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
        this->m_ImplPtr = std::make_unique<Impl>(cfg);
        this->m_HttpServer = std::make_unique<httplib::Server>();
    }

    void Application::run(std::string_view host, uint16_t port)
    {
        this->m_HttpServer->listen(host.data(), port);
        LOG_INFO("Main thread preparing to exit");
    }

    using namespace Judge::Language;

    void Application::submit(Judge::Submission &&submission)
    {
        auto file_extension{ getFileExtension(submission.language_id) };
        std::string source_path{ m_ImplPtr->m_SubmissionTempDir / (boost::uuids::to_string(submission.submission_id) + file_extension) };
        asio::io_context ioc{};
        try {
            Core::FileWriter writer{ ioc };
            writer.write(source_path.c_str(), std::get<std::string>(submission.uploade_code_file_path));
        } catch (const std::exception &e) {
            LOG_ERROR("Write Code File Error");
        }
        submission.uploade_code_file_path = fs::path(source_path);
        m_ImplPtr->m_JudgerPool->enqueue([this](Judge::Submission sub){
            
        }, submission);
    }
    
    void Application::processSubmission(Judge::Submission &&sub, asio::io_context& ioc)
    {
        // 读数据库 ， 找测试用例 ， 但是先阻塞返回一个生成器 ，
        // 如果编译失败可以直接返回
        // 运行每个测试前才正真读取数据库测试用例

        // Judge::JudgeResult result{ sub.problem, sub.submission_id };
        // 因为是生成器读取测试 ， 只要有一个错就不再评测 ， result不再需要测试用例数作为参数 ， 待改 ! ! !

        std::unique_ptr<Judge::Compiler> compiler{ nullptr };
        switch (sub.language_id)
        {
        case LangID::CPP:
            compiler = Judge::createCompiler<Judge::CppCompiler>();
            break;
        default:
            break;
        }

        if (!ioc.stopped()) {
            ioc.run(); // 保证编译文件前 ， 文件已经写到磁盘上
        }

        if (compiler) { // 需要编译器的语言
            auto source_path{ std::get<fs::path>(sub.uploade_code_file_path) };
            auto exe_path{ source_path.parent_path() / "main" };
            auto success{ compiler->compile(source_path.c_str(), exe_path.c_str(), sub.compile_options) };
            if (!success) {
                // 直接可以记录编译错误信息然后返回
                return;
            }
        }
    }
}