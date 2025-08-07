#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "judgedb/JudgeWriter.hpp"
#include "Logger.hpp"

namespace JudgeDB
{

    JudgeDB::JudgeWriter::JudgeWriter(std::string_view host
        , std::string_view user
        , std::string_view password
        , std::string_view database)
        : MySQLDB::Database{ host, user, password, database }
    {
    }
    
    using Judge::Language::getLanguage;
    void JudgeWriter::createSubmission(const Judge::Submission &submission, const std::string &source_code)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt (
                m_ConnPtr->prepareStatement(
R"SQL(SELECT id
FROM users
WHERE username=?;)SQL"
                )
            );
            pstmt->setString(1, submission.user_name);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (!res->next()) 
                throw sql::SQLException("user not exists");
            uint32_t user_id = res->getUInt("id");
            
            pstmt.reset(
                m_ConnPtr->prepareStatement(
R"SQL(SELECT id
FROM problems
WHERE title=?;)SQL"
                )
            );
            pstmt->setString(1, submission.problem_title);
            res.reset(pstmt->executeQuery());
            if (!res->next())
                throw sql::SQLException("problem not exists");
            uint32_t problem_id = res->getUInt("id");

            pstmt.reset(
                m_ConnPtr->prepareStatement(
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
            throw sql::SQLException(std::format("create submission failed: {}", e.what()));
        }
    }

    using Core::Types::timeStampToMySQLString;
    void JudgeWriter::writeJudgeResult(const Judge::JudgeResult &judge_result)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_ConnPtr->prepareStatement(
R"SQL(UPDATE submissions
SET overall_status=?;)SQL"
                )
            );
            pstmt->setString(1, Judge::toString(judge_result.status()));
            pstmt->executeUpdate();
            
            for (size_t i{0}; i < judge_result.results.size(); ++i) {
                pstmt.reset(
                    m_ConnPtr->prepareStatement(
R"SQL(INSERT INTO judge_results (submission_id, test_case_id, status, cpu_time_us, memory_kb, exit_code, signal_code, create_at)
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
            throw sql::SQLException(std::format("write judge result failed: {}", e.what()));
        }
    }

    void JudgeWriter::updateSubmission(SubID sub_id, Judge::SubmissionStatus status)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_ConnPtr->prepareStatement(
                    std::format("UPDATE submissions SET overall_status = ? WHERE id = ?;").data()
                )
            );
            pstmt->setString(1, Judge::toString(status));
            pstmt->setString(2, boost::uuids::to_string(sub_id));
            pstmt->executeUpdate();
        } catch (const sql::SQLException &e) {
            throw sql::SQLException(std::format("update submission status failed: {}", e.what()));
        }
    }

    void JudgeWriter::insertTestCases(const std::vector<TestCase>& test_cases, std::string_view problem_title)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_ConnPtr->prepareStatement(
R"SQL(INSERT INTO test_cases (problem_id, stdin, expected_output, sequence, is_hidden)
VALUES (?, ?, ?, ?, ?);)SQL"
                )
            );
            for (const auto& tc : test_cases) {
                pstmt->setString(1, problem_title.data());
                pstmt->setString(2, tc.stdin.data());
                pstmt->setString(3, tc.expected_output.data());
                pstmt->setInt(4, tc.sequence);
                pstmt->setBoolean(5, !tc.is_hidden);
                pstmt->executeUpdate();
            }
        } catch (const sql::SQLException& e) {
            LOG_ERROR("insert test cases failed: {}", e.what());
            throw sql::SQLException(std::format("insert test case failed: {}", e.what()));
        }
    }
}
