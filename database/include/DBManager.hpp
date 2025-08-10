#pragma once
#include <mysql_connection.h>
#include "Core.hpp"
#include "utilities/JudgeResult.hpp"
#include "utilities/Submission.hpp"
#include "ProblemInfo.hpp"
#include "UserInfo.hpp"
#include "DBTask.hpp"

namespace Database
{

    using Core::Types::TestCase;
    struct TestCasesGenerator
    {
        struct promise_type
        {
            std::optional<TestCase> current_test_case;
            std::exception_ptr   eptr;

            inline TestCasesGenerator get_return_object() {
                return TestCasesGenerator{
                    std::coroutine_handle<promise_type>::from_promise(*this)
                };
            }

            inline std::suspend_always initial_suspend() { return {}; } // 在创建协程的开始挂起
            inline std::suspend_always final_suspend() noexcept { return {}; } // 在协程销毁前一刻挂起
            inline void unhandled_exception() { eptr = std::current_exception(); } // 协程异常 ， 保存异常至 eptr
            inline void return_void() {}

            inline std::suspend_always yield_value(TestCase t) {
                current_test_case = std::move(t);
                return {};
            }
        };

        std::coroutine_handle<promise_type> handler;

        explicit TestCasesGenerator(std::coroutine_handle<promise_type> handler);

        bool next();

        TestCase getCurrentTestCase();

        ~TestCasesGenerator();

    private:
        TestCasesGenerator(const TestCasesGenerator&) = delete;
        TestCasesGenerator& operator=(const TestCasesGenerator&) = delete;
    };

    using Judge::Language::LangID;
    using Judge::ResourceLimits;
    class DBManager
    {
    public:
        DBManager(std::string_view host
                , uint16_t port
                , std::string_view user
                , std::string_view password
                , std::string_view database);
        
        ~DBManager();

        ResourceLimits queryProblemLimits(std::string_view problem_title, LangID language_id);

        ProblemInfo queryProblemInfo(std::string_view problem_title);

        UserInfo queryUserInfo(std::string_view username);

        TestCasesGenerator queryTestCases(std::string_view problem_title);

        DBTask insertProblemLimits(std::string problem_title, LangID language_id, Judge::ResourceLimits limits);

        DBTask insertSubmission(Judge::Submission submission, Judge::SubmissionStatus status);

        DBTask insertJudgeResult(Judge::JudgeResult judge_result);
        
        DBTask insertTestCases(std::vector<TestCase> test_cases, std::string problem_title);

        DBTask insertProblem(ProblemInfo problem);

        DBTask insertUser(UserInfo user);

        bool isUsernameExist(std::string_view username);

        bool isEmailExist(std::string_view email);

    private:
        void connect();

    private:
        std::shared_ptr<sql::Connection> m_Connection;
        std::string m_Host;
        std::string m_User;
        std::string m_Password;
        std::string m_Database;
        uint16_t m_Port;
    };
}