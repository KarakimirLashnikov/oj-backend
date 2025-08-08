#include "utilities/ExecutionResult.hpp"

namespace Judge
{
    std::string TestResult::toString() const
    {
        std::string result{ std::format("[\n  Status: {}\n  Runtime: {} us\n  Memory: {} KB\n  Exit Code: {}\n  Exit Signal: {}\n]\n"
            , Judge::toString(status)
            , duration_us
            , memory_kb
            , exit_code
            , signal) };
        return result;
    }
}