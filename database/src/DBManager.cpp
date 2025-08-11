#include <mysql_driver.h>
#include <cppconn/prepared_statement.h>
#include "DBManager.hpp"
#include "Logger.hpp"

namespace Database
{
    TestCasesGenerator::TestCasesGenerator(std::coroutine_handle<promise_type> handler) 
        : handler{ handler }
    { }

    bool TestCasesGenerator::next() {
        handler.resume();
        return !handler.done();
    }
    TestCase TestCasesGenerator::getCurrentTestCase() {
        if (handler.promise().eptr) {
            std::rethrow_exception(handler.promise().eptr);
        }
        return handler.promise().current_test_case.value();
    }
    TestCasesGenerator::~TestCasesGenerator() {
        if (handler)
            handler.destroy();
    }

    DBManager::DBManager(std::string_view host, uint16_t port, std::string_view user, std::string_view password, std::string_view database)
        : m_Host{ host }
        , m_Port{ port }
        , m_User{ user }
        , m_Password{ password }
        , m_Database{ database }
        , m_Connection{ nullptr }
    {
        this->connect();
    }

    DBManager::~DBManager()
    {
        if (m_Connection) {
            m_Connection->close();
            m_Connection.reset();
        }
    }

    ResourceLimits DBManager::queryProblemLimits(std::string_view problem_title, LangID language_id)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(SELECT time_limit_ms, extra_time_ms, wall_time_ms, memory_limit_kb, stack_limit_kb
FROM problem_limits pl JOIN problems p ON (pl.problem_id = p.id)
WHERE p.problem_title = ? AND pl.language = ?;)SQL"
            )};
            pstmt->setString(1, problem_title.data());
            pstmt->setString(2, Judge::Language::toString(language_id));


            std::unique_ptr<sql::ResultSet> res{ pstmt->executeQuery() };
            if (!res->next()) {  
                throw sql::SQLException("Unexpected error during querying from problem_limits: no such language id or problem");
            }
            Judge::ResourceLimits lim{
                .time_limit_s = static_cast<float>(res->getUInt(1)) / 1000.0f,
                .extra_time_s = static_cast<float>(res->getUInt(2)) / 1000.0f,
                .wall_time_s = static_cast<float>(res->getUInt(3)) / 1000.0f,
                .memory_limit_kb = res->getUInt(4),
                .stack_limit_kb = res->getUInt(5)
            };
            return lim;
        } catch (sql::SQLException &e) {
            LOG_ERROR("SQLException in Database::queryProblemLimits, code: {}, what: {}", e.getErrorCode(), e.what());
            throw e;
        }
    }

    UserInfo DBManager::queryUserInfoByName(std::string_view username)
    {
        try {
            LOG_INFO("Searching user from users: {}", username.data());
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(SELECT user_uuid, password_hash, email, salt
FROM users WHERE username = ?;)SQL"
            )};
            pstmt->setString(1, username.data());
            std::unique_ptr<sql::ResultSet> res{ pstmt->executeQuery() };
            if (!res->next())
                throw sql::SQLException("Unexpected error during querying from users: no such user");
            return {
                .user_uuid{ res->getString("user_uuid") },
                .username{ username.data() },
                .password_hash{ res->getString("password_hash") },
                .email{ res->getString("email") },
                .salt{ res->getString("salt") }
            };
        } catch (sql::SQLException &e) {
            LOG_ERROR("SQLException in Database::queryUserInfo, code: {}, what: {}", e.getErrorCode(), e.what());
            throw e;
        }
    }

    UserInfo DBManager::queryUserInfoByUUID(std::string_view uuid)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(SELECT username, password_hash, email, salt
FROM users WHERE user_uuid = ?;)SQL"
            )};
            pstmt->setString(1, uuid.data());
            std::unique_ptr<sql::ResultSet> res{ pstmt->executeQuery() };
            if (!res->next())
                throw sql::SQLException("Unexpected error during querying from users: no such uuid");
            return {
                .user_uuid{ uuid.data() },
                .username{ res->getString("username") },
                .password_hash{ res->getString("password_hash") },
                .email{ res->getString("email") },
                .salt{ res->getString("salt") }
            };
        } catch (sql::SQLException &e) {
            LOG_ERROR("SQLException in Database::queryUserInfoByUUID, code: {}, what: {}", e.getErrorCode(), e.what());
            throw e;
        }
    }

    TestCasesGenerator DBManager::queryTestCases(std::string_view problem_title)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(SELECT stdin, expected_output, sequence
FROM test_cases tcs JOIN problems p ON (tcs.problem_id = p.id)
WHERE p.title = ? ORDER BY tcs.sequence ASC;)SQL"
            )};
            pstmt->setString(1, problem_title.data());

            std::unique_ptr<sql::ResultSet> res{ pstmt->executeQuery() };
            if (!res->next())
                throw sql::SQLException("Unexpected error during querying from test_cases: no such problem title");

            do {
                TestCase tc{
                    .stdin_data{ res->getString(1) },
                    .expected_output{ res->getString(2) },
                    .sequence{ res->getUInt(3) }
                };
                co_yield tc;
            } while (res->next());
        } catch (sql::SQLException &e) {
            LOG_ERROR("SQLException in Database::queryTestCases, code: {}, what: {}", e.getErrorCode(), e.what());
            throw e;
        }
    }

    DBTask DBManager::insertProblemLimits(std::string problem_title, LangID language_id, Judge::ResourceLimits limits)
    {
        try {
            m_Connection->setAutoCommit(false);  // 开始事务
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(WITH p AS (SELECT id FROM problems WHERE title = ? LIMIT 1)
INSERT INTO problem_limits pl SET pl.problem_id = (SELECT id FROM p), language = ?, time_limit_ms = ?, extra_time_ms = ?, wall_time_ms = ?, memory_limit_kb = ?, stack_limit_kb = ?
ON DUPLICATE KEY UPDATE time_limit_ms = ?, extra_time_ms = ?, wall_time_ms = ?, memory_limit_kb = ?, stack_limit_kb = ?;)SQL"  // 修复子查询语法
            )};
            pstmt->setString(1, problem_title.data());
            pstmt->setString(2, Judge::Language::toString(language_id));
            pstmt->setUInt(3, static_cast<uint32_t>(limits.time_limit_s * 1000.f));
            pstmt->setUInt(4, static_cast<uint32_t>(limits.extra_time_s * 1000.f));
            pstmt->setUInt(5, static_cast<uint32_t>(limits.wall_time_s * 1000.f));
            pstmt->setUInt(6, static_cast<uint32_t>(limits.memory_limit_kb));
            pstmt->setUInt(7, static_cast<uint32_t>(limits.stack_limit_kb));
            pstmt->setUInt(8, static_cast<uint32_t>(limits.time_limit_s * 1000.f));
            pstmt->setUInt(9, static_cast<uint32_t>(limits.extra_time_s * 1000.f));
            pstmt->setUInt(10, static_cast<uint32_t>(limits.wall_time_s * 1000.f));
            pstmt->setUInt(11, static_cast<uint32_t>(limits.memory_limit_kb));
            pstmt->setUInt(12, static_cast<uint32_t>(limits.stack_limit_kb));
            
            if (pstmt->executeUpdate() != 1) {
                throw sql::SQLException("Unexpected error: no affected rows or more than one row inserted");
            }
            
            m_Connection->commit();  // 提交事务
            m_Connection->setAutoCommit(true);  // 恢复自动提交
        } catch (sql::SQLException &e) {
            try { m_Connection->rollback(); } catch (...) {}  // 回滚事务
            m_Connection->setAutoCommit(true);  // 恢复自动提交
            LOG_ERROR("SQLException in Database::insertProblemLimits, code: {}, what: {}", e.getErrorCode(), e.what());
        }
        co_return;
    }

    DBTask DBManager::insertSubmission(Judge::Submission submission, Judge::SubmissionStatus status)
    {
        try {
            m_Connection->setAutoCommit(false);  // 开始事务
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(WITH p AS (SELECT id FROM problems WHERE title = ? LIMIT 1),
s AS (SELECT id FROM users WHERE user_uuid = ? LIMIT 1) 
INSERT INTO submissions sub SET sub.sub_strid = ?, sub.problem_id = (SELECT id FROM p), sub.user_id = (SELECT id FROM s), 
sub.language = ?, sub.code = ?, sub.overall_status = ?;)SQL"
            ) };
            pstmt->setString(1, submission.problem_title);
            pstmt->setString(2, submission.user_uuid);
            pstmt->setString(3, submission.submission_id);
            pstmt->setString(4, Judge::Language::toString(submission.language_id));
            pstmt->setString(5, submission.source_code);
            pstmt->setString(6, Judge::toString(status));
            
            if (pstmt->executeUpdate() != 1) {
                throw sql::SQLException("Unexpected error: no affected rows or more than one row inserted");
            }

            m_Connection->commit();  // 提交事务
            m_Connection->setAutoCommit(true);  // 恢复自动提交
            
        } catch (sql::SQLException &e) {
            try { m_Connection->rollback(); } catch (...) {}  // 回滚事务
            m_Connection->setAutoCommit(true);  // 恢复自动提交
            LOG_ERROR("SQLException in Database::insertSubmission, code: {}, what: {}", e.getErrorCode(), e.what());
        }
        co_return;
    }


    DBTask DBManager::insertJudgeResult(Judge::JudgeResult judge_result)
    {
        try {
            m_Connection->setAutoCommit(false);  // 开始事务
            
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(
                INSERT INTO judge_results (submission_id, test_case_id, status, cpu_time_ms, memory_kb, exit_code, signal_code)
                VALUES (
                    (SELECT id FROM submissions WHERE sub_strid = ? LIMIT 1),
                    (SELECT id FROM test_cases 
                    WHERE problem_id = (SELECT id FROM problems WHERE title = ?) 
                    AND sequence = ? 
                    LIMIT 1),
                    ?, ?, ?, ?, ?
                )SQL"
            )};

            for (uint32_t i{0}; i < judge_result.results.size(); ++i) {
                const Judge::TestResult& tc{ judge_result.results[i] };
                pstmt->setString(1, judge_result.sub_id);
                pstmt->setString(2, judge_result.problem_title);
                pstmt->setUInt(3, i + 1);  // sequence 从 1 开始
                pstmt->setString(4, Judge::toString(tc.status));
                pstmt->setUInt(5, static_cast<uint32_t>(tc.duration_us / 1000));
                pstmt->setUInt(6, tc.memory_kb);
                pstmt->setInt(7, tc.exit_code);
                pstmt->setInt(8, tc.signal);
                
                if (pstmt->executeUpdate() != 1) {
                    m_Connection->rollback();
                    m_Connection->setAutoCommit(true);
                    throw sql::SQLException("Unexpected error: no affected rows or more than one row inserted");
                }
            }
            
            m_Connection->commit();
            m_Connection->setAutoCommit(true);
            
        } catch (sql::SQLException &e) {
            try { m_Connection->rollback(); } catch (...) {}
            m_Connection->setAutoCommit(true);
            LOG_ERROR("SQLException in Database::insertJudgeResult, code: {}, what: {}", e.getErrorCode(), e.what());
        }
        co_return;
    }

    DBTask DBManager::insertTestCases(std::vector<TestCase>&& test_cases, std::string problem_title)
    {
        try {
            m_Connection->setAutoCommit(false); // 开启事务
            
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(
                INSERT INTO test_cases (problem_id, stdin, expected_output, sequence)
                SELECT 
                    (SELECT id FROM problems WHERE title = ? LIMIT 1),
                    ?, ?, ?
                )SQL"
            )};

            for (const auto& tc : test_cases) {
                pstmt->setString(1, problem_title.data());
                pstmt->setString(2, tc.stdin_data);
                pstmt->setString(3, tc.expected_output);
                pstmt->setUInt(4, tc.sequence);
                
                if (pstmt->executeUpdate() != 1) {
                    m_Connection->rollback();
                    m_Connection->setAutoCommit(true);
                    throw sql::SQLException("Unexpected error: no affected rows or more than one row inserted");
                }
            }
            
            m_Connection->commit();
            m_Connection->setAutoCommit(true);
            
        } catch (sql::SQLException &e) {
            try { m_Connection->rollback(); } catch (...) {}
            m_Connection->setAutoCommit(true);
            LOG_ERROR("SQLException in Database::insertTestCases, code: {}, what: {}", e.getErrorCode(), e.what());
        }
        co_return;
    }

    DBTask DBManager::insertProblem(ProblemInfo problem)
    {
        try {
            m_Connection->setAutoCommit(false);  // 开始事务
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(WITH s AS (SELECT id FROM users WHERE user_uuid = ? LIMIT 1)
                INSERT INTO problems p SET p.title = ?, p.difficulty = ?, description = ?, author_id = (SELECT id FROM s);)SQL"  // 修复子查询语法
            ) };
            pstmt->setString(1, problem.author_uuid);
            pstmt->setString(2, problem.title);
            pstmt->setString(3, Core::Types::difficultyToString(problem.level)); 
            pstmt->setString(4, problem.description);
            
            if (pstmt->executeUpdate() != 1) {
                throw sql::SQLException("Unexpected error: no affected rows or more than one row inserted");
            }
            m_Connection->commit();  // 提交事务
            m_Connection->setAutoCommit(true);  // 恢复自动提交
            
        } catch (sql::SQLException &e) {
            try { m_Connection->rollback(); } catch (...) {}  // 回滚事务
            m_Connection->setAutoCommit(true);  // 恢复自动提交
            LOG_ERROR("SQLException in Database::insertProblem, code: {}, what: {}", e.getErrorCode(), e.what());
        }
        co_return;
    }

    DBTask DBManager::insertUser(UserInfo user)
    {
        try {
            m_Connection->setAutoCommit(false);  // 开始事务
            std::unique_ptr<sql::PreparedStatement> pstmt{ m_Connection->prepareStatement(
                R"SQL(INSERT INTO users (username, password_hash, email, user_uuid, salt)
VALUES (?, ?, ?, ?, ?);)SQL"
            ) };
            pstmt->setString(1, user.username);
            pstmt->setString(2, user.password_hash);
            pstmt->setString(3, user.email);
            pstmt->setString(4, user.user_uuid);
            pstmt->setString(5, user.salt);
            
            if (pstmt->executeUpdate() != 1) {
                throw sql::SQLException("Unexpected error: no affected rows or more than one row inserted");
            }
            m_Connection->commit();  // 提交事务
            m_Connection->setAutoCommit(true);  // 恢复自动提交
            
        } catch (sql::SQLException &e) {
            try { m_Connection->rollback(); } catch (...) {}  // 回滚事务
            m_Connection->setAutoCommit(true);  // 恢复自动提交
            LOG_ERROR("SQLException in Database::insertUser, code: {}, what: {}", e.getErrorCode(), e.what());
        }
        co_return;
    }

    bool DBManager::isUsernameExist(std::string_view username)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt{m_Connection->prepareStatement(
                R"SQL(SELECT * FROM users u WHERE u.username = ? LIMIT 1;)SQL")};
            pstmt->setString(1, username.data());
            std::unique_ptr<sql::ResultSet> res{pstmt->executeQuery()};
            return res->next();
        } catch (sql::SQLException &e) {
            LOG_ERROR("SQLException in Database::isUsernameExist, code: {}, what: {}", e.getErrorCode(), e.what());
            return false;
        }
    }

    bool DBManager::isEmailExist(std::string_view email)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt{m_Connection->prepareStatement(
                R"SQL(SELECT * FROM users u WHERE u.email = ? LIMIT 1;)SQL")};
            pstmt->setString(1, email.data());
            std::unique_ptr<sql::ResultSet> res{pstmt->executeQuery()};
            return res->next();
        } catch (sql::SQLException &e) {
            LOG_ERROR("SQLException in Database::isEmailExist, code: {}, what: {}", e.getErrorCode(), e.what());
            return false;
        }
    }


    void DBManager::connect()
    {
        sql::mysql::MySQL_Driver *driver{sql::mysql::get_mysql_driver_instance()};
        sql::ConnectOptionsMap connectionOptions;
        connectionOptions["hostName"] = m_Host.data();
        connectionOptions["userName"] = m_User.data();
        connectionOptions["password"] = m_Password.data();
        connectionOptions["schema"] = m_Database.data();
        m_Connection.reset(driver->connect(connectionOptions));
    }
}
