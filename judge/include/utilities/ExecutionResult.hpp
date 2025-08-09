#pragma once
#include "Core.hpp"
#include "Types.hpp"

namespace Judge
{
    using Core::Types::TimeStamp;

    enum class TestStatus
    {
        UNKNOWN               = BITMASK(0),
        ACCEPTED              = BITMASK(1),
        WRONG_ANSWER          = BITMASK(2),
        RUNTIME_ERROR         = BITMASK(3),
        MEMORY_LIMIT_EXCEEDED = BITMASK(4),
        TIME_LIMIT_EXCEEDED   = BITMASK(5),
        INTERNAL_ERROR        = BITMASK(6),
        COMPILATION_ERROR     = BITMASK(7)
    };

    inline std::string toString(TestStatus t)
    {
        switch (t)
        {
        case TestStatus::ACCEPTED: return "ACCEPTED";
        case TestStatus::WRONG_ANSWER: return "WRONG_ANSWER";
        case TestStatus::COMPILATION_ERROR: return "COMPILATION_ERROR";
        case TestStatus::RUNTIME_ERROR: return "RUNTIME_ERROR";
        case TestStatus::TIME_LIMIT_EXCEEDED: return "TIME_LIMIT_EXCEEDED";
        case TestStatus::MEMORY_LIMIT_EXCEEDED: return "MEMORY_LIMIT_EXCEEDED";
        case TestStatus::INTERNAL_ERROR: return "INTERNAL_ERROR";
        default: throw std::out_of_range{"Invalid enum value"};
        }
    }

    struct TestResult
    {
        uint64_t duration_us{0};
        int exit_code{-1};
        int memory_kb{-1};
        int signal{-1};
        TestStatus status;

        [[nodiscard]] std::string toString() const;
    };


    struct ExecutionResult
    {
        std::string stdout;
        std::string stderr;
        TestResult test_result;
    };
}