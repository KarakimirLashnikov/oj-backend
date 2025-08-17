#include <sodium.h>
#include <openssl/evp.h>
#include "Application.hpp"
#include "Configurator.hpp"
#include "ThreadPool.hpp"
#include "FileWriter.hpp"
#include "FileException.hpp"
#include "SystemException.hpp"
#include "utilities/JudgeResult.hpp"
#include "Judger.hpp"
#include "DbManager.hpp"
#include "RedisManager.hpp"
#include "services/AuthService.hpp"
#include "dbop/DbOpFactory.hpp"

namespace OJApp
{
    std::unique_ptr<Application> Application::s_AppPtr{ nullptr };

    struct Application::Impl
    {
        std::unique_ptr<Core::ThreadPool> m_JudgerPool;
        std::unique_ptr<Core::Configurator> m_Configurator;
        std::unique_ptr<DbManager> m_DbManager;
        std::unique_ptr<RedisManager> m_RedisManager;
        std::unique_ptr<sw::redis::Subscriber> m_DbOpSubscriber;
        std::unique_ptr<AuthService> m_AuthService;
        fs::path m_SubmissionTempDir;
        std::atomic<bool> m_ShouldExit{ false };

        Impl() = default;

        Impl(std::string_view conf_file_path)
        : m_JudgerPool{ nullptr }
        , m_DbManager{}
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

            m_DbManager = std::make_unique<DbManager>(*m_Configurator);
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
        Judge::Actuator::initSystem(this->getConfigurator());
    }

