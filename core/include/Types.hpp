#pragma once
#include "Core.hpp"

namespace Core::Types
{
    using TimeStamp = std::chrono::time_point<std::chrono::steady_clock>;
    using Millis = std::chrono::milliseconds;
    using SubID = std::uint64_t;
    using StatusCode = std::uint8_t;
    using TestInput = std::string;
    using ExpectedOutput = std::string;
    using TestCase = std::pair<TestInput, ExpectedOutput>;
    using TestCases = std::vector<TestCase>;
}
namespace bp = boost::process;
namespace fs = std::filesystem;