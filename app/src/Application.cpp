#include <sodium.h>
#include <openssl/evp.h>
#include "Application.hpp"
#include "Types.hpp"
#include "Configurator.hpp"
#include "ThreadPool.hpp"
#include "FileWriter.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/Language.hpp"
#include "FileException.hpp"
#include "SystemException.hpp"
#include "Queue.hpp"
#include "Judger.hpp"
#include "DBManager.hpp"
#include "RedisManager.hpp"
#include "AuthService.hpp"

namespace OJApp
{
    std::unique_ptr<Application> Application::s_AppPtr{ nullptr };

    struct Map{};

    using Core::Queue;
    struct Application::Impl
    {
        std::unique_ptr<Core::ThreadPool> m_JudgerPool;
        std::unique_ptr<Core::Configurator> m_Configurator;
        std::unique_ptr<DBManager> m_DBManager;
        std::unique_ptr<RedisManager> m_RedisManager;
        std::unique_ptr<sw::redis::Subscriber> m_DbOpSubscriber;
        std::unique_ptr<AuthService> m_AuthService;
        fs::path m_SubmissionTempDir;
        std::atomic<bool> m_ShouldExit{ false };

        Impl() = default;

        Impl(std::string_view conf_file_path)
        : m_JudgerPool{ nullptr }
        , m_DBManager{}
        , m_RedisManager{}
        , m_DbOpSubscriber{}
        , m_SubmissionTempDir{}
        , m_Configurator{ std::make_unique<Core::Configurator>(conf_file_path.data()) }
        {
            if (sodium_init() == -1) 
                throw Exceptions::makeSystemException("lib sodium init error");
            LOG_INFO("lib sodium init OK");

            OpenSSL_add_all_algorithms();
            LOG_INFO("OpenSSL add all algorithm OK");

            size_t wokers_num = m_Configurator->get<int>("application", "JUDGER_NUMBER", std::thread::hardware_concurrency());
            m_JudgerPool = std::make_unique<Core::ThreadPool>(wokers_num);
            LOG_INFO("judger threadpool created, with {} workers", wokers_num);

            m_DBManager = std::make_unique<DBManager>(*m_Configurator);
            LOG_INFO("redis connection OK");

            m_RedisManager = std::make_unique<RedisManager>(*m_Configurator);
            LOG_INFO("redis manager create OK");

            m_AuthService = std::make_unique<AuthService>(*m_Configurator);
            LOG_INFO("auth service create OK");

            m_DbOpSubscriber = std::make_unique<sw::redis::Subscriber>(m_RedisManager->getRedis()->subscriber());
            m_DbOpSubscriber->on_message([this](const std::string& channel, const std::string& msg) {
                App.processDbOperateEvent(channel, msg);
            });
            std::string channel{ m_Configurator->get<std::string>("redis", "OPERATE_CHANNEL", "db_operate_channel") };
            m_DbOpSubscriber->subscribe(channel); // 订阅数据库操作通道
            LOG_INFO("subscriber subscribed with topic [oj.submission.queue]");
            
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
        m_ImplPtr->m_JudgerPool->enqueue([this]() {
            while (!m_ImplPtr->m_ShouldExit) {
                try {
                    // 长连接消费消息
                    m_ImplPtr->m_DbOpSubscriber->consume();
                } catch (const sw::redis::TimeoutError &e) {
                    LOG_WARN("Redis timeout, retrying...");
                    continue;  // 直接重试，不重建连接
                } catch (const sw::redis::ClosedError &e) {
                    LOG_ERROR("Connection closed, reconnecting...");
                    // 重建 Subscriber（限流：每分钟最多重试 1 次）
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                    m_ImplPtr->m_DbOpSubscriber = std::make_unique<sw::redis::Subscriber>(m_ImplPtr->m_RedisManager->getRedis()->subscriber());
                    m_ImplPtr->m_DbOpSubscriber->subscribe("db_operate_channel");
                } catch (const std::exception &e) {
                    LOG_ERROR("Fatal error: {}", e.what());
                    break;
                }
            }
        });
        this->m_HttpServer->listen(host.data(), port);
        m_ImplPtr->m_ShouldExit = true;
    }

    Core::Configurator& Application::getConfigurator() {
        return *(m_ImplPtr->m_Configurator);
    }

    DBManager& Application::getDBManager() {
        return *(m_ImplPtr->m_DBManager);
    }

    RedisManager& Application::getRedisManager() {
        return *(m_ImplPtr->m_RedisManager);
    }

    AuthService& Application::getAuthService() {
        return *(m_ImplPtr->m_AuthService);
    }

    void Application::processDbOperateEvent(std::string channel, std::string msg) {
        m_ImplPtr->m_JudgerPool->enqueue([this, message = std::move(msg)]->void{
            DbOperateMessage op_msg{ DbOperateMessage::deserialize(message) };
            if (op_msg.op_type == DbOperateMessage::DbOpType::INSERT) {
                std::string data{ m_ImplPtr->m_RedisManager->getRedis()->get(op_msg.data_key).value() };
                LOG_INFO("message is op_insert, key: {}, value: {}", op_msg.data_key, data);
                m_ImplPtr->m_DBManager->insertOp(op_msg.table_name, njson::parse(data));
                return;
            }
        });
    }
}