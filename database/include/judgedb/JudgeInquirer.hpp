#pragma once
#include "MySQLDB.hpp"
#include "Core.hpp"
#include "Types.hpp"
#include "utilities/ResourceLimits.hpp"
#include "utilities/Submission.hpp"


namespace JudgeDB
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


    using Core::Types::SubID;

    class JudgeInquirer : public MySQLDB::Database
    {
    public:
        JudgeInquirer(std::string_view host
                    , uint16_t port
                    , std::string_view user
                    , std::string_view password
                    , std::string_view database);
        virtual ~JudgeInquirer() = default;

        bool isExists(SubID submission_id);

        Judge::ResourceLimits getProblemLimits(std::string_view problem_title);

        TestCasesGenerator getTestCases(std::string_view problem_title);
    };
}