#include "utilities/ExecutionResult.hpp"

namespace Judge
{
    std::string TestResult::toString() const
    {
        std::string result{ std::format("[\n  Status: {}\n  Runtime: {} us\n  Memory: {} KB\n  Exit Code: {}\n  Exit Signal: {}\n]\n"
            , Judge::testStatusToString(status)
            , duration_us
            , memory_kb
            , exit_code
            , signal) };
        return result;
    }

    std::string testStatusToString(TestStatus t)
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
}