    void Application::run(std::string_view host, uint16_t port)
    {
        m_ImplPtr->m_JudgerPool->enqueue([this]() {
            while (!m_ImplPtr->m_ShouldExit) {
                try {
                    // 长连接消费消息
                    LOG_INFO("Start polling...");
                    m_ImplPtr->m_DbOpSubscriber->consume();
                } catch (const sw::redis::TimeoutError &e) {
                    LOG_WARN("Redis timeout, retrying...");
                    continue;  // 直接重试，不重建连接
                } catch (const sw::redis::ClosedError &e) {
                    LOG_ERROR("Connection closed, reconnecting...");
                    // 重建 Subscriber（限流：每分钟最多重试 1 次）
                    std::this_thread::sleep_for(std::chrono::seconds(60));
                    m_ImplPtr->m_DbOpSubscriber = std::make_unique<sw::redis::Subscriber>(
                        m_ImplPtr->m_RedisManager->getRedis()->subscriber());
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

    DbManager& Application::getDbManager() {
        return *(m_ImplPtr->m_DbManager);
    }

    RedisManager& Application::getRedisManager() {
        return *(m_ImplPtr->m_RedisManager);
    }

    AuthService& Application::getAuthService() {
        return *(m_ImplPtr->m_AuthService);
    }

    using DbOp::makeDbOp;
    void Application::processDbOperateEvent(std::string channel, std::string msg) {
        m_ImplPtr->m_JudgerPool->enqueue([this, message = msg]()->void {
                try {
                    LOG_INFO("db_op message receive");
                    DbOperateMessage op_msg{ DbOperateMessage::deserialize(message) };
                    auto db_op = makeDbOp(op_msg.op_type, std::move(op_msg.sql), std::move(op_msg.param_array));
                    m_ImplPtr->m_DbManager->execute(db_op.get());
                    LOG_INFO("db_op message done");
                } catch (...) {
                    LOG_ERROR("DbOperateEvent failed, message: {}", message);
                }
            }
        );
    }

    using Exceptions::FileException;
    using Core::Types::getFileExtension;
    using DbOp::OpType;
    using DbOp::DbQueryOp;
    void Application::processJudgeTask(SubmissionInfo info)
    {
        App.getRedisManager().publishDbOperate(DbOperateMessage{
                .op_type = OpType::Insert,
                .sql = R"SQL(INSERT INTO submissions (sub_uuid, user_id, problem_id, language, code, overall_status) 
VALUES (?, (SELECT id FROM users WHERE username = ? LIMIT 1), (SELECT id FROM problems WHERE title = ? LIMIT 1), ?, ?, ?);)SQL",
                .data_key = info.submission_id,
                .param_array = { info.submission_id
                    , info.username
                    , info.problem_title
                    , LanguageIdtoString(info.language_id)
                    , info.source_code
                    , Judge::submissionStatusToString(Judge::SubmissionStatus::PENDING)
                }
            }
        );
        m_ImplPtr->m_JudgerPool->enqueue([this, sub = std::move(info)] -> void {
                try {
                    fs::path code_dir{ m_ImplPtr->m_SubmissionTempDir / sub.submission_id };
                    if (!fs::create_directory(code_dir))
                        throw FileException(FileException::ErrorType::WriteError, code_dir.string(), "fiailed create temp dir to save code");
                    asio::io_context ic{};
                    fs::path code_file = code_dir / ("main" + getFileExtension(sub.language_id));

                    {
                        Core::FileWriter writer(ic);
                        writer.write(code_file.string(), sub.source_code);
                    }

                    auto db_op = makeDbOp(OpType::Query
                                , R"SQL(SELECT * FROM test_cases tc JOIN problems p ON tc.problem_id = p.id WHERE p.title = ? ORDER BY tc.sequence ASC;)SQL"
                                , { sub.problem_title });
                    auto tcs = App.getDbManager().query(dynamic_cast<DbQueryOp*>(db_op.get()));

                    db_op = makeDbOp(OpType::Query
                                , R"SQL(SELECT * FROM problem_limits pl JOIN problems p ON pl.problem_id = p.id WHERE p.title = ? AND pl.language = ? LIMIT 1;)SQL"
                                , { sub.problem_title, LanguageIdtoString(sub.language_id) });
                    auto limit = App.getDbManager().query(dynamic_cast<DbQueryOp*>(db_op.get())).front();
                    LOG_INFO("Query problem_limit, limit: {}", limit.dump());
                    LimitsInfo lm_info{};
                    lm_info.time_limit_s = static_cast<float>(limit.at("time_limit_ms").get<uint32_t>()) / 1000.f;
                    lm_info.extra_time_s = static_cast<float>(limit.at("extra_time_ms").get<uint32_t>()) / 1000.f;
                    lm_info.wall_time_s = static_cast<float>(limit.at("wall_time_ms").get<uint32_t>()) / 1000.f;
                    lm_info.memory_limit_kb = limit.at("memory_limit_kb").get<uint32_t>();
                    lm_info.stack_limit_kb = limit.at("stack_limit_kb").get<uint32_t>();

                    if (!ic.stopped()) {
                        ic.stop();
                    }

                    Judge::JudgeResult judge_result{sub.problem_title, sub.submission_id, sub.language_id};

                    Judge::Judger judger{sub.language_id, code_file, lm_info};
                    std::vector<DbOperateMessage> msgs;
                    msgs.reserve(tcs.size());
                    uint32_t idx{1};
                    for (const auto& tc : tcs) {
                        TestCaseInfo tc_info{};
                        tc_info.stdin_data = tc.at("stdin_data").get<std::string>();
                        tc_info.expected_output = tc.at("expected_output").get<std::string>();
                        auto tc_result = judger.judge(tc_info.stdin_data, tc_info.expected_output);

                        msgs.push_back(DbOperateMessage{
                            .op_type = OpType::Insert,
                            .sql = R"SQL(INSERT INTO judge_results (submission_id, test_case_id, status, cpu_time_ms, memory_kb, exit_code, signal_code) 
VALUES ((SELECT id FROM submissions WHERE sub_uuid = ?),
        (SELECT id FROM test_cases WHERE problem_id = (SELECT id FROM problems WHERE title = ?) AND sequence = ?),
        ?, ?, ?, ?, ?);)SQL",
                            .data_key = sub.submission_id + std::to_string(idx),
                            .param_array = { sub.submission_id
                                    , sub.problem_title
                                    , idx++
                                    , Judge::testStatusToString(tc_result.status)
                                    , static_cast<uint32_t>(tc_result.duration_us / 1000)
                                    , tc_result.memory_kb
                                    , tc_result.exit_code
                                    , tc_result.signal
                                }
                            }
                        );

                        judge_result.results.push_back(std::move(tc_result));

                        if (judge_result.results.back().status != Judge::TestStatus::ACCEPTED)
                            break;
                    }

                    judge_result.setStatus();

                    App.getRedisManager().publishDbOperate(DbOperateMessage{
                        .op_type = OpType::Update,
                        .sql = R"SQL(UPDATE submissions SET overall_status = ? WHERE sub_uuid = ?;)SQL",
                        .data_key = sub.submission_id + "_overall_status",
                        .param_array = { 
                                Judge::submissionStatusToString(judge_result.status()),
                                sub.submission_id
                            }
                        }
                    );
                    
                    for (const auto& m : msgs) {
                        App.getRedisManager().publishDbOperate(m);
                    }

                    if (!fs::remove_all(code_dir))
                        LOG_ERROR("submission file {} remove failed", code_dir.string());

                    App.getRedisManager().set("submission_tmp_" + sub.submission_id, "COMPLETE");
                } catch (const njson::exception& e) {
                    LOG_ERROR("JudgeTask parse json error: {}", e.what());
                } catch (const sql::SQLException& e) {
                    LOG_ERROR("JudgeTask query db error: {}", e.what());
                } catch (const std::exception& e) {
                    LOG_ERROR("JudgeTask std exception: {}", e.what());
                } catch (...) {
                    LOG_ERROR("JudgeTask failed, Submission ID: {}", sub.submission_id);
                }
            }
        );
    }
}