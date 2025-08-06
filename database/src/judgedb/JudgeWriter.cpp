#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <mysql_connection.h>
#include <mysql_driver.h>
#include "judgedb/JudgeWriter.hpp"
#include "judgedb/JudgeWriter.hpp"

namespace JudgeDB
{
    void JudgeWriter::connect() {
        sql::Driver* driver = sql::mysql::get_driver_instance();
        sql::ConnectOptionsMap options;
        options["hostName"] = m_Host;
        options["userName"] = m_User;
        options["password"] = m_Password;
        options["schema"] = m_Database;
        m_Conn.reset(driver->connect(options));
    }

    JudgeDB::JudgeWriter::JudgeWriter(std::string_view host
        , std::string_view user
        , std::string_view password
        , std::string_view database)
        : m_Conn{ nullptr }
        , m_Host{ host.data() }
        , m_User{ user.data() }
        , m_Password{ password.data() }
        , m_Database{ database.data() }
    {
    }

    JudgeWriter::~JudgeWriter()
    {
        if (m_Conn) {
            m_Conn->close();
            m_Conn.reset();
        }
    }
    
    using Judge::Language::getLanguage;
    void JudgeWriter::createSubmission(const Judge::Submission &submission, const std::string &source_code)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt (
                m_Conn->prepareStatement(
R"SQL(SELECT id
FROM users
WHERE username=?;)SQL"
                )
            );
            pstmt->setString(1, submission.user_name);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (!res->next()) 
                throw sql::SQLException("用户不存在");
            uint32_t user_id = res->getUInt("id");
            
            pstmt.reset(
                m_Conn->prepareStatement(
R"SQL(SELECT id
FROM problems
WHERE title=?;)SQL"
                )
            );
            pstmt->setString(1, submission.problem_title);
            res.reset(pstmt->executeQuery());
            if (!res->next())
                throw sql::SQLException("试题不存在");
            uint32_t problem_id = res->getUInt("id");

            pstmt.reset(
                m_Conn->prepareStatement(
                    "INSERT INTO submissions (user_id, problem_id, language, code, overall_status) "
                    "VALUES (?, ?, ?, ?, 'PENDING')"
                )
            );
            pstmt->setInt(1, user_id);
            pstmt->setInt(2, problem_id);
            pstmt->setString(3, getLanguage(submission.language_id));
            pstmt->setString(4, source_code);
            pstmt->executeUpdate();
        } catch (const sql::SQLException& e) {
            throw sql::SQLException(std::format("创建提交失败: {}", e.what()));
        }
    }

    using Core::Types::timeStampToMySQLString;
    void JudgeWriter::writeJudgeResult(const Judge::JudgeResult &judge_result)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_Conn->prepareStatement(
R"SQL(UPDATE submissions
SET overall_status=?;)SQL"
                )
            );
            pstmt->setString(1, Judge::toString(judge_result.status()));
            pstmt->executeUpdate();
            
            for (size_t i{0}; i < judge_result.results.size(); ++i) {
                pstmt.reset(
                    m_Conn->prepareStatement(
R"SQL(INSERT INTO judge_results (submission_id, test_case_id, status, cpu_time_us, memory_kb, exit_code, signal, create_at)
VALUES (?, ?, ?, ?, ?, ?, ?, ?);)SQL"
                    )
                );
                pstmt->setString(1, boost::uuids::to_string(judge_result.sub_id));
                pstmt->setInt(2, i);
                pstmt->setString(3, Judge::toString(judge_result.results[i].status));
                pstmt->setInt64(4, judge_result.results[i].duration_us);
                pstmt->setInt(5, judge_result.results[i].memory_kb);
                pstmt->setInt(6, judge_result.results[i].exit_code);
                pstmt->setInt(7, judge_result.results[i].signal);
                pstmt->setString(8, timeStampToMySQLString(judge_result.results[i].create_at));
                pstmt->executeUpdate();
            }
        } catch (const sql::SQLException &e) {
            throw sql::SQLException(std::format("写入评测结果失败: {}", e.what()));
        }
    }

    void JudgeWriter::updateSubmission(SubID sub_id, Judge::SubmissionStatus status)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_Conn->prepareStatement(
                    std::format("UPDATE submissions SET overall_status = ? WHERE id = ?;").data()
                )
            );
            pstmt->setString(1, Judge::toString(status));
            pstmt->setString(2, boost::uuids::to_string(sub_id));
            pstmt->executeUpdate();
        } catch (const sql::SQLException &e) {
            throw sql::SQLException(std::format("更新提交状态失败: {}", e.what()));
        }
    }
}
