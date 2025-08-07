#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include "judgedb/JudgeInquirer.hpp"
#include "Types.hpp"

namespace JudgeDB
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

    JudgeInquirer::JudgeInquirer(std::string_view host
                                , std::string_view user
                                , std::string_view password
                                , std::string_view database)
        : MySQLDB::Database{ host, user, password, database }
    {
    }


    bool JudgeInquirer::isExists(SubID submission_id)
    {
        std::string sub_id{ boost::uuids::to_string(submission_id) };
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
                "SELECT submission_id, submit_time FROM submissions "
                "WHERE submission_id=?"
            )
        );
        pstmt->setString(1, sub_id);
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        return res->next();
    }

    Judge::ResourceLimits JudgeInquirer::getProblemLimits(std::string_view problem_title)
    {
        std::unique_ptr<sql::PreparedStatement> pstmt(
            m_ConnPtr->prepareStatement(
                R"SQL(SELECT cpu_time_limit_s, cpu_extra_time_s, wall_time_limit_s, memory_limit_kb, stack_limit_kb
FROM problems
WHERE title=? LIMIT 1)SQL"
            )
        );
        pstmt->setString(1, problem_title.data());
        
        std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
        
        if (res->next()) {
            Judge::ResourceLimits limits;
            limits.cpu_time_limit_s = res->getDouble("cpu_time_limit_s");
            limits.cpu_extra_time_s = res->getDouble("cpu_extra_time_s");
            limits.wall_time_limit_s = res->getDouble("wall_time_limit_s");
            limits.memory_limit_kb = res->getInt("memory_limit_kb");
            limits.stack_limit_kb = res->getInt("stack_limit_kb");
            
            return limits;
        }
        
        throw sql::SQLException(std::format("No such problem: {}", problem_title.data()));
    }

    TestCasesGenerator JudgeInquirer::getTestCases(std::string_view problem_title)
    {
        // 首先获取题目ID
        int problem_id = -1;
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_ConnPtr->prepareStatement("SELECT id FROM problems WHERE title = ?")
            );
            pstmt->setString(1, problem_title.data());
            
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            
            if (!res->next()) {
                throw sql::SQLException("No such problem.");
            }
            
            problem_id = res->getInt("id");
        } catch (sql::SQLException& e) {
            throw sql::SQLException("select problem failed: " + std::string(e.what()));
        }
        
        // 协程生成器，逐个返回测试用例
        co_await std::suspend_never{};
        
        try {
            std::unique_ptr<sql::PreparedStatement> pstmt(
                m_ConnPtr->prepareStatement("SELECT stdin, expected_output, sequence, is_hidden "
                                    "FROM test_cases WHERE problem_id = ? "
                                    "ORDER BY sequence ASC")
            );
            pstmt->setInt(1, problem_id);
            
            std::unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
            
            while (res->next()) {
                TestCase tc;
                tc.stdin = res->getString("stdin");
                tc.expected_output = res->getString("expected_output");
                tc.sequence = res->getInt("sequence");
                tc.is_hidden = res->getBoolean("is_hidden");

                co_yield tc;
            } 
        } catch (sql::SQLException& e) {
            throw sql::SQLException("select testcases failed: " + std::string(e.what()));
        }
    }
}