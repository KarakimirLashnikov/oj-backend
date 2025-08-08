#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "judgedb/JudgeWriter.hpp"
#include "Logger.hpp"

namespace JudgeDB
{

    JudgeDB::JudgeWriter::JudgeWriter(std::string_view host
                                    , uint16_t port
                                    , std::string_view user
                                    , std::string_view password
                                    , std::string_view database)
        : MySQLDB::Database{ host, port, user, password, database }
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
            pstmt->setString(1, submission.username);
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            if (!res->next()) 
                throw sql::SQLException("user not exists");
            uint32_t user_id = res->getUInt("id");
            
            pstmt.reset(m_ConnPtr->prepareStatement(
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
R"SQL(INSERT INTO submissions (submission_id, user_id, problem_id, language, code, overall_status)
VALUES (?, ?, ?, ?, ?, 'PENDING'))SQL"
                )
            );
            pstmt->setString(1, boost::uuids::to_string(submission.submission_id));
            pstmt->setUInt(2, user_id);
            pstmt->setUInt(3, problem_id);
            pstmt->setString(4, getLanguage(submission.language_id));
            pstmt->setString(5, source_code);
            pstmt->executeUpdate();
        } catch (const sql::SQLException& e) {
            throw sql::SQLException(std::format("create submission failed: {}", e.what()));
        }
    }

    void JudgeWriter::writeJudgeResult(const Judge::JudgeResult &judge_result)
    {
        // LOG_INFO("write judge result: {}", judge_result.toString());
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_ConnPtr->prepareStatement(
R"SQL(UPDATE submissions
SET overall_status=?
WHERE submission_id=?;)SQL"
                )
            );
            pstmt->setString(1, Judge::toString(judge_result.status()));
            pstmt->setString(2, judge_result.sub_id.data());
            pstmt->executeUpdate();

            pstmt.reset(m_ConnPtr->prepareStatement(
R"SQL(SELECT id FROM submissions WHERE submission_id = ?;)SQL"
                )
            );
            pstmt->setString(1, judge_result.sub_id.data());
            std::unique_ptr<sql::ResultSet> res{ pstmt->executeQuery() };
            if (!res->next())
                throw sql::SQLException("sub ID not exists");
            uint32_t submission_id{ res->getUInt("id") };

            pstmt.reset(m_ConnPtr->prepareStatement(
R"SQL(SELECT id FROM problems WHERE title = ?;)SQL"
                )
            );
            pstmt->setString(1, judge_result.problem_title.data());
            res.reset(pstmt->executeQuery());
            if (!res->next())
                throw sql::SQLException{ "problem not exists" };
            uint32_t problem_id{ res->getUInt("id") };
            
            for (size_t i{1}; i <= judge_result.results.size(); ++i) {
                pstmt.reset(m_ConnPtr->prepareStatement(
R"SQL(SELECT id FROM test_cases WHERE problem_id=? AND sequence=?;)SQL"
                    )
                );
                pstmt->setUInt(1, problem_id);
                pstmt->setInt(2, i);
                res.reset(pstmt->executeQuery());
                if (!res->next())
                    throw sql::SQLException{ "test case no exists" };
                uint32_t tc_id{ res->getUInt("id") };
                pstmt.reset(m_ConnPtr->prepareStatement(
R"SQL(INSERT INTO judge_results (submission_id, test_case_id, status, cpu_time_us, memory_kb, exit_code, signal_code)
VALUES (?, ?, ?, ?, ?, ?, ?);)SQL"
                    )
                );
                pstmt->setUInt(1, submission_id);
                pstmt->setUInt(2, tc_id);
                pstmt->setString(3, Judge::toString(judge_result.results[i-1].status));
                pstmt->setUInt(4, judge_result.results[i-1].duration_us);
                pstmt->setUInt(5, judge_result.results[i-1].memory_kb);
                pstmt->setInt(6, judge_result.results[i-1].exit_code);
                pstmt->setInt(7, judge_result.results[i-1].signal);
                pstmt->executeUpdate();
            }
        } catch (const sql::SQLException &e) {
            throw sql::SQLException(std::format("write judge result failed: {}", e.what()));
        }
    }

    void JudgeWriter::updateSubmission(std::string_view sub_id, Judge::SubmissionStatus status)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_ConnPtr->prepareStatement(
R"SQL(SELECT id FROM submissions WHERE submission_id=?;)SQL"
                )
            );
            pstmt->setString(1, sub_id.data());
            auto res{ std::unique_ptr<sql::ResultSet>(pstmt->executeQuery()) };
            if (!res->next())
                throw sql::SQLException{ "submission ID not exists" };
            std::unique_ptr<sql::PreparedStatement> pupdate(
                m_ConnPtr->prepareStatement(
R"SQL(UPDATE submissions
SET overall_status=?
WHERE id=?;)SQL"
                )
            );
            pupdate->setString(1, Judge::toString(status));
            pupdate->setUInt(2, res->getUInt("id"));
            pupdate->executeUpdate();
        } catch (const sql::SQLException &e) {
            throw sql::SQLException(std::format("update submission status failed: {}", e.what()));
        }
    }

    void JudgeWriter::insertTestCases(const std::vector<TestCase>& test_cases, std::string_view problem_title)
    {
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_ConnPtr->prepareStatement(
R"SQL(SELECT id from problems WHERE title=?;)SQL"
                )
            );
            pstmt->setString(1, problem_title.data());
            auto result = std::unique_ptr<sql::ResultSet>(pstmt->executeQuery());
            if (!result->next())
                throw sql::SQLException{ "no such problem title exists." };
            uint32_t problem_id{ result->getUInt("id") };

            pstmt.reset(m_ConnPtr->prepareStatement(
R"SQL(INSERT INTO test_cases (problem_id, stdin, expected_output, sequence, is_hidden)
VALUES (?, ?, ?, ?, ?);)SQL"
                )
            );
            for (const auto& tc : test_cases) {
                pstmt->setUInt(1, problem_id);
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
