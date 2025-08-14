#pragma once
#include "Core.hpp"
#include "Types.hpp"

namespace Judge
{
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

    std::string testStatusToString(TestStatus t);

